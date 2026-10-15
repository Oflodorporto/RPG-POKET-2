// Native copied-frame recovery and modal-input regression. These are real
// Session/Training/Input headers, not an RF, ESP32 or physical-screen test.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main existing_session_suite_main
#include <coop_training_session.cpp>
#undef main
#pragma GCC diagnostic pop
#include "Coop/CoopInput.h"

namespace recoverydiag {
using coopview::Control;
unsigned scenarios=0;

struct CampaignGuard {
 Rig& rig;
 std::array<uint8_t,rpg::PERSISTENT_SAVE_SIZE> save[2];
 uint32_t rng[2];
 explicit CampaignGuard(Rig& r):rig(r),save{packed(r.game[0]),packed(r.game[1])},rng{r.game[0].randomState,r.game[1].randomState}{}
 void verify()const{for(unsigned i=0;i<2;++i){CHECK(packed(rig.game[i])==save[i]);CHECK(rig.game[i].randomState==rng[i]);}}
};
static void startAt(Rig& r,uint32_t start){
 if(r.now==start)return;
 for(auto& s:r.s)s.leave(r.now);
 r.frames.clear();r.now=start;
 for(unsigned i=0;i<2;++i)CHECK(r.s[i].open(r.game[i],mac[i],456+i,r.now));
 r.flush();
}
static Frame engineFrame(const Rig& r,unsigned source,coop::Type type,uint64_t nonce){
 coop::Packet packet{};const auto& e=r.s[source].training().endpoint;
 auto w=coop::begin(packet,type,e.local,nonce,e.state.revision);
 if(type==coop::Type::Heartbeat||type==coop::Type::Ack)w.u32(coop::stateHash(e.state));
 CHECK(coop::finish(packet,w));Frame f{};f.source=mac[source];f.destination=mac[1-source];f.size=packet.size;std::memcpy(f.bytes,packet.bytes,packet.size);return f;
}
static void resynchronize(Rig& r,unsigned host=0){
 // A valid same-session request at this exact time repairs both endpoints,
 // without advancing a nearly-expired 9,999 ms clock past its deadline.
 r.partition=false;auto request=engineFrame(r,1-host,coop::Type::SnapshotRequest,r.s[host].sessionToken());
 CHECK(r.s[host].receive(request.source,request.bytes,request.size,r.now));r.flush();r.equal();
 for(auto& s:r.s){CHECK(s.state()==SessionState::Playing);CHECK(!s.recovering());}
}
static void clearedLobby(const Session& s,uint32_t now){
 CHECK(s.state()==SessionState::Browsing&&!s.isHost()&&!s.recovering());
 CHECK(s.endReason()==SessionEnd::None&&s.notice()==SessionNotice::None);
 CHECK(!s.sessionToken()&&!s.joinToken()&&!s.pendingPeer());
 CHECK(!s.invitationConfirmed()&&!s.awaitingDecision()&&!s.decisionRemainingMs(now));
 CHECK(!s.training().active()&&!s.training().binding().nonce&&!s.training().endpoint.nonce);
 CHECK(!s.training().endpoint.hasState&&!s.training().endpoint.pending&&!s.training().endpoint.awaitingAck);
 CHECK(!s.training().endpoint.outgoing.count&&!s.training().endpoint.ui.count);
 CHECK(s.localProfile().owner==0&&s.localProfile().id==1&&admittedProfile(s.localProfile(),0));
 for(size_t i=0;i<Session::kMaxPeers;++i)CHECK(!s.peer(i));
 CHECK(s.observedTime()==now);
}
static void linkRoles(Rig& r,unsigned host){
 const unsigned guest=1-host;
 CHECK(r.s[host].host(r.now));r.flush();CHECK(r.s[guest].peer(0));
 CHECK(r.s[guest].join(0,r.now));r.flush();CHECK(r.s[host].state()==SessionState::AwaitingAccept);
 CHECK(r.s[host].accept(r.now));r.flush();
 for(auto& s:r.s)CHECK(s.state()==SessionState::Playing&&!s.recovering());
 CHECK(r.s[host].isHost()&&!r.s[guest].isHost());
 CHECK(r.s[host].localProfile().owner==0&&r.s[host].localProfile().id==1);
 CHECK(r.s[guest].localProfile().owner==1&&r.s[guest].localProfile().id==2);r.equal();
}
static void boundaries(){
 static_assert(coop::kDisconnectMs==1800&&Session::kLinkTimeoutMs==10000,"Recovery deadlines changed; review the independent boundary oracles");
 for(uint32_t start:{100u,UINT32_MAX-1000u}){
  Rig r;CampaignGuard save(r);startAt(r,start);r.link();const uint64_t token=r.s[0].sessionToken();
  const auto hash=coop::stateHash(r.s[0].training().endpoint.state);r.partition=true;
  r.advance(1799);for(auto& s:r.s){CHECK(s.state()==SessionState::Playing);CHECK(!s.recovering()&&!s.training().endpoint.paused);}
  r.advance(1);for(auto& s:r.s){CHECK(s.state()==SessionState::Playing);CHECK(s.recovering()&&s.training().endpoint.paused);CHECK(!s.training().endpoint.canPlay());}
  r.advance(8199);for(auto& s:r.s){CHECK(s.state()==SessionState::Playing&&s.recovering());CHECK(s.sessionToken()==token);CHECK(coop::stateHash(s.training().endpoint.state)==hash);}
  r.advance(1);for(auto& s:r.s){CHECK(s.state()==SessionState::Ended&&s.endReason()==SessionEnd::LinkLost);CHECK(!s.recovering()&&!s.training().active());}
  r.partition=false;r.advance(500);for(auto& s:r.s)CHECK(s.state()==SessionState::Ended&&!s.recovering());
  save.verify();++scenarios;
 }
 std::puts("PASS: exact 1,799/1,800/9,999/10,000 ms loss boundaries, terminal recovery predicate and uint32 wrap; campaign Save30/RNG unchanged");
}
static void receiveDeadlines(){
 for(unsigned side=0;side<2;++side)for(uint32_t elapsed:{9999u,10000u}){
  Rig r;CampaignGuard save(r);r.link();const auto token=r.s[side].sessionToken();
  auto live=engineFrame(r,1-side,coop::Type::Heartbeat,token);r.partition=true;r.now+=elapsed;
  const bool accepted=r.s[side].receive(live.source,live.bytes,live.size,r.now);
  CHECK(accepted==(elapsed<10000));
  CHECK(r.s[side].state()==(elapsed<10000?SessionState::Playing:SessionState::Ended));
  if(elapsed==10000)CHECK(r.s[side].endReason()==SessionEnd::LinkLost&&!r.s[side].recovering());
  save.verify();++scenarios;
 }
 for(uint32_t start:{100u,UINT32_MAX-1000u}){
  Rig r;CampaignGuard save(r);startAt(r,start);r.link();const auto oldStart=r.lastStart,oldReady=r.lastReady;
  const auto token=r.s[0].sessionToken();r.partition=true;
  for(unsigned n=0;n<40;++n){
   r.advance(250);
   for(unsigned target=0;target<2;++target){
    auto wrong=engineFrame(r,1-target,coop::Type::Heartbeat,token+1);
    CHECK(!r.s[target].receive(wrong.source,wrong.bytes,wrong.size,r.now));
    auto badMac=engineFrame(r,1-target,coop::Type::Heartbeat,token);badMac.source=mac[2];
    CHECK(!r.s[target].receive(badMac.source,badMac.bytes,badMac.size,r.now));
   }
   r.inject(0,oldReady);r.inject(1,oldStart); // Admission duplicates are not combat liveness.
   if(n<39)for(auto& session:r.s)CHECK(session.state()==SessionState::Playing);
  }
  for(auto& session:r.s)CHECK(session.state()==SessionState::Ended&&session.endReason()==SessionEnd::LinkLost);
  save.verify();++scenarios;
 }
 std::puts("PASS: valid receive at 9,999ms survives but exact 10,000ms expires before processing; duplicate admission, wrong nonce and wrong MAC never extend the combat deadline");
}
static void sameSessionRecovery(){
 for(uint32_t start:{100u,UINT32_MAX-1000u})for(uint32_t elapsed:{1800u,3000u,9999u}){
  Rig r;CampaignGuard save(r);startAt(r,start);r.link();
  const uint64_t room=r.s[0].sessionToken(),join=r.s[0].joinToken();
  const auto hash=coop::stateHash(r.s[0].training().endpoint.state);
  const uint32_t generation[2]={r.s[0].training().generationId(),r.s[1].training().generationId()};
  r.partition=true;r.advance(elapsed);for(auto& s:r.s)CHECK(s.recovering());resynchronize(r);
  for(unsigned i=0;i<2;++i){CHECK(r.s[i].sessionToken()==room&&r.s[i].joinToken()==join);CHECK(r.s[i].training().generationId()==generation[i]);CHECK(coop::stateHash(r.s[i].training().endpoint.state)==hash);}
  CHECK(r.s[0].training().endpoint.submit(coop::Action::Guard,0,r.now)==coop::Error::None);r.advance(0);r.equal();
  CHECK(r.s[0].training().endpoint.state.revision==1);save.verify();++scenarios;
 }
 std::puts("PASS: 1.8s/3s/9.999s same-session resynchronization resumes without reminting tokens, restarting training or rerolling state, including wrap");
}
static void ackOnlyLoss(){
 for(uint32_t start:{100u,UINT32_MAX-1000u}){
  Rig r;CampaignGuard save(r);startAt(r,start);r.link();r.engineDrops[unsigned(coop::Type::Ack)]=1000;
  auto& host=r.s[0].training().endpoint;
  CHECK(host.submit(coop::Action::Guard,0,r.now)==coop::Error::None);r.advance(0);
  CHECK(host.awaitingAck);const auto hash=coop::stateHash(host.state);const uint32_t revision=host.state.revision;
  for(unsigned i=0;i<39;++i)r.advance(250);r.advance(249);
  CHECK(r.s[0].state()==SessionState::Playing&&r.s[0].recovering());
  CHECK(host.awaitingAck&&host.state.revision==revision&&coop::stateHash(host.state)==hash);
  CHECK(r.heartbeats>10&&r.dropped>0); // Other traffic is alive; reliable progress is not.
  r.advance(1);CHECK(r.s[0].state()==SessionState::Ended&&r.s[0].endReason()==SessionEnd::LinkLost&&!r.s[0].recovering());
  r.engineDrops[unsigned(coop::Type::Ack)]=0;r.advance(500);
  CHECK(r.s[0].state()==SessionState::Ended);save.verify();++scenarios;
 }
 std::puts("PASS: ACK-only loss reaches the exact 10s reliable-stall deadline despite live heartbeats; late ACK cannot revive it");
}
static void cancelAndSwitchRoles(){
 for(unsigned cancelSide=0;cancelSide<2;++cancelSide)for(bool timeout:{false,true}){
  Rig r;CampaignGuard save(r);r.link();uint64_t minted[2][8]{};unsigned count[2]={};
  minted[0][count[0]++]=r.s[0].sessionToken();minted[1][count[1]++]=r.s[1].joinToken();
  for(unsigned cycle=0;cycle<4;++cycle){
   const Frame old[]={r.lastJoin,r.lastStart,r.lastReady,r.lastEngine};
   const uint64_t room=r.s[0].sessionToken(),join=r.s[0].joinToken();
   r.partition=true;r.advance(timeout?10000:1800);
   CHECK(r.s[cancelSide].returnToLobby(r.now));clearedLobby(r.s[cancelSide],r.now);
   for(const auto& f:old)if(!sameMac(f.source,mac[cancelSide]))CHECK(!r.s[cancelSide].receive(f.source,f.bytes,f.size,r.now));
   clearedLobby(r.s[cancelSide],r.now);
   CHECK(r.s[1-cancelSide].returnToLobby(r.now));clearedLobby(r.s[1-cancelSide],r.now);
   r.frames.clear();r.partition=false;
   const unsigned host=(cycle+1)%2;linkRoles(r,host); // No open() or setSender(): retained identity and sender are required.
   CHECK(r.s[0].sessionToken()!=room&&r.s[0].joinToken()!=join);
   for(unsigned side=0;side<2;++side){
    uint64_t token=side==host?r.s[side].sessionToken():r.s[side].joinToken();CHECK(token);
    for(unsigned i=0;i<count[side];++i)CHECK(token!=minted[side][i]);minted[side][count[side]++]=token;
   }
   const auto hash=coop::stateHash(r.s[0].training().endpoint.state);
   for(const auto& f:old){const unsigned target=sameMac(f.source,mac[0])?1:0;CHECK(!r.s[target].receive(f.source,f.bytes,f.size,r.now));}
   r.flush();r.equal();for(auto& s:r.s){CHECK(s.state()==SessionState::Playing&&!s.recovering());CHECK(coop::stateHash(s.training().endpoint.state)==hash);}
   save.verify();++scenarios;
  }
 }
 std::puts("PASS: both-role cancel after pause/terminal loss, four in-place role reversals, profile normalization, fresh mint stream, retained sender, stale JOIN/START/READY/engine rejection, unchanged Save30/RNG");
}
static void cancelAfterCombat(){
 Rig r;CampaignGuard save(r);const auto profiles=std::array<coop::Actor,2>{r.s[0].localProfile(),r.s[1].localProfile()};r.link();
 auto& host=r.s[0].training().endpoint;auto& guest=r.s[1].training().endpoint;
 CHECK(host.submit(coop::Action::Guard,0,r.now)==coop::Error::None);r.advance(0);
 CHECK(guest.submit(coop::Action::Guard,1,r.now)==coop::Error::None);r.advance(0);
 for(unsigned i=0;i<coop::kEnemies;++i)r.advance(Session::kEnemyPresentationMs);
 CHECK(host.state.revision>2);const auto generation=r.s[0].training().generationId();
 r.partition=true;r.advance(1800);for(auto& s:r.s){CHECK(s.returnToLobby(r.now));clearedLobby(s,r.now);}
 r.frames.clear();r.partition=false;linkRoles(r,1);
 for(unsigned side=0;side<2;++side){
  auto profile=profiles[side];profile.owner=side?0:1;profile.id=profile.owner+1;
  CHECK(admission::sameProfile(r.s[side].localProfile(),profile));
  CHECK(admission::sameProfile(r.s[side].training().endpoint.state.actors[profile.owner],profile));
  CHECK(r.s[side].training().endpoint.state.revision==0);
 }
 CHECK(r.s[0].training().generationId()==generation+1);save.verify();++scenarios;
 std::puts("PASS: post-combat cancellation discards battle revision and damage/guard state; reversed roles import the original profiles into one fresh generation");
}
static void connectedCancelNotifiesPeer(){
 for(unsigned side=0;side<2;++side){
  Rig r;CampaignGuard save(r);r.link();r.partition=true;r.advance(1800);r.partition=false;
  CHECK(r.s[side].returnToLobby(r.now));clearedLobby(r.s[side],r.now);r.flush();
  CHECK(r.s[1-side].state()==SessionState::Ended&&r.s[1-side].endReason()==SessionEnd::PeerLeft);
  CHECK(r.s[1-side].returnToLobby(r.now));clearedLobby(r.s[1-side],r.now);r.flush();
  save.verify();++scenarios;
 }
 std::puts("PASS: cancel from either recovering role sends a bound departure when the transport is available");
}
static void everyReturnState(){
 Session closed;const auto before=closed;CHECK(!closed.returnToLobby(777));CHECK(!std::memcmp(&before,&closed,sizeof(closed)));CHECK(!closed.recovering());
 for(unsigned side=0;side<2;++side)for(auto state:{SessionState::Browsing,SessionState::Hosting,SessionState::AwaitingAccept,SessionState::Joining,SessionState::Starting,SessionState::Playing,SessionState::Ended}){
  Rig r;CampaignGuard save(r);const auto original=r.s[side].localProfile();
  if(state==SessionState::Hosting){CHECK(r.s[side].host(r.now));r.flush();}
  if(state==SessionState::AwaitingAccept||state==SessionState::Joining){
   const unsigned host=state==SessionState::AwaitingAccept?side:1-side;
   CHECK(r.s[host].host(r.now));r.flush();CHECK(r.s[1-host].join(0,r.now));r.flush();
  }
  if(state==SessionState::Starting){
   r.request();r.admissionDrops[unsigned(admission::Type::Ready)]=1000;CHECK(r.s[0].accept(r.now));r.flush();
  }
  if(state==SessionState::Playing||state==SessionState::Ended){r.link();if(state==SessionState::Ended)r.s[side].leave(r.now);}
  CHECK(r.s[side].state()==state);if(state!=SessionState::Playing)CHECK(!r.s[side].recovering());
  r.partition=true;r.frames.clear();r.now+=17;
  CHECK(r.s[side].returnToLobby(r.now));clearedLobby(r.s[side],r.now);
  CHECK(admission::sameProfile(original,r.s[side].localProfile()));
  CHECK(r.s[side].peerAge(r.now)==0&&r.s[side].progressAge(r.now)==0);
  CHECK(r.s[1-side].returnToLobby(r.now));r.frames.clear();r.partition=false;
  for(auto& drops:r.admissionDrops)drops=0;
  linkRoles(r,side);save.verify();++scenarios;
 }
 std::puts("PASS: every non-Closed state returns to a clean reusable lobby; Closed is immutable, original profiles survive, peer/progress age and decisions reset");
}
static void pureDiagnosticProjection(){
 coopdiag::Signal signal;
 CHECK(!signal.valid&&!signal.fresh(0)&&!signal.bars(0));
 signal.observe(-79,6,3,UINT32_MAX-1000u);
 CHECK(signal.valid&&signal.last==-79&&signal.minimum==-79&&signal.maximum==-79&&signal.channel==6&&signal.rate==3&&signal.samples==1);
 CHECK(signal.age(1000u)==2001u&&signal.fresh(1798u)&&signal.bars(1798u)==1);
 CHECK(signal.age(1799u)==2800u&&!signal.fresh(1799u)&&signal.bars(1799u)==0);
 for(int rssi:{-127,-79,-78,-68,-67,-56,-55,-1}){
  signal.observe(int8_t(rssi),11,4,777);
  auto before=signal;const uint8_t bars=rssi>=-55?4:rssi>=-67?3:rssi>=-78?2:1;
  CHECK(signal.bars(777)==bars&&signal.bars(3576)==bars&&signal.bars(3577)==0);
  CHECK(!std::memcmp(&before,&signal,sizeof(signal)));
 }
 CHECK(signal.minimum==-127&&signal.maximum==-1&&signal.channel==11&&signal.rate==4&&signal.samples==9);
 signal.samples=UINT32_MAX;signal.observe(-70,1,2,800);CHECK(signal.samples==UINT32_MAX);
 uint32_t counter=UINT32_MAX-1;coopdiag::increment(counter);CHECK(counter==UINT32_MAX);coopdiag::increment(counter);CHECK(counter==UINT32_MAX);
 CHECK(sizeof(coopdiag::Snapshot)<=320);++scenarios;
 for(bool art:{false,true})for(bool diagnostics:{false,true}){
  Rig r;CampaignGuard save(r);r.link();r.partition=true;r.advance(1800);
  for(auto& s:r.s){
   auto before=s;const auto v=coopview::model(s,art,diagnostics);
   CHECK(v.artReady==art&&v.diagnostics==diagnostics&&v.recovering&&v.screen==coopview::Screen::Playing);
   CHECK(v.dots==uint8_t((s.observedTime()/350)%3+1));CHECK(!v.linkTools);
   CHECK(!std::memcmp(&before,&s,sizeof(s)));
   auto enabled=v;enabled.linkTools=true;
   const auto d=coopview::kDiagnostics;CHECK(coopview::hitTest(enabled,d.x,d.y)==Control::Diagnostics);
   CHECK(coopview::hitTest(enabled,d.x+d.w-1,d.y+d.h-1)==Control::Diagnostics);
   CHECK(coopview::hitTest(enabled,280,10)==Control::Exit);
   const auto c=coopview::kRecoveryCancel;
   CHECK(coopview::hitTest(enabled,c.x+c.w/2,c.y+c.h/2)==(diagnostics?Control::None:Control::ReturnLobby));
   CHECK(coopview::hitTest(enabled,50,195)==Control::None);
   CHECK(coopview::hitTest(enabled,-1,10)==Control::None&&coopview::hitTest(enabled,320,10)==Control::None);
  }
  save.verify();++scenarios;
 }
 Rig clock;for(uint32_t time:{349u,350u,699u,700u,1049u,1050u}){
  clock.now=time;clock.s[0].tick(time);auto before=clock.s[0];auto v=coopview::model(clock.s[0],false,true);
  CHECK(v.dots==(time/350)%3+1&&!v.recovering&&v.diagnostics);CHECK(!std::memcmp(&before,&clock.s[0],sizeof(before)));
 }
 ++scenarios;
 std::puts("PASS: pure diagnostic projection, 350ms animation phases, modal hit isolation, RSSI 4/3/2/1 boundaries and 2.8s stale cutoff across clock wrap; bounded/saturating counters");
}
static Control sample(Input& input,Session& session,bool valid,bool down,int x,int y,uint32_t now){
 auto p=toPortrait(x,y);input.capture(session,valid,down,p.x,p.y,now);return input.dispatch(session,now);
}
static void capture(Input& input,Session& session,bool valid,bool down,int x,int y,uint32_t now){auto p=toPortrait(x,y);input.capture(session,valid,down,p.x,p.y,now);}
static void release(Input& input,Session& session,uint32_t& now){
 CHECK(sample(input,session,true,false,0,0,now)==Control::None);now+=31;CHECK(sample(input,session,true,false,0,0,now)==Control::None);
}
static Control click(Input& input,Session& session,int x,int y,uint32_t& now){
 release(input,session,now);CHECK(sample(input,session,true,true,x,y,now)==Control::None);
 now+=8;CHECK(sample(input,session,true,false,0,0,now)==Control::None);now+=31;return sample(input,session,true,false,0,0,now);
}
static coopplatform::Point center(coopview::Rect rect){return {int16_t(rect.x+rect.w/2),int16_t(rect.y+rect.h/2)};}
static void makePlayable(Rig& r,unsigned side){
 if(side){CHECK(r.s[0].training().endpoint.submit(coop::Action::Guard,0,r.now)==coop::Error::None);r.advance(0);}
 CHECK(r.s[side].training().ui.playable(r.s[side].training().endpoint));
}
static void recoveryControls(){
 static_assert(unsigned(Control::None)==0&&unsigned(Control::Exit)==1&&unsigned(Control::Host)==2&&unsigned(Control::Join0)==3&&unsigned(Control::Join3)==6&&unsigned(Control::Accept)==7&&unsigned(Control::Decline)==8,"Existing control values are wire-independent but must remain source compatible");
 for(unsigned side=0;side<2;++side){
  Rig r;CampaignGuard save(r);r.link();Input input;auto& s=r.s[side];auto point=center(coopview::kRecoveryCancel);
  CHECK(coopview::hitTest(coopview::model(s,true),point.x,point.y)!=Control::ReturnLobby);
  r.partition=true;r.advance(1800);
  for(bool art:{false,true}){
   auto before=s;auto v=coopview::model(s,art,false);
   CHECK(!std::memcmp(&before,&s,sizeof(s)));CHECK(v.screen==coopview::Screen::Playing);
   const auto rect=coopview::kRecoveryCancel;
   for(auto p:{coopplatform::Point{rect.x,rect.y},coopplatform::Point{int16_t(rect.x+rect.w-1),rect.y},coopplatform::Point{rect.x,int16_t(rect.y+rect.h-1)},coopplatform::Point{int16_t(rect.x+rect.w-1),int16_t(rect.y+rect.h-1)},point})CHECK(coopview::hitTest(v,p.x,p.y)==Control::ReturnLobby);
   for(auto p:{coopplatform::Point{int16_t(rect.x-1),point.y},coopplatform::Point{int16_t(rect.x+rect.w),point.y},coopplatform::Point{point.x,int16_t(rect.y-1)},coopplatform::Point{point.x,int16_t(rect.y+rect.h)}})CHECK(coopview::hitTest(v,p.x,p.y)!=Control::ReturnLobby);
   CHECK(coopview::hitTest(v,280,10)==Control::Exit);
  }
  CHECK(click(input,s,point.x,point.y,r.now)==Control::ReturnLobby);CHECK(s.state()==SessionState::Playing&&s.recovering());
  CHECK(s.returnToLobby(r.now));clearedLobby(s,r.now);save.verify();++scenarios;
 }
 std::puts("PASS: both recovery overlays retain Exit and expose exact inclusive cancel bounds with/without art; capture dispatch returns an action without mutating Session");
}
static void modalGameplayIsolation(){
 for(unsigned side=0;side<2;++side)for(bool batched:{false,true}){
  Rig r;CampaignGuard save(r);r.link();makePlayable(r,side);Input input;auto& s=r.s[side];auto& t=s.training();
  auto guard=center(coopui::kButtons[2]);release(input,s,r.now);
  const auto submitted=t.ui.submitted,revision=t.endpoint.state.revision;
  if(batched){capture(input,s,true,true,guard.x,guard.y,r.now);capture(input,s,true,false,0,0,r.now+8);capture(input,s,true,false,0,0,r.now+39);}
  else CHECK(sample(input,s,true,true,guard.x,guard.y,r.now)==Control::None);
  CHECK(t.ui.submitted==submitted&&t.endpoint.state.revision==revision);
  input.setDiagnostics(true,s,r.now);CHECK(input.diagnosticsOpen());
  CHECK(input.dispatch(s,r.now)==Control::None);
  for(bool down:{false,true,false})for(unsigned n=0;n<64;++n){r.now+=8;capture(input,s,true,down,down?guard.x:0,down?guard.y:0,r.now);}
  CHECK(t.ui.submitted==submitted&&t.endpoint.state.revision==revision);CHECK(input.dispatch(s,r.now)==Control::None);
  CHECK(t.ui.submitted==submitted&&t.endpoint.state.revision==revision);
  // A finger still touching when the modal closes cannot become a Guard.
  CHECK(sample(input,s,true,true,guard.x,guard.y,r.now)==Control::None);
  input.setDiagnostics(false,s,r.now);CHECK(!input.diagnosticsOpen());
  CHECK(sample(input,s,true,true,guard.x,guard.y,r.now)==Control::None);release(input,s,r.now);
  CHECK(t.ui.submitted==submitted&&t.endpoint.state.revision==revision);
  CHECK(click(input,s,guard.x,guard.y,r.now)==Control::None);r.advance(0);
  CHECK(t.ui.submitted==submitted+1&&t.endpoint.state.revision==revision+1);save.verify();++scenarios;
 }
 std::puts("PASS: diagnostics open/close invalidates queued and held gameplay on both roles; 64-sample modal batches cannot mutate combat; fresh Guard resolves once");
}
static void modalMenuIsolation(){
 for(bool playing:{false,true})for(bool batched:{false,true}){
  Rig r;CampaignGuard save(r);if(playing)r.link();Input input;auto& s=r.s[0];
  const int x=playing?280:50,y=playing?10:195;
  release(input,s,r.now);
  if(batched){capture(input,s,true,true,x,y,r.now);capture(input,s,true,false,0,0,r.now+8);capture(input,s,true,false,0,0,r.now+39);}
  else CHECK(sample(input,s,true,true,x,y,r.now)==Control::None);
  input.setDiagnostics(true,s,r.now);CHECK(input.dispatch(s,r.now)==Control::None);release(input,s,r.now);
  input.setDiagnostics(false,s,r.now);CHECK(sample(input,s,true,true,x,y,r.now)==Control::None);release(input,s,r.now);
  CHECK(s.state()==(playing?SessionState::Playing:SessionState::Browsing));
  CHECK(click(input,s,x,y,r.now)==(playing?Control::Exit:Control::Host));
  auto point=center(coopview::kDiagnostics);CHECK(click(input,s,point.x,point.y,r.now)==Control::Diagnostics);
  CHECK(!input.diagnosticsOpen());input.setDiagnostics(true,s,r.now);CHECK(input.diagnosticsOpen());
  CHECK(click(input,s,point.x,point.y,r.now)==Control::Diagnostics);input.setDiagnostics(false,s,r.now);
  save.verify();++scenarios;
 }
 std::puts("PASS: diagnostics transitions invalidate queued/held Host and Exit menu gestures; explicit clean diagnostics toggles work in lobby and combat");
}
static void recoveryTransitionGestures(){
 for(unsigned side=0;side<2;++side)for(bool queued:{false,true}){
  Rig r;CampaignGuard save(r);r.link();makePlayable(r,side);Input input;auto& s=r.s[side];
  auto guard=center(coopui::kButtons[2]);auto cancel=center(coopview::kRecoveryCancel);release(input,s,r.now);
  const auto submitted=s.training().ui.submitted;
  if(queued)capture(input,s,true,true,guard.x,guard.y,r.now);else CHECK(sample(input,s,true,true,guard.x,guard.y,r.now)==Control::None);
  r.partition=true;r.advance(1800);CHECK(s.recovering());
  CHECK(input.dispatch(s,r.now)==Control::None);CHECK(sample(input,s,true,true,cancel.x,cancel.y,r.now)==Control::None);release(input,s,r.now);
  CHECK(s.state()==SessionState::Playing&&s.training().ui.submitted==submitted);
  // A cancel held while recovery finishes must not become a later action.
  CHECK(sample(input,s,true,true,cancel.x,cancel.y,r.now)==Control::None);resynchronize(r);release(input,s,r.now);
  CHECK(s.state()==SessionState::Playing&&!s.recovering()&&s.training().ui.submitted==submitted);
  r.partition=true;r.advance(1800);CHECK(click(input,s,cancel.x,cancel.y,r.now)==Control::ReturnLobby);
  CHECK(s.returnToLobby(r.now));clearedLobby(s,r.now);save.verify();++scenarios;
 }
 std::puts("PASS: queued/held gameplay cannot become recovery cancel, and cancel held across same-session reconnect cannot fire; fresh cancel remains reachable");
}
static void terminalControls(){
 for(unsigned side=0;side<2;++side)for(unsigned reason=0;reason<6;++reason){
  Rig r;CampaignGuard save(r);Input input;auto& s=r.s[side];
  SessionEnd expected=SessionEnd::None;
  if(reason==1){
   const unsigned host=1-side;CHECK(r.s[host].host(r.now));r.flush();r.admissionDrops[unsigned(admission::Type::Waiting)]=1000;
   CHECK(s.join(0,r.now));r.flush();r.advance(12000);expected=SessionEnd::JoinTimeout;
  }else{
   r.link();
   if(reason==0){r.partition=true;r.advance(10000);expected=SessionEnd::LinkLost;}
   if(reason==2){s.leave(r.now);r.flush();expected=SessionEnd::Left;}
   if(reason==3){r.s[1-side].leave(r.now);r.flush();expected=SessionEnd::PeerLeft;}
   if(reason>=4){
    auto state=s.training().endpoint.state;
    if(reason==4){state.wave=2;coop::spawnWave(state);for(unsigned i=coop::kPlayerCapacity;i<coop::kActors;++i)state.actors[i].hp=0;expected=SessionEnd::Won;}
    else{state.actors[0].hp=state.actors[1].hp=0;expected=SessionEnd::Lost;}
    coop::advance(state);CHECK(coop::validState(state));
    for(auto& session:r.s){auto& endpoint=session.training().endpoint;endpoint.state=state;endpoint.awaitingAck=endpoint.pending=false;}
    r.advance(0);
   }
  }
  CHECK(s.state()==SessionState::Ended&&s.endReason()==expected&&!s.recovering());
  auto p=center(coopview::kTerminalReturn);auto v=coopview::model(s,true);CHECK(coopview::hitTest(v,p.x,p.y)==Control::Exit);
  v.linkTools=true;const auto desired=reason<2?Control::ReturnLobby:Control::Exit;
  CHECK(coopview::hitTest(v,p.x,p.y)==desired);CHECK(click(input,s,p.x,p.y,r.now)==desired);
  input.setDiagnostics(true,s,r.now);CHECK(click(input,s,p.x,p.y,r.now)==Control::None);
  CHECK(s.state()==SessionState::Ended&&s.endReason()==expected);input.setDiagnostics(false,s,r.now);
  CHECK(click(input,s,p.x,p.y,r.now)==desired);r.partition=true;CHECK(s.returnToLobby(r.now));clearedLobby(s,r.now);
  save.verify();++scenarios;
 }
 std::puts("PASS: both-role LinkLost/JoinTimeout central controls return to lobby; Won/Lost/Left/PeerLeft retain Guild exit; modal diagnostic capture cannot activate either action");
}
static void diagnosticsGestureFaults(){
 for(unsigned fault=0;fault<5;++fault){
  Rig r;CampaignGuard save(r);r.link();Input input;auto& s=r.s[0];const auto point=center(coopview::kDiagnostics);release(input,s,r.now);
  const auto submitted=s.training().ui.submitted,revision=s.training().endpoint.state.revision;
  CHECK(sample(input,s,true,true,point.x,point.y,r.now)==Control::None);
  if(fault==0){CHECK(sample(input,s,false,true,point.x,point.y,r.now)==Control::None);CHECK(sample(input,s,true,true,point.x,point.y,r.now)==Control::None);}
  if(fault==1){CHECK(sample(input,s,true,true,200,40,r.now)==Control::None);CHECK(sample(input,s,true,true,point.x,point.y,r.now)==Control::None);}
  if(fault==2){for(unsigned n=0;n<20;++n)capture(input,s,n%2,true,point.x,point.y,++r.now);CHECK(input.dispatch(s,r.now)==Control::None);CHECK(input.dropped>0);}
  if(fault==3){r.partition=true;r.advance(1800);CHECK(s.recovering());}
  if(fault==4){input.setDiagnostics(true,s,r.now);input.setDiagnostics(false,s,r.now);}
  release(input,s,r.now);CHECK(!input.diagnosticsOpen());CHECK(s.training().ui.submitted==submitted&&s.training().endpoint.state.revision==revision);
  CHECK(click(input,s,point.x,point.y,r.now)==Control::Diagnostics);input.setDiagnostics(true,s,r.now);CHECK(input.diagnosticsOpen());
  input.reset();CHECK(!input.diagnosticsOpen());CHECK(sample(input,s,true,true,point.x,point.y,r.now)==Control::None);release(input,s,r.now);
  save.verify();++scenarios;
 }
 std::puts("PASS: invalid/drag/overflow/recovery/modal-transition diagnostic gestures fail closed, fresh toggle succeeds, reset suppresses carried contact");
}

}

int main(){
 std::setvbuf(stdout,nullptr,_IONBF,0);const auto initialAllocations=allocations;
 using namespace recoverydiag;
 boundaries();receiveDeadlines();sameSessionRecovery();ackOnlyLoss();cancelAndSwitchRoles();cancelAfterCombat();connectedCancelNotifiesPeer();
 everyReturnState();pureDiagnosticProjection();recoveryControls();modalGameplayIsolation();modalMenuIsolation();recoveryTransitionGestures();terminalControls();diagnosticsGestureFaults();
 CHECK(allocations==initialAllocations);
 std::printf("PASS: %u recovery/modal-input scenarios, %u checks, zero C++ allocations; native copied-frame evidence only\n",scenarios,checks);
}
