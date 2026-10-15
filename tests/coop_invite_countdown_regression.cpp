// Independent bounded-invitation regression. The existing controller fixture is
// generated outside the checkout, with its helpers and production code intact.
#include "coop_invite_controller_fixture.generated.h"
#include <limits>

#ifndef EXPECT_INVITE_COUNTDOWN
#define EXPECT_INVITE_COUNTDOWN 1
#endif

namespace invitediag {
using namespace coopplatform;
constexpr Mac addresses[3]={{{2,0,0,0,0,1}},{{2,0,0,0,0,2}},{{2,0,0,0,0,3}}};
constexpr unsigned kWaiting=8,kExpired=9,kTypes=10;
static unsigned cases=0;
#define REQUIRE(x) do { if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort();} } while(0)
struct Frame {unsigned from=0;Mac source{},destination{};uint16_t size=0;uint8_t bytes[coop::kMaxPacket]{};};
struct Rig {
 Game games[2]={::hero(0,0),::hero(0,2)};Session sessions[2];
 struct Port {Rig* rig;unsigned side;} ports[2]{{this,0},{this,1}};
 coop::Queue<Frame,256> frames{},held{};Frame last[kTypes]{};
 unsigned drops[kTypes]{},counts[kTypes]{};bool hold[kTypes]{};bool partition=false;
 uint32_t time=100,requestedAt=0;
 explicit Rig(uint32_t start=100):time(start){
  for(unsigned i=0;i<2;++i){sessions[i].setSender(send,&ports[i]);REQUIRE(sessions[i].open(games[i],addresses[i],453+i,time));}flush();
 }
 static bool send(void* context,const Mac& destination,const uint8_t* bytes,size_t size){
  auto& p=*static_cast<Port*>(context);auto& r=*p.rig;REQUIRE(size<=coop::kMaxPacket);
  Frame f{};f.from=p.side;f.source=addresses[p.side];f.destination=destination;f.size=uint16_t(size);std::memcpy(f.bytes,bytes,size);
  admission::Header h{};
  if(admission::decode(bytes,size,h)){
   const unsigned type=unsigned(h.type);REQUIRE(type<kTypes);r.last[type]=f;++r.counts[type];
   if(r.drops[type]){--r.drops[type];return true;}
   if(r.hold[type]){REQUIRE(r.held.push(f));return true;}
  }
  if(r.partition)return true;
  REQUIRE(r.frames.push(f));return true;
 }
 void flush(){Frame f{};unsigned n=0;while(frames.pop(f)){
  REQUIRE(++n<2000);const unsigned target=1-f.from;
  if(sameMac(f.destination,addresses[target])||f.destination.bytes[0]==255)sessions[target].receive(f.source,f.bytes,f.size,time);
 }}
 void step(uint32_t ms){time+=ms;for(auto& s:sessions)s.tick(time);flush();}
 void deliverHeld(){Frame f{};while(held.pop(f)){const unsigned target=1-f.from;sessions[target].receive(f.source,f.bytes,f.size,time);}flush();}
 void discover(){REQUIRE(sessions[0].host(time));flush();REQUIRE(sessions[1].peer(0));}
 void request(){discover();requestedAt=time;REQUIRE(sessions[1].join(0,time));flush();}
 bool inject(unsigned side,const Frame& f){return sessions[side].receive(f.source,f.bytes,f.size,time);}
 void unchanged(const Game& a,const Game& b){REQUIRE(packed(games[0])==packed(a)&&packed(games[1])==packed(b));REQUIRE(games[0].randomState==a.randomState&&games[1].randomState==b.randomState);}
 void quiet(){REQUIRE(!sessions[0].training().active()&&!sessions[1].training().active());REQUIRE(!sessions[0].training().generationId()&&!sessions[1].training().generationId());}
};
static Frame control(const Mac& source,unsigned type,uint64_t room,uint64_t join,uint32_t value=0,const coop::Actor* profile=nullptr){
 admission::Packet packet{};auto w=admission::begin(packet,admission::Type(type),room,join);
 if(profile)coop::writeActor(w,*profile);
 if(type==kWaiting||type==unsigned(admission::Type::Start)||type==unsigned(admission::Type::Ready))w.u32(value);
 REQUIRE(admission::finish(packet,w));Frame f{};f.source=source;f.size=packet.size;std::memcpy(f.bytes,packet.bytes,packet.size);return f;
}
static void historicalTimeout(){
 for(uint32_t start:{100u,UINT32_MAX-6000u}){
  Rig r(start);const Game a=r.games[0],b=r.games[1];r.request();const uint64_t token=r.sessions[0].sessionToken();
  r.step(11999);REQUIRE(r.sessions[0].state()==SessionState::AwaitingAccept&&r.sessions[1].state()==SessionState::Joining);r.quiet();
  r.step(1);REQUIRE(r.sessions[0].state()==SessionState::Hosting&&r.sessions[1].state()==SessionState::Ended);REQUIRE(r.sessions[1].endReason()==SessionEnd::JoinTimeout);
  REQUIRE(r.sessions[0].sessionToken()!=token&&!r.sessions[0].accept(r.time));r.quiet();
  r.step(18000);REQUIRE(!r.sessions[0].accept(r.time));r.step(90000);REQUIRE(!r.sessions[0].accept(r.time));r.unchanged(a,b);++cases;
 }
 std::puts("PASS baseline: delayed acceptance expires at exactly 12,000 ms on both boards, including millis wrap; 30s/2min cannot start; Game/Save30/RNG unchanged");
}

#if EXPECT_INVITE_COUNTDOWN
// Candidate cases are below. All timing expectations are independent literals;
// a change to the production constants must not silently alter these oracles.
static void expectPending(Rig& r,uint32_t remaining){
 REQUIRE(r.sessions[0].state()==SessionState::AwaitingAccept&&r.sessions[1].state()==SessionState::Joining);
 for(auto& s:r.sessions){
  REQUIRE(s.awaitingDecision()&&s.decisionRemainingMs(r.time)==remaining&&s.observedTime()==r.time);
  auto before=s;auto v=coopview::model(s,true);
  REQUIRE(v.showDecisionCountdown&&v.decisionSeconds==(remaining+999)/1000&&!v.synchronizing);
  REQUIRE(std::memcmp(&before,&s,sizeof(s))==0);
 }
 REQUIRE(r.sessions[1].invitationConfirmed()&&!r.sessions[0].invitationConfirmed());r.quiet();
}
static void expectExpired(Rig& r){
 REQUIRE(r.sessions[0].state()==SessionState::Hosting&&!r.sessions[0].pendingPeer());
 REQUIRE(r.sessions[0].notice()==SessionNotice::InviteExpired);
 REQUIRE(r.sessions[1].state()==SessionState::Ended&&r.sessions[1].endReason()==SessionEnd::InviteExpired&&r.sessions[1].notice()==SessionNotice::InviteExpired);
 for(auto& s:r.sessions){auto v=coopview::model(s,true);REQUIRE(!s.awaitingDecision()&&!s.decisionRemainingMs(r.time)&&!v.showDecisionCountdown&&!v.allowAccept);}
 REQUIRE(coopview::model(r.sessions[1],true).end==coopview::EndReason::InviteExpired);r.quiet();
}
static void fixedDeadline(){
 static_assert(Session::kAcceptTimeoutMs==30000&&Session::kAdmissionTimeoutMs==12000,"Human and technical deadlines must remain distinct");
 for(uint32_t start:{100u,UINT32_MAX-15000u}){
  Rig r(start);const auto a=r.games[0],b=r.games[1];r.request();const auto room=r.sessions[0].sessionToken();auto original=r.last[unsigned(admission::Type::Join)];
  expectPending(r,30000);
  uint32_t elapsed=0;
  for(uint32_t milestone:{1u,999u,1000u,11999u,12000u,29999u}){r.step(milestone-elapsed);elapsed=milestone;expectPending(r,30000-elapsed);REQUIRE(r.inject(0,original));r.flush();expectPending(r,30000-elapsed);}
  r.step(1);expectExpired(r);REQUIRE(r.sessions[0].sessionToken()!=room);REQUIRE(!r.sessions[0].accept(r.time)&&!r.sessions[0].decline(r.time));
  for(unsigned i=0;i<360;++i){r.step(250);REQUIRE(!r.inject(0,original));}expectExpired(r);r.unchanged(a,b);++cases;
 }
 std::puts("PASS: fixed 30,000 ms decision deadline; both countdowns 30/29/18/1 then expiry; duplicates and two-minute delay cannot extend or autoaccept; millis wrap safe");
}
static void exactBoundary(){
 for(uint32_t start:{100u,UINT32_MAX-15000u})for(unsigned method=0;method<3;++method){
  Rig r(start);r.request();r.time+=30000;
  if(method==0)REQUIRE(!r.sessions[0].accept(r.time));
  else if(method==1)REQUIRE(!r.sessions[0].decline(r.time));
  else REQUIRE(!r.inject(0,r.last[unsigned(admission::Type::Join)]));
  r.flush();REQUIRE(r.sessions[0].state()==SessionState::Hosting);REQUIRE(r.sessions[1].endReason()==SessionEnd::InviteExpired);r.quiet();++cases;
 }
 for(uint32_t start:{100u,UINT32_MAX-15000u}){
  Rig r(start);r.request();r.step(29999);REQUIRE(r.sessions[0].accept(r.time));r.flush();
  REQUIRE(r.sessions[0].state()==SessionState::Playing&&r.sessions[1].state()==SessionState::Playing);
  REQUIRE(coop::stateHash(r.sessions[0].training().endpoint.state)==coop::stateHash(r.sessions[1].training().endpoint.state));
  REQUIRE(r.sessions[0].training().generationId()==1&&r.sessions[1].training().generationId()==1);++cases;
 }
 for(uint32_t start:{100u,UINT32_MAX-15000u}){
  Rig r(start);r.request();auto late=control(addresses[0],unsigned(admission::Type::Start),r.sessions[0].sessionToken(),r.sessions[0].joinToken(),123,&r.sessions[0].localProfile());
  r.time+=30000;REQUIRE(!r.inject(1,late));REQUIRE(r.sessions[1].endReason()==SessionEnd::InviteExpired);r.quiet();++cases;
 }
 std::puts("PASS: explicit acceptance at 29,999 ms succeeds once; accept/decline/JOIN receive at exact 30,000 ms expire first, including wrap");
}
static void wireAndBinding(){
 static_assert(admission::kVersion==2&&unsigned(admission::Type::Waiting)==8&&unsigned(admission::Type::Expired)==9&&admission::kTypeCount==10,"Bounded invitation wire contract changed");
 Rig r;r.request();auto room=r.sessions[0].sessionToken(),join=r.sessions[0].joinToken();const auto pending=*r.sessions[0].pendingPeer();
 for(unsigned type:{kWaiting,kExpired}){
  auto valid=control(addresses[0],type,room,join,30000);admission::Header h{};REQUIRE(admission::decode(valid.bytes,valid.size,h));REQUIRE(h.payload==(type==kWaiting?4:0));
  for(size_t n=0;n<valid.size;++n)REQUIRE(!r.sessions[1].receive(valid.source,valid.bytes,n,r.time));
  auto extra=valid;++extra.size;REQUIRE(!r.inject(1,extra));
  for(unsigned field:{4u,5u,6u,7u,24u,25u,unsigned(valid.size-1)}){auto bad=valid;bad.bytes[field]^=0x40;REQUIRE(!r.inject(1,bad));}
  auto old=valid;old.bytes[4]=1;const uint32_t crc=coop::digest(old.bytes,old.size-4);for(unsigned i=0;i<4;++i)old.bytes[old.size-4+i]=uint8_t(crc>>(8*i));REQUIRE(!r.inject(1,old));
  for(unsigned zero=0;zero<2;++zero){auto invalid=control(addresses[0],type,zero?room:0,zero?0:join,30000);REQUIRE(!r.inject(1,invalid));}
  admission::Packet wrongPayload{};auto w=admission::begin(wrongPayload,admission::Type(type),room,join);if(type==kExpired)w.u32(123);
  REQUIRE(admission::finish(wrongPayload,w));REQUIRE(!r.sessions[1].receive(addresses[0],wrongPayload.bytes,wrongPayload.size,r.time));
  for(unsigned fault=0;fault<4;++fault){
   auto bad=control(fault==0?addresses[2]:addresses[0],type,room+(fault==1),join+(fault==2),30000);
   if(fault==3)bad=control(addresses[1],type,room,join,30000);
   REQUIRE(!r.inject(fault==3?0:1,bad));expectPending(r,30000);
  }
  ++cases;
 }
 for(uint32_t budget:{0u,30001u,UINT32_MAX}){auto invalid=control(addresses[0],kWaiting,room,join,budget);REQUIRE(!r.inject(1,invalid));expectPending(r,30000);++cases;}
 for(unsigned fault=0;fault<4;++fault){
  auto profile=pending.profile;if(fault==0)++profile.attack;else if(fault==1)profile.owner=0;else if(fault==2)profile.cls=4;else profile.hp=0;
  auto altered=control(addresses[1],unsigned(admission::Type::Join),room,join,0,&profile);const auto replies=r.counts[kWaiting];REQUIRE(!r.inject(0,altered));
  REQUIRE(admission::sameProfile(r.sessions[0].pendingPeer()->profile,pending.profile)&&r.counts[kWaiting]==replies);expectPending(r,30000);++cases;
 }
 for(unsigned fault=0;fault<3;++fault){
  auto competing=control(fault==0?addresses[2]:addresses[1],unsigned(admission::Type::Join),room+(fault==1),join+(fault==2),0,&pending.profile);
  REQUIRE(!r.inject(0,competing));r.flush();REQUIRE(r.sessions[0].joinToken()==join&&sameMac(r.sessions[0].pendingPeer()->mac,addresses[1]));expectPending(r,30000);++cases;
 }
 r.step(30000);expectExpired(r);
 std::puts("PASS: v2 Waiting/Expired exact payload, version isolation, malformed lengths/CRC/fields, wrong MAC/token/role, out-of-range budgets and altered/invalid duplicate profiles fail closed");
}
static void retryAndDelay(){
 {Rig r;r.drops[unsigned(admission::Type::Join)]=3;r.request();REQUIRE(r.sessions[0].state()==SessionState::Hosting&&!r.sessions[1].invitationConfirmed());
  r.step(250);r.step(250);r.step(250);expectPending(r,30000);r.step(29999);expectPending(r,1);r.step(1);expectExpired(r);++cases;}
 {Rig r;r.drops[kWaiting]=3;r.request();REQUIRE(!r.sessions[1].invitationConfirmed());
  r.step(250);r.step(250);r.step(250);expectPending(r,29250);r.step(29249);expectPending(r,1);r.step(1);expectExpired(r);++cases;}
 {Rig r;r.hold[unsigned(admission::Type::Join)]=true;r.request();r.step(11999);REQUIRE(r.sessions[0].state()==SessionState::Hosting&&!r.sessions[1].invitationConfirmed());
  r.hold[unsigned(admission::Type::Join)]=false;r.deliverHeld();expectPending(r,30000);r.step(29999);expectPending(r,1);r.step(1);expectExpired(r);REQUIRE(uint32_t(r.time-r.requestedAt)==41999);++cases;}
 {Rig r;r.hold[kWaiting]=true;r.request();auto original=r.last[kWaiting];r.step(5000);REQUIRE(!r.sessions[1].invitationConfirmed());
  REQUIRE(r.inject(1,original));REQUIRE(r.sessions[1].decisionRemainingMs(r.time)==30000);
  auto newer=r.last[kWaiting];REQUIRE(r.inject(1,newer));REQUIRE(r.sessions[1].decisionRemainingMs(r.time)==25000);
  REQUIRE(r.inject(1,original));REQUIRE(r.sessions[1].decisionRemainingMs(r.time)==25000);r.hold[kWaiting]=false;r.held.clear();
  r.step(24999);expectPending(r,1);r.step(1);expectExpired(r);++cases;}
 // A first Waiting delayed until the final millisecond of the technical
 // response budget can create only one 30s estimate. All further copies are
 // stale and must never reset it. Lost Expired cannot cause an unbounded wait.
 for(uint32_t start:{100u,UINT32_MAX-15000u}){
  Rig r(start);r.hold[kWaiting]=true;r.drops[kExpired]=1;r.request();auto initial=r.last[kWaiting];
  r.step(11999);REQUIRE(!r.sessions[1].invitationConfirmed());REQUIRE(r.inject(1,initial));REQUIRE(r.sessions[1].decisionRemainingMs(r.time)==30000);
  r.held.clear();r.hold[kWaiting]=false;r.partition=true;
  for(unsigned i=0;i<119;++i){r.step(250);REQUIRE(r.inject(1,initial));}
  r.step(249);REQUIRE(r.sessions[1].state()==SessionState::Joining&&r.sessions[1].decisionRemainingMs(r.time)==1);
  r.step(1);REQUIRE(r.sessions[1].state()==SessionState::Ended&&r.sessions[1].endReason()==SessionEnd::InviteExpired);
  REQUIRE(uint32_t(r.time-r.requestedAt)==41999);r.quiet();++cases;
 }
 std::puts("PASS: lost first JOIN/Waiting retry; delayed/reordered Waiting only shortens; lost Expired plus maximum delayed first reply ends by 41,999 ms, including wrap");
}
static void technicalTimeouts(){
 for(bool loseJoin:{false,true})for(uint32_t start:{100u,UINT32_MAX-6000u}){
  Rig r(start);r.drops[loseJoin?unsigned(admission::Type::Join):kWaiting]=1000;r.request();
  r.step(11999);REQUIRE(r.sessions[1].state()==SessionState::Joining&&!r.sessions[1].invitationConfirmed());auto v=coopview::model(r.sessions[1],true);REQUIRE(!v.showDecisionCountdown&&!v.synchronizing);
  auto late=control(addresses[0],kWaiting,r.sessions[1].sessionToken(),r.sessions[1].joinToken(),30000);r.time+=1;REQUIRE(!r.inject(1,late));
  REQUIRE(r.sessions[1].endReason()==SessionEnd::JoinTimeout&&r.sessions[1].notice()!=SessionNotice::InviteExpired);r.quiet();++cases;
 }
 {Rig r;r.request();r.drops[kExpired]=1000;r.partition=true;r.step(30000);expectExpired(r);++cases;}
 for(unsigned dropped:{unsigned(admission::Type::Start),unsigned(admission::Type::Ready)}){
  Rig r;r.request();r.drops[dropped]=1000;REQUIRE(r.sessions[0].accept(r.time));r.flush();
  auto before=coopview::model(r.sessions[0],true);REQUIRE(before.synchronizing&&!before.showDecisionCountdown);
  const auto room=r.sessions[0].sessionToken(),join=r.sessions[0].joinToken();
  for(unsigned type:{kWaiting,kExpired})for(unsigned target=0;target<2;++target){auto f=control(addresses[1-target],type,room,join,30000);if(r.sessions[target].state()==SessionState::Starting)REQUIRE(!r.inject(target,f));}
  r.step(11999);REQUIRE(r.sessions[0].state()==SessionState::Starting);r.step(1);REQUIRE(r.sessions[0].state()==SessionState::Ended&&r.sessions[0].endReason()==SessionEnd::JoinTimeout);++cases;
 }
 {Rig r;r.request();REQUIRE(r.sessions[0].accept(r.time));r.flush();auto room=r.sessions[0].sessionToken(),join=r.sessions[0].joinToken();
  auto hash=coop::stateHash(r.sessions[0].training().endpoint.state);
  for(unsigned type:{kWaiting,kExpired})for(unsigned target=0;target<2;++target){auto f=control(addresses[1-target],type,room,join,30000);REQUIRE(!r.inject(target,f));REQUIRE(r.sessions[target].state()==SessionState::Playing);}
  REQUIRE(coop::stateHash(r.sessions[0].training().endpoint.state)==hash);++cases;}
 std::puts("PASS: absent first reply and START/READY synchronization keep separate 12s technical limits; Waiting/Expired rejected after Starting/Playing without state mutation");
}
static void declineRetryAndStaleStart(){
 for(bool decline:{false,true}){
  Rig r;r.request();const auto room=r.sessions[0].sessionToken(),join=r.sessions[0].joinToken();const auto oldJoin=r.last[unsigned(admission::Type::Join)];
  auto oldStart=control(addresses[0],unsigned(admission::Type::Start),room,join,123,&r.sessions[0].localProfile());
  if(decline){r.step(1234);REQUIRE(r.sessions[0].decline(r.time));r.flush();REQUIRE(r.sessions[1].notice()==SessionNotice::Declined&&r.sessions[1].endReason()==SessionEnd::JoinTimeout);REQUIRE(coopview::model(r.sessions[1],true).end==coopview::EndReason::Declined);}
  else{r.step(30000);expectExpired(r);}
  REQUIRE(!r.inject(1,oldStart));REQUIRE(!r.inject(0,oldJoin));r.quiet();
  REQUIRE(r.sessions[1].open(r.games[1],addresses[1],454,r.time));r.flush();REQUIRE(r.sessions[1].peer(0)&&r.sessions[1].join(0,r.time));r.flush();
  expectPending(r,30000);REQUIRE(r.sessions[0].sessionToken()!=room&&r.sessions[1].joinToken()!=join);
  REQUIRE(!r.inject(1,oldStart)&&!r.inject(0,oldJoin));
  auto oldWaiting=control(addresses[0],kWaiting,room,join,30000),oldExpiry=control(addresses[0],kExpired,room,join);
  REQUIRE(!r.inject(1,oldWaiting)&&!r.inject(1,oldExpiry));expectPending(r,30000);
  REQUIRE(r.sessions[0].accept(r.time));r.flush();REQUIRE(r.sessions[0].state()==SessionState::Playing&&r.sessions[1].state()==SessionState::Playing);++cases;
 }
 std::puts("PASS: manual Declined is distinct from InviteExpired; explicit fresh retry rotates both tokens; old JOIN/START/Waiting/Expired cannot revive or autoaccept");
}
struct ControllerPair {
 Session peer;bool localHost;uint32_t requestedAt=0;
 struct Port {ControllerPair* pair;unsigned side;} ports[2]{{this,0},{this,1}};
 coop::Queue<Frame,128> frames{};
 explicit ControllerPair(bool host):localHost(host){
  setup(0,0,0);enterGuild();press(175,238);REQUIRE(coopIsOpen());
  coopSession.setSender(send,&ports[0]);peer.setSender(send,&ports[1]);REQUIRE(peer.open(game,addresses[1],0xabcdef,now));flush();
  if(localHost){coopClick(50,195);flush();REQUIRE(peer.peer(0)&&peer.join(0,now));flush();}
  else{REQUIRE(peer.host(now));flush();REQUIRE(coopSession.peer(0));coopClick(200,60);flush();}
  requestedAt=now;REQUIRE(hostSession().state()==SessionState::AwaitingAccept&&guestSession().state()==SessionState::Joining);
 }
 Session& hostSession(){return localHost?coopSession:peer;}
 Session& guestSession(){return localHost?peer:coopSession;}
 static bool send(void* context,const Mac& destination,const uint8_t* bytes,size_t size){
  auto& p=*static_cast<Port*>(context);Frame f{};f.from=p.side;f.source=addresses[p.side];f.destination=destination;REQUIRE(size<=sizeof(f.bytes));f.size=uint16_t(size);std::memcpy(f.bytes,bytes,size);REQUIRE(p.pair->frames.push(f));return true;
 }
 void flush(){Frame f{};unsigned n=0;while(frames.pop(f)){
  REQUIRE(++n<2000);unsigned target=1-f.from;if(sameMac(f.destination,addresses[target])||f.destination.bytes[0]==255)(target?peer:coopSession).receive(f.source,f.bytes,f.size,now);
 }}
 void step(uint32_t ms){now+=ms;tickCoopTraining(now);peer.tick(now);flush();}
 void exitAndReopen(const Game& before,const std::map<std::string,std::vector<uint8_t>>& blobs){
  coopClick(280,10);REQUIRE(coopClosing&&coopIsOpen());step(349);REQUIRE(coopIsOpen());step(1);
  REQUIRE(!coopIsOpen()&&view.page==Page::Guild&&coopradio::owner==coopradio::Owner::None&&!pendingTouch&&!gate.armed);
  REQUIRE(packed(game)==packed(before)&&game.randomState==before.randomState&&nvs.blobs==blobs&&!fake::nvsWrites&&!fake::erases);
  portrait(true,true,120,292);REQUIRE(view.page==Page::Guild);press(120,292);REQUIRE(view.page==Page::Village);enterGuild();press(175,238);
  REQUIRE(coopIsOpen()&&coopSession.state()==SessionState::Browsing);flush();
  coopSample(true,true,280,10);coopRelease();REQUIRE(!coopClosing);
  coopClick(280,10);step(350);REQUIRE(!coopIsOpen()&&view.page==Page::Guild&&coopradio::owner==coopradio::Owner::None);
  REQUIRE(packed(game)==packed(before)&&game.randomState==before.randomState&&nvs.blobs==blobs&&!fake::nvsWrites&&!fake::erases);
 }
};
static void controllerCases(){
 for(bool host:{true,false})for(uint32_t delay:{30000u,120000u}){
  ControllerPair pair(host);const auto before=game;const auto blobs=nvs.blobs;
  auto v=coopview::model(coopSession,true);REQUIRE(v.showDecisionCountdown&&v.decisionSeconds==30);
  coopRelease();coopSample(true,true,50,195); // A held old acceptance contact.
  const uint32_t elapsed=uint32_t(now-pair.requestedAt);REQUIRE(elapsed<delay);pair.step(delay-elapsed);
  REQUIRE(pair.hostSession().state()==SessionState::Hosting&&pair.guestSession().state()==SessionState::Ended);
  REQUIRE(pair.hostSession().notice()==SessionNotice::InviteExpired&&pair.guestSession().endReason()==SessionEnd::InviteExpired);
  REQUIRE(!coopSession.training().active()&&!pair.peer.training().active()&&!coopClosing);
  coopRelease();REQUIRE(!coopClosing&&pair.hostSession().state()==SessionState::Hosting);
  REQUIRE(coopIsOpen()&&view.page==Page::CoopTraining&&!coopview::model(coopSession,true).showDecisionCountdown);
  pair.exitAndReopen(before,blobs);++cases;
 }
 // A previously captured accept cannot become a fresh invitation's accept,
 // even when a new request arrives at the same screen/control coordinates.
 {
  ControllerPair pair(true);const auto before=game;const auto blobs=nvs.blobs;const auto room=coopSession.sessionToken();
  coopRelease();auto p=toPortrait(50,195);captureCoopTouch(true,true,p.x,p.y,now);
  now=pair.requestedAt+30000;coopSession.tick(now);pair.peer.tick(now);pair.flush();
  REQUIRE(coopSession.state()==SessionState::Hosting&&coopSession.sessionToken()!=room);
  REQUIRE(pair.peer.open(game,addresses[1],0xabcdef,now));pair.flush();REQUIRE(pair.peer.join(0,now));pair.flush();
  REQUIRE(coopSession.state()==SessionState::AwaitingAccept);const auto stale=coopInput.stale;
  captureCoopTouch(true,true,p.x,p.y,now); // Continuation of the old held contact.
  captureCoopTouch(true,false,0,0,now);now+=31;captureCoopTouch(true,false,0,0,now);tickCoopTraining(now);pair.flush();
  REQUIRE(coopInput.stale>stale&&coopSession.state()==SessionState::AwaitingAccept&&!coopSession.training().active());
  coopClick(50,195);pair.flush();REQUIRE(coopSession.state()==SessionState::Playing&&pair.peer.state()==SessionState::Playing);
  pair.exitAndReopen(before,blobs);++cases;
 }
 std::puts("PASS: real controller host/guest 30s and two-minute expiry, stale held/captured Accept cannot autoaccept, fresh retry succeeds; Exit/reentry restores portrait/radio and preserves Save30/NVS/campaign RNG");
}
static void candidateCases(){fixedDeadline();exactBoundary();wireAndBinding();retryAndDelay();technicalTimeouts();declineRetryAndStaleStart();controllerCases();}
#endif
}

int main(){
 std::setvbuf(stdout,nullptr,_IONBF,0);
#if EXPECT_INVITE_COUNTDOWN
 invitediag::candidateCases();
#else
 invitediag::historicalTimeout();
#endif
 std::printf("PASS: %u invitation scenarios; native simulation only\n",invitediag::cases);
}
