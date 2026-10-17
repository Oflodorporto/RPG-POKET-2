#include "WifiRadioTestFlow.h"
#include "WifiRadioTestCsv.h"
#include <assert.h>
#include <stdio.h>
#include <deque>
#include <vector>
#include <functional>
using namespace wifi_radio_test;
static const uint8_t macs[3][6]={{2,1,0,0,0,1},{2,1,0,0,0,2},{2,1,0,0,0,3}};
struct Packet {unsigned source;WireFrame wire;std::vector<uint8_t> bytes;};
struct Rig {
 struct Side {Rig*rig;unsigned id;int8_t actual=80,baseline=80,ceiling=80;bool failRestore=false,failApply=false;unsigned applied=0,restored=0;uint32_t applyDelay=0;};
 uint32_t now;Flow f[2];Side side[2];std::deque<Packet> queue;std::vector<Packet> log;
 bool partition=false,dropBaseline=false,duplicate=false,reverse=false;unsigned count=0;
 std::function<bool(const Packet&)> drop;
 std::function<bool(const Packet&)> apiFail;
 explicit Rig(uint32_t start=100):now(start),side{{this,0},{this,1}}{
  for(unsigned i=0;i<2;++i){Hooks h;h.context=&side[i];h.nowMs=clock;h.send=send;h.applyPower=apply;h.restorePower=restore;f[i].setHooks(h);assert(f[i].begin(macs[i],100+i,{80,80,true,0},80,now));}
 }
 static uint32_t clock(void*p){return static_cast<Side*>(p)->rig->now;}
 static PowerResult apply(void*p,int8_t raw){Side&s=*static_cast<Side*>(p);++s.applied;s.actual=s.rig->f[s.id].selectedLevel()==Level::Baseline?raw:(raw<s.ceiling?raw:s.ceiling);s.rig->now+=s.applyDelay;return {raw,s.actual,!s.failApply,s.failApply?-7:0};}
 static PowerResult restore(void*p){Side&s=*static_cast<Side*>(p);++s.restored;if(!s.failRestore)s.actual=s.baseline;return {s.baseline,s.actual,true,s.failRestore?-8:0};}
 static int32_t send(void*p,const uint8_t*,const uint8_t*bytes,size_t size){Side&s=*static_cast<Side*>(p);Rig&r=*s.rig;Packet q;q.source=s.id;q.bytes.assign(bytes,bytes+size);assert(decode(bytes,size,q.wire));r.log.push_back(q);if(r.apiFail&&r.apiFail(q))return -20;r.queue.push_back(q);return 0;}
 void deliver(const Packet&q){if(partition||(dropBaseline&&(side[q.source].actual==80||side[1-q.source].actual==80))||(drop&&drop(q)))return;f[1-q.source].receive(macs[q.source],q.bytes.data(),q.bytes.size(),int8_t(-40-int(q.wire.seq%7)),true,now);}
 void flush(){unsigned safe=0;while(!queue.empty()){assert(++safe<1000);Packet q=reverse?queue.back():queue.front();if(reverse)queue.pop_back();else queue.pop_front();deliver(q);if(duplicate)deliver(q);}}
 void step(uint32_t ms=10){now+=ms;f[0].tick(now);f[1].tick(now);flush();}
 void advance(uint32_t ms){while(ms){uint32_t n=ms<10?ms:10;step(n);ms-=n;}}
 void until(const std::function<bool()>&predicate,uint32_t bound){uint32_t start=now;while(!predicate()&&!elapsed(now,start,bound))step();if(!predicate())fprintf(stderr,"timeout now%u states%u/%u reports%zu/%zu reasons%u/%u runs%u/%u started%d/%d unique%u/%u\n",now,unsigned(f[0].state()),unsigned(f[1].state()),f[0].reportCount(),f[1].reportCount(),unsigned(f[0].reason()),unsigned(f[1].reason()),f[0].current().run,f[1].current().run,f[0].current().localStarted,f[1].current().localStarted,f[0].current().local.uniqueRx,f[1].current().local.uniqueRx);assert(predicate());}
 void pair(){size_t prior=f[0].reportCount();step();assert(f[0].peerCount()==1&&f[1].peerCount()==1);assert(f[0].invitePeer(macs[1],now));flush();assert(f[1].state()==State::IncomingInvite);assert(!f[1].pairedIdentity());assert(f[1].acceptInvite(now));flush();assert(f[0].pairedIdentity()&&f[1].pairedIdentity());assert(f[0].state()==State::DistancePause&&f[1].state()==State::DistancePause);assert(f[0].selectedDistance()==Distance::TwoPalms);assert(f[0].reportCount()==prior);}
 void bothContinue(){for(auto&v:f)assert(v.continueAtDistance(v.selectedDistance(),now));flush();}
 void bothManual(Distance d,Level l){for(auto&v:f)assert(v.armManual(d,l,now));flush();}
 void running(){until([&]{return f[0].state()==State::Running&&f[1].state()==State::Running;},4000);}
 void report(size_t n){until([&]{return f[0].reportCount()>=n&&f[1].reportCount()>=n;},35000);}
 Packet first(Type type,unsigned src=0){for(const auto&p:log)if(p.wire.type==type&&p.source==src)return p;assert(false);return {};}
};
static void allFifteen(){
 Rig r;r.pair();
 for(unsigned d=1;d<=3;++d){assert(r.f[0].selectedDistance()==Distance(d));r.bothContinue();r.until([&]{return r.f[0].reportCount()==d*5&&r.f[1].reportCount()==d*5;},160000);}
 for(auto&f:r.f){assert(f.state()==State::Complete&&f.reportCount()==15);for(size_t i=0;i<15;++i){const auto&a=*f.reportAt(i);assert(a.stage==i+1&&a.complete&&a.paired&&a.remoteRestoreConfirmed);assert(a.local.txAttempts==30&&a.local.apiAccepted==30&&a.local.apiErrors==0&&a.local.uniqueRx==30&&a.remote.uniqueRx==30);assert(lossPermille(a,true)==0&&lossPermille(a,false)==0);assert(a.local.echoTxAttempts==30&&a.local.echoApiAccepted==30&&a.local.echoApiErrors==0);assert(a.local.controlTxAttempts>0&&a.local.controlApiAccepted+a.local.controlApiErrors==a.local.controlTxAttempts);assert(uint32_t(a.local.endedAt-a.armedAt)+kPreliminaryMs<=kArmedLimitMs);assert(a.localPower.requestedRaw==requestedPower(Level(i%5),80));assert(a.local.rssiValid&&a.local.rssiMin==-46&&a.local.rssiMax==-40);}}
 assert(r.side[0].restored==15&&r.side[1].restored==15);puts("PASS all15 automatic rounds; both Continue at each distance;30 probes/direction and fresh RSSI");
}
static void lossReorderAndStale(){
 Rig r;r.duplicate=true;r.reverse=true;r.drop=[](const Packet&p){return p.wire.type==Type::Probe&&p.source==0&&p.wire.seq%3==0;};r.pair();r.bothManual(Distance::TwoPalms,Level::N2);r.report(1);
 const Report&a=*r.f[0].reportAt(0);assert(a.local.txAttempts==30&&a.remote.uniqueRx==20&&a.local.uniqueRx==30);assert(lossPermille(a,true)==333&&lossPermille(a,false)==0);assert(a.local.echoes==20);
 Packet old=r.first(Type::Probe);uint32_t oldrun=a.run;r.bothManual(Distance::TwoPalms,Level::N3);r.running();assert(r.f[1].current().local.uniqueRx==0);assert(!r.f[1].receive(macs[0],old.bytes.data(),old.bytes.size(),-2,true,r.now));r.f[0].onMacSend(oldrun,0,true,r.now);assert(r.f[0].current().local.macSuccess==0);r.report(2);assert(r.f[0].reportAt(1)->run!=oldrun);puts("PASS packetloss, duplicate/reordered frames, stale prior run and callback rejection");
}
static void manualBaselineUnreachable(){
 Rig r;r.pair();r.dropBaseline=true;r.bothContinue();r.report(1);for(auto&f:r.f){assert(f.state()==State::ManualPause);assert(!f.reportAt(0)->localStarted&&lossPermille(*f.reportAt(0),true)==-1);}
 // N1-N3 use a power with reachable transport; every chosen distance is local
 // input, never an inferred range. Final summaries at baseline are all dropped.
 for(unsigned d=1;d<=3;++d)for(unsigned l=1;l<=3;++l){size_t n=r.f[0].reportCount();r.bothManual(Distance(d),Level(l));assert(r.side[0].actual==requestedPower(Level(l),80));r.report(n+1);for(auto&f:r.f){const auto&a=*f.reportAt(n);assert(a.complete&&comparable(a)&&!a.remoteRestoreConfirmed&&a.localRestoreAttempted);assert(lossPermille(a,true)==0);assert(f.state()==State::ManualPause);}}
 // N4 equals the original20dBm baseline, so a loss-at-baseline fixture also
 // loses N4: this is explicitly unknown rather than a fabricated improvement.
 r.bothManual(Distance::TwoMeters,Level::N4);r.report(11);assert(!r.f[0].reportAt(10)->localStarted);assert(r.side[0].actual==80&&r.side[1].actual==80);puts("PASS baseline-dropped manual levels/distances; pre-restore counts retained; missing final unknown restoration");
}
static void handshakeAndIdentity(){
 Rig r;r.pair();assert(r.f[0].continueAtDistance(Distance::TwoPalms,r.now));r.advance(9000);assert(r.f[0].state()==State::ManualPause);assert(r.f[1].state()==State::DistancePause);assert(!r.f[0].reportAt(0)->localStarted);assert(lossPermille(*r.f[0].reportAt(0),true)==-1);
 r.bothManual(Distance::OneMeter,Level::N1);r.running();Packet p=r.first(Type::Ready);p.wire.run=r.f[1].current().run;uint8_t bytes[kMaxFrame];size_t n=encode(p.wire,bytes,sizeof(bytes));assert(!r.f[1].receive(macs[2],bytes,n,-30,true,r.now));p.wire.session++;n=encode(p.wire,bytes,sizeof(bytes));assert(!r.f[1].receive(macs[0],bytes,n,-30,true,r.now));r.until([&]{return r.f[0].reportCount()==2&&r.f[1].reportCount()==1;},35000);assert(r.f[0].reportAt(1)->complete);
 Rig bad;bad.pair();assert(bad.f[0].armManual(Distance::OneMeter,Level::N1,bad.now));assert(bad.f[1].armManual(Distance::TwoMeters,Level::N1,bad.now));bad.report(1);assert(bad.f[0].reportAt(0)->configMismatch&&bad.f[1].reportAt(0)->configMismatch);assert(!bad.f[0].reportAt(0)->localStarted);puts("PASS one-sided Continue, lost handshake, identity/session/config mismatches");
}
static void timeoutCancelRestore(){
 Rig r;r.pair();r.bothManual(Distance::OneMeter,Level::N1);r.running();r.side[0].failRestore=true;r.f[0].cancel(r.now);assert(r.f[0].state()==State::Fatal&&r.f[0].powerArmed());assert(!r.f[0].armManual(Distance::OneMeter,Level::N2,r.now));assert(!r.f[0].begin(macs[0],999,{80,80,true,0},80,r.now));assert(r.f[0].current().localRestore.error==-8);r.f[0].acknowledgeCleanup(false);assert(r.f[0].state()==State::Fatal);r.f[0].acknowledgeCleanup(true);assert(r.f[0].state()==State::Cancelled);
 Rig timeout;timeout.pair();timeout.bothManual(Distance::OneMeter,Level::N2);timeout.running();timeout.partition=true;timeout.now+=30000;for(auto&f:timeout.f)f.tick(timeout.now);for(unsigned i=0;i<2;++i){assert(timeout.side[i].actual==80&&!timeout.f[i].powerArmed());assert(timeout.f[i].state()==State::Finalizing);}timeout.advance(3000);assert(timeout.f[0].state()==State::ManualPause);
 Rig cancel;cancel.pair();cancel.bothManual(Distance::OneMeter,Level::N3);cancel.running();cancel.f[0].cancel(cancel.now);assert(cancel.side[0].actual==80&&cancel.f[0].state()==State::Cancelled);cancel.flush();cancel.advance(4000);assert(cancel.side[1].actual==80);Rig hardware;hardware.pair();hardware.bothManual(Distance::TwoPalms,Level::N1);hardware.running();hardware.f[0].deviceFailure(Reason::PowerFailure,{20,44,true,-77},hardware.now);hardware.f[0].cancel(hardware.now);assert(hardware.f[0].state()==State::Cancelled);assert(hardware.f[0].reportAt(0)->reason==Reason::PowerFailure&&hardware.f[0].reportAt(0)->localPower.actualRaw==44&&hardware.f[0].reportAt(0)->localPower.error==-77);assert(!hardware.f[0].reportAt(0)->complete&&hardware.f[0].reportAt(0)->localRestore.actualRaw==80);
 Rig dual;dual.pair();dual.bothManual(Distance::TwoPalms,Level::N1);dual.running();dual.side[0].failRestore=true;dual.f[0].deviceFailure(Reason::ConfigMismatch,{20,44,true,-88},dual.now);assert(dual.f[0].state()==State::Fatal&&dual.f[0].reason()==Reason::RestoreFailure);assert(dual.f[0].reportAt(0)->reason==Reason::ConfigMismatch&&dual.f[0].reportAt(0)->localRestore.error==-8);
 Rig lobby;lobby.f[0].deviceFailure(Reason::ConfigMismatch,{80,0,false,-88},lobby.now);lobby.f[0].cancel(lobby.now);assert(lobby.f[0].reportAt(0)->reason==Reason::ConfigMismatch&&lobby.f[0].reportAt(0)->distance==Distance::PairingNear);
 puts("PASS independent armed deadline, cancellation, fatalrestore and preserveddevicefailure");
}
static void overflowMissingAndApi(){
 Rig r;r.pair();r.bothManual(Distance::TwoPalms,Level::N1);r.running();r.f[0].noteQueueOverflow();r.f[0].onMacSendCounts(100000,3,r.now);r.report(1);const auto&a=*r.f[0].reportAt(0);assert(a.queueOverflow&&!a.telemetryCorrelated&&a.local.macSuccess==65535&&a.local.macErrors==3&&lossPermille(a,true)==-1);assert(r.f[1].reportAt(0)->queueOverflow);
 Rig absent;absent.pair();absent.drop=[](const Packet&p){return p.wire.type==Type::Preliminary||p.wire.type==Type::Final;};absent.bothManual(Distance::TwoPalms,Level::N2);absent.report(1);assert(!absent.f[0].reportAt(0)->remotePresent&&lossPermille(*absent.f[0].reportAt(0),true)==-1);
 Rig errors;errors.pair();errors.apiFail=[](const Packet&p){return p.wire.type==Type::Probe&&p.source==0&&p.wire.seq%2==0;};errors.bothManual(Distance::TwoPalms,Level::N1);errors.report(1);const auto&e=*errors.f[0].reportAt(0);assert(e.local.txAttempts==30&&e.local.apiAccepted==15&&e.local.apiErrors==15&&e.remote.uniqueRx==15&&lossPermille(e,true)==0);puts("PASS queue overflow invalidation; APIerrors separated; missing report unknown");
}
static void clocksAndHistory(){
 Rig r(UINT32_MAX-1500);r.pair();r.bothManual(Distance::TwoPalms,Level::N1);r.running();r.advance(100);uint32_t old=r.now;r.now+=200;r.f[0].onMacSendCounts(1,0,r.now);r.f[0].tick(old);assert(r.f[0].state()==State::Running&&r.f[0].powerArmed());r.report(1);assert(r.f[0].reportAt(0)->complete);
 Rig delayed;delayed.pair();delayed.side[0].applyDelay=500;uint32_t before=delayed.now;assert(delayed.f[0].armManual(Distance::TwoPalms,Level::N1,before));delayed.f[0].tick(before);assert(delayed.f[0].state()==State::Armed);delayed.f[0].cancel(delayed.now);size_t kept=delayed.f[0].reportCount();assert(delayed.f[0].begin(macs[0],999,{80,80,true,0},80,delayed.now));assert(delayed.f[0].reportCount()==kept);
 for(unsigned n=0;n<26;++n){// Ring is exercised on the retained selected pair.
  assert(r.f[0].armManual(Distance::TwoPalms,Level::N1,r.now));r.f[0].cancel(r.now);r.f[0].acknowledgeCleanup(true);
  // Cancelled is terminal. Reopen then explicitly pair, preserving the ring.
  assert(r.f[0].begin(macs[0],100,{80,80,true,0},80,r.now));r.f[1].cancel(r.now);assert(r.f[1].begin(macs[1],101,{80,80,true,0},80,r.now));r.queue.clear();r.pair();
 }
 assert(r.f[0].reportCount()==24&&r.f[0].reportAt(24)==nullptr);puts("PASS uint32wrap, future callbacks, slowapply and history persistence/ringbound");
}

static void timingAdversaries(){
 // Reproduction from independent review: no lost probes, only delayed StartAck.
 Rig delayed;delayed.pair();uint32_t start=delayed.now;delayed.drop=[&](const Packet&p){return p.wire.type==Type::StartAck&&!elapsed(delayed.now,start,2100);};delayed.bothManual(Distance::TwoPalms,Level::N1);delayed.report(1);
 for(auto&f:delayed.f){assert(!comparable(*f.reportAt(0)));assert(lossPermille(*f.reportAt(0),true)==-1);assert(!f.powerArmed());}
 // Missed scheduling slot must not be silently compressed into a catch-up burst.
 Rig stall;stall.pair();stall.bothManual(Distance::TwoPalms,Level::N2);stall.running();stall.now+=950;stall.f[0].tick(stall.now);assert(stall.f[0].state()==State::Finalizing);assert(stall.f[0].current().reason==Reason::TimingInvalid);assert(!stall.f[0].current().local.timingValid);assert(stall.side[0].actual==80);
 // Original capture timestamp, rather than queue drain time, controls RTT; old
 // captured frames cannot add successes into a current reception window.
 Rig stale;stale.pair();stale.bothManual(Distance::TwoPalms,Level::N3);stale.running();stale.advance(750);Packet probe=stale.first(Type::Probe);uint32_t capture=stale.now;stale.now+=300;assert(!stale.f[1].receive(macs[0],probe.bytes.data(),probe.bytes.size(),-40,true,capture));stale.report(1);assert(!stale.f[1].reportAt(0)->local.timingValid);
 Rig finalGap;finalGap.pair();finalGap.bothManual(Distance::TwoPalms,Level::N1);finalGap.running();finalGap.advance(22100);finalGap.now+=1000;finalGap.f[0].tick(finalGap.now);assert(!finalGap.f[0].current().local.timingValid);assert(finalGap.f[0].current().reason==Reason::TimingInvalid);
 puts("PASS partialStartAck loss, missedprobe slot and stalecallback timestamp invalidate comparison");
}
static void malformedAndRestoreProof(){
 Rig r;r.pair();r.bothManual(Distance::TwoPalms,Level::N1);r.running();r.advance(1000);Packet p=r.first(Type::Probe);p.wire.type=Type::Preliminary;p.wire.stats.txAttempts=30;p.wire.stats.apiAccepted=31;uint8_t data[kMaxFrame];size_t n=encode(p.wire,data,sizeof(data));assert(!r.f[1].receive(macs[0],data,n,-30,true,r.now));assert(r.f[1].current().configMismatch);r.report(1);assert(lossPermille(*r.f[1].reportAt(0),true)==-1);
 Rig power;power.pair();power.bothManual(Distance::TwoPalms,Level::N2);power.running();power.advance(700);p=power.first(Type::Probe);p.wire.power.actualRaw=0;n=encode(p.wire,data,sizeof(data));assert(!power.f[1].receive(macs[0],data,n,-30,true,power.now));assert(power.f[1].current().configMismatch);
 Rig restore;restore.pair();restore.bothManual(Distance::TwoPalms,Level::N1);restore.running();restore.until([&]{return restore.f[0].state()==State::Finalizing&&restore.f[1].state()==State::Finalizing;},30000);p=restore.first(Type::Final);p.wire.restore={20,20,true,0};p.wire.flags|=SeenRemoteRestore|RestoreAttempted;n=encode(p.wire,data,sizeof(data));assert(restore.f[1].receive(macs[0],data,n,-30,true,restore.now));assert(!restore.f[1].current().remoteRestoreConfirmed);restore.drop=[](const Packet&p){return p.wire.type==Type::Final;};restore.report(1);assert(!restore.f[1].reportAt(0)->remoteRestoreConfirmed);
 // A finalized row is deliberately immutable. A late final cannot turn manual
 // pause into an automatic sweep or retroactively claim verified restoration.
 auto before=*restore.f[1].reportAt(0);p=restore.first(Type::Final);p.wire.flags|=SeenRemoteRestore;n=encode(p.wire,data,sizeof(data));assert(!restore.f[1].receive(macs[0],data,n,-30,true,restore.now));assert(restore.f[1].reportAt(0)->remoteRestoreConfirmed==before.remoteRestoreConfirmed&&restore.f[1].state()==State::ManualPause);
 puts("PASS impossibletelemetry, powerbounds, frozen remotebaseline proof and immutable latefinal");
}
static void pairingAndConfigLimits(){
 Rig alone;alone.partition=true;alone.advance(31000);assert(alone.f[0].state()==State::Browsing&&alone.f[0].reportCount()==1);assert(alone.f[0].reportAt(0)->distance==Distance::PairingNear);size_t n=alone.log.size();alone.advance(1000);assert(alone.log.size()>n&&alone.f[0].reportCount()==1);
 Rig reject;reject.step();assert(reject.f[0].invitePeer(macs[1],reject.now));reject.flush();reject.f[1].rejectInvite(reject.now);reject.flush();assert(!reject.f[0].pairedIdentity()&&reject.f[0].state()==State::Browsing);
 Rig cap;for(unsigned i=0;i<2;++i){cap.side[i].ceiling=52;assert(cap.f[i].begin(macs[i],100+i,{80,80,true,0},52,cap.now));}cap.pair();cap.bothManual(Distance::TwoMeters,Level::N4);cap.report(1);assert(cap.f[0].reportAt(0)->localPower.requestedRaw==80&&cap.f[0].reportAt(0)->localPower.actualRaw==52&&comparable(*cap.f[0].reportAt(0)));
 Rig bucket;for(unsigned i=0;i<2;++i){bucket.side[i].baseline=bucket.side[i].actual=34;bucket.side[i].ceiling=32;assert(bucket.f[i].begin(macs[i],100+i,{34,34,true,0},32,bucket.now));}bucket.pair();bucket.bothContinue();bucket.report(1);assert(bucket.f[0].reportAt(0)->localPower.actualRaw==34&&bucket.f[0].reportAt(0)->localRestore.actualRaw==34);bucket.report(2);assert(bucket.f[0].reportAt(1)->localPower.actualRaw==20);bucket.report(3);assert(bucket.f[0].reportAt(2)->localPower.requestedRaw==44&&bucket.f[0].reportAt(2)->localPower.actualRaw==32);
 for(int baseline:{34,66}){Rig exact;int cap=baseline==34?32:64;for(unsigned i=0;i<2;++i){exact.side[i].baseline=exact.side[i].actual=int8_t(baseline);exact.side[i].ceiling=int8_t(cap);assert(exact.f[i].begin(macs[i],100+i,{int8_t(baseline),int8_t(baseline),true,0},int8_t(cap),exact.now));}exact.pair();exact.bothManual(Distance::TwoMeters,Level::N4);exact.report(1);for(auto&f:exact.f){const auto&r=*f.reportAt(0);assert(r.localPower.requestedRaw==80&&r.localPower.actualRaw==cap&&r.localRestore.actualRaw==baseline&&r.remoteRestoreConfirmed);}}
 Rig cfg;cfg.f[1].setRadioConfig(6,7,1);cfg.pair();cfg.bothManual(Distance::TwoPalms,Level::N1);cfg.report(1);assert(cfg.f[0].reportAt(0)->configMismatch&&!cfg.f[0].reportAt(0)->localStarted);
 // More than four discovered identities never allocate or replace the chosen one.
 Flow scanner;assert(scanner.begin(macs[0],1,{80,80,true,0},80,1));for(unsigned i=0;i<8;++i){uint8_t mac[6]={2,3,0,0,0,uint8_t(i+1)};WireFrame f;f.type=Type::Beacon;f.nonce=i+2;f.power={80,80,true,0};f.baselineRaw=80;uint8_t bytes[kMaxFrame];size_t len=encode(f,bytes,sizeof(bytes));scanner.receive(mac,bytes,len,-20,true,i+2);}assert(scanner.peerCount()==4);
 puts("PASS continuingdiscovery, explicitreject, frozen52raw cap, PS mismatch and fourpeerbound");
}
int main(){allFifteen();lossReorderAndStale();manualBaselineUnreachable();handshakeAndIdentity();timeoutCancelRestore();overflowMissingAndApi();clocksAndHistory();timingAdversaries();malformedAndRestoreProof();pairingAndConfigLimits();puts("wifi_radio_test_flow PASS");}
