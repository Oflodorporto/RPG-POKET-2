#pragma once
// UI fixtures driven by the real finite protocol; simulated transport, no RF claims.
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
