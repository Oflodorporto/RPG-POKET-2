// Host regression of real firmware admission + authoritative RAM-only combat.
// No ESP32 timing/radio claim: copied bounded frames are delivered by this rig.
#include "../firmware/RPG_POKET_2/Coop/CoopSession.h"
#include "../firmware/RPG_POKET_2/Coop/CoopEligibilityBridge.h"
#include "../firmware/RPG_POKET_2/ClubProtocol.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <array>
using namespace coopplatform;
static unsigned checks=0,allocations=0;
#define CHECK(x) do {++checks;assert(x);} while(0)
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
constexpr Mac mac[3]={{{2,0,0,0,0,1}},{{2,0,0,0,0,2}},{{2,0,0,0,0,3}}};
static rpg::Game hero(uint8_t cls,unsigned level){
 auto g=rpg::create(cls,123+cls);while(g.p.level<level){g.p.xp=rpg::xpNeeded(g);rpg::levelUp(g);}
 g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.race=cls;g.shirt=cls+1;g.trousers=7-cls;
 CHECK(rpg::persistentValid(g));return g;
}
static std::array<uint8_t,rpg::PERSISTENT_SAVE_SIZE> packed(const rpg::Game& g){
 std::array<uint8_t,rpg::PERSISTENT_SAVE_SIZE> out{};rpg::encodePersistent(g,47,0,out.data());return out;
}
struct Frame {Mac source{},destination{};uint16_t size=0;uint8_t bytes[coop::kMaxPacket]{};};
struct Rig {
 rpg::Game game[2]={hero(0,20),hero(2,20)};Session s[2];
 struct Port {Rig* rig=nullptr;unsigned from=0;} ports[2];
 coop::Queue<Frame,512> frames{};Frame lastJoin{},lastStart{},lastReady{},lastEngine{};
 uint32_t now=100;unsigned admissionDrops[8]{},engineDrops[7]{},dropped=0,heartbeats=0;
 bool partition=false,sendFailure=false;
 Rig(){for(unsigned i=0;i<2;++i){ports[i]={this,i};s[i].setSender(&send,&ports[i]);CHECK(s[i].open(game[i],mac[i],456+i,now));}flush();}
 static bool send(void* ctx,const Mac& destination,const uint8_t* bytes,size_t size){
  auto& port=*static_cast<Port*>(ctx);auto& r=*port.rig;CHECK(size<=coop::kMaxPacket);
  Frame f{};f.source=mac[port.from];f.destination=destination;f.size=uint16_t(size);std::memcpy(f.bytes,bytes,size);
  admission::Header h{};coop::Header e{};coop::Packet packet{};packet.size=f.size;std::memcpy(packet.bytes,bytes,size);
  if(admission::decode(bytes,size,h)){
   if(h.type==admission::Type::Join)r.lastJoin=f;
   if(h.type==admission::Type::Start)r.lastStart=f;
   if(h.type==admission::Type::Ready)r.lastReady=f;
   if(r.admissionDrops[unsigned(h.type)]){--r.admissionDrops[unsigned(h.type)];++r.dropped;return true;}
  }else if(coop::decode(packet,e)){
   r.lastEngine=f;if(e.type==coop::Type::Heartbeat)++r.heartbeats;
   if(r.engineDrops[unsigned(e.type)]){--r.engineDrops[unsigned(e.type)];++r.dropped;return true;}
  }
  if(r.partition){++r.dropped;return true;}
  if(r.sendFailure)return false;
  CHECK(r.frames.push(f));return true;
 }
 void flush(){
  Frame f{};unsigned delivered=0;
  while(frames.pop(f)){
   CHECK(++delivered<2000);
   for(unsigned i=0;i<2;++i)if(!sameMac(f.source,mac[i])&&(sameMac(f.destination,mac[i])||f.destination.bytes[0]==255))
    s[i].receive(f.source,f.bytes,f.size,now);
  }
 }
 void advance(uint32_t ms){now+=ms;for(auto& session:s)session.tick(now);flush();}
 void discover(){CHECK(s[0].host(now));flush();CHECK(s[1].peer(0));CHECK(s[1].peer(0)->profile.cls==game[0].p.cls);}
 void request(){discover();CHECK(s[1].join(0,now));flush();CHECK(s[0].state()==SessionState::AwaitingAccept);CHECK(!s[0].training().active());CHECK(!s[1].training().active());}
 void link(){request();CHECK(s[0].accept(now));flush();CHECK(s[0].state()==SessionState::Playing);CHECK(s[1].state()==SessionState::Playing);equal();}
 void equal(){CHECK(coop::stateHash(s[0].training().endpoint.state)==coop::stateHash(s[1].training().endpoint.state));}
 void inject(unsigned target,const Frame& f){s[target].receive(f.source,f.bytes,f.size,now);}
};
static Frame admissionFrame(const Mac& source,admission::Type type,uint64_t host,uint64_t join,const coop::Actor* profile=nullptr,uint32_t value=0){
 admission::Packet p{};auto w=admission::begin(p,type,host,join);
 if(type==admission::Type::Beacon)w.u8(1);
 if(profile)coop::writeActor(w,*profile);
 if(type==admission::Type::Start||type==admission::Type::Ready)w.u32(value);
 CHECK(admission::finish(p,w));Frame out{};out.source=source;out.size=p.size;std::memcpy(out.bytes,p.bytes,p.size);return out;
}
static void basicAndRetry(){
 Rig r;const auto before0=packed(r.game[0]),before1=packed(r.game[1]);
 r.request();Frame original=r.lastJoin;
 for(unsigned i=0;i<20;++i)r.inject(0,original);
 CHECK(r.s[0].state()==SessionState::AwaitingAccept&&r.s[0].joinToken()==r.s[1].joinToken());
 CHECK(r.s[0].pendingPeer()->profile.race==r.game[1].race);
 CHECK(r.s[0].pendingPeer()->profile.shirt==r.game[1].shirt);
 CHECK(r.s[0].pendingPeer()->profile.trousers==r.game[1].trousers);
 r.admissionDrops[unsigned(admission::Type::Start)]=4;
 r.admissionDrops[unsigned(admission::Type::Ready)]=4;
 r.engineDrops[unsigned(coop::Type::Snapshot)]=3;
 r.engineDrops[unsigned(coop::Type::Ack)]=4;
 CHECK(r.s[0].accept(r.now));r.flush();
 for(unsigned i=0;i<35&&(r.s[0].state()!=SessionState::Playing||r.s[1].state()!=SessionState::Playing);++i)r.advance(250);
 CHECK(r.s[0].state()==SessionState::Playing&&r.s[1].state()==SessionState::Playing);r.equal();CHECK(r.dropped>=15);
 CHECK(r.s[0].training().generationId()==1&&r.s[1].training().generationId()==1);
 CHECK(packed(r.game[0])==before0&&packed(r.game[1])==before1);
 // The first import is an immutable projection; the session has no Game pointer.
 CHECK(r.s[0].training().endpoint.state.actors[1].maxhp==r.game[1].p.maxhp);
 const auto hash=coop::stateHash(r.s[0].training().endpoint.state);
 r.inject(0,original);r.flush();CHECK(coop::stateHash(r.s[0].training().endpoint.state)==hash);
 std::puts("PASS: deliberate admission, copied profiles, duplicate JOIN, lost START/READY/snapshot/ACK converge once");
}
static void discoveryJoinRetries(){
 Rig r;r.admissionDrops[unsigned(admission::Type::Beacon)]=2;CHECK(r.s[0].host(r.now));r.flush();CHECK(!r.s[1].peer(0));
 for(unsigned i=0;i<4&&!r.s[1].peer(0);++i)r.advance(700);
 CHECK(r.s[1].peer(0));r.admissionDrops[unsigned(admission::Type::Join)]=3;
 CHECK(r.s[1].join(0,r.now));r.flush();CHECK(r.s[0].state()==SessionState::Hosting);
 for(unsigned i=0;i<4&&r.s[0].state()==SessionState::Hosting;++i)r.advance(250);
 CHECK(r.s[0].state()==SessionState::AwaitingAccept);CHECK(r.s[0].accept(r.now));r.flush();r.equal();
 // Mere passage of time with valid heartbeats never creates an idle deadline.
 for(unsigned i=0;i<400;++i)r.advance(750);
 CHECK(r.s[0].state()==SessionState::Playing&&r.s[1].state()==SessionState::Playing);
 std::puts("PASS: discovery/JOIN retries and five-minute idle session stay bounded and live");
}
static void validationAndCollisions(){
 Rig r;r.request();const auto token=r.s[0].joinToken();const auto pending=r.s[0].pendingPeer()->profile;
 Frame other=admissionFrame(mac[2],admission::Type::Join,r.s[0].sessionToken(),token+1,&pending);
 r.inject(0,other);CHECK(sameMac(r.s[0].pendingPeer()->mac,mac[1])&&r.s[0].joinToken()==token);
 Frame changed=r.lastJoin;auto changedProfile=pending;changedProfile.attack++;
 changed=admissionFrame(mac[1],admission::Type::Join,r.s[0].sessionToken(),token,&changedProfile);r.inject(0,changed);
 CHECK(admission::sameProfile(r.s[0].pendingPeer()->profile,pending));
 for(unsigned i=0;i<14;++i){
  auto bad=pending;
  switch(i){case 0:bad.cls=4;break;case 1:bad.level=0;break;case 2:bad.race=4;break;case 3:bad.shirt=8;break;case 4:bad.trousers=8;break;
   case 5:bad.attack=1025;break;case 6:bad.hp=0;break;case 7:bad.hp=bad.maxhp+1;break;case 8:bad.attacks=5;break;
   case 9:bad.guard=1;break;case 10:bad.owner=0;break;case 11:bad.kind=coop::Kind::Boss;break;case 12:bad.fatiguePenalty=3;break;case 13:bad.criticalChance=101;break;}
  auto f=admissionFrame(mac[1],admission::Type::Join,r.s[0].sessionToken(),token,&bad);CHECK(!r.s[0].receive(f.source,f.bytes,f.size,r.now));
 }
 auto f=r.lastJoin;for(size_t n=0;n<f.size;++n)CHECK(!r.s[0].receive(f.source,f.bytes,n,r.now));
 CHECK(!r.s[0].receive(f.source,f.bytes,sizeof(f.bytes),r.now));
 f.bytes[7]=1;CHECK(!r.s[0].receive(f.source,f.bytes,f.size,r.now));
 CHECK(r.s[0].joinToken()==token&&r.s[0].state()==SessionState::AwaitingAccept);
 CHECK(r.s[0].accept(r.now));r.flush();r.equal();
 auto old=r.lastStart;old.source=mac[2];CHECK(!r.s[1].receive(old.source,old.bytes,old.size,r.now));
 auto stale=admissionFrame(mac[0],admission::Type::Start,r.s[0].sessionToken(),token+1,&r.s[0].localProfile(),123);
 CHECK(!r.s[1].receive(stale.source,stale.bytes,stale.size,r.now));
 club::Packet pvp{};pvp.magic=club::MAGIC;pvp.version=club::VERSION;
 CHECK(sizeof(pvp)==52&&club::VERSION==5);
 CHECK(!r.s[0].receive(mac[1],reinterpret_cast<const uint8_t*>(&pvp),sizeof(pvp),r.now));
 r.equal();std::puts("PASS: wrong MAC/token, collisions, altered duplicate profile, malformed lengths/fields, PvP v5 isolation");
}
static void declineTimeoutAndRestart(){
 Rig r;r.request();auto oldJoin=r.lastJoin;uint64_t oldHost=r.s[0].sessionToken();
 CHECK(r.s[0].decline(r.now));r.flush();CHECK(r.s[0].state()==SessionState::Hosting);
 CHECK(r.s[0].sessionToken()!=oldHost&&r.s[1].state()==SessionState::Ended&&r.s[1].notice()==SessionNotice::Declined);
 r.inject(0,oldJoin);CHECK(r.s[0].state()==SessionState::Hosting);
 CHECK(r.s[1].open(r.game[1],mac[1],457,r.now));r.flush();CHECK(r.s[1].join(0,r.now));r.flush();
 CHECK(r.s[0].accept(r.now));r.flush();auto oldStart=r.lastStart;auto oldEngine=r.lastEngine;
 r.s[1].leave(r.now);r.flush();CHECK(r.s[0].endReason()==SessionEnd::PeerLeft&&r.s[1].endReason()==SessionEnd::Left);
 r.inject(1,oldStart);r.inject(0,oldJoin);r.inject(1,oldEngine);r.advance(1000);
 CHECK(r.s[0].state()==SessionState::Ended&&r.s[1].state()==SessionState::Ended);
 CHECK(r.s[0].open(r.game[0],mac[0],456,r.now));CHECK(r.s[1].open(r.game[1],mac[1],457,r.now));r.flush();
 r.link();CHECK(r.s[0].sessionToken()!=oldHost);r.inject(1,oldStart);r.flush();r.equal();
 // Replayed JOIN cannot extend the host's deliberate decision window.
 Rig timeout;timeout.request();auto request=timeout.lastJoin;
 for(unsigned i=0;i<48;++i){timeout.advance(250);timeout.inject(0,request);}
 CHECK(timeout.s[0].state()==SessionState::Hosting&&timeout.s[1].state()==SessionState::Ended);
 CHECK(timeout.s[0].sessionToken()!=oldHost);
 std::puts("PASS: decline rotates room token, leave/stale reentry blocked, fresh explicit open, absolute admission deadline");
}
static void reconnectAndIrrecoverableLoss(){
 Rig r;r.link();auto saved=packed(r.game[0]);uint32_t campaignRng=r.game[0].randomState;uint64_t token=r.s[0].sessionToken();
 r.partition=true;for(unsigned i=0;i<12;++i)r.advance(250);
 CHECK(r.s[0].training().endpoint.paused&&r.s[1].training().endpoint.paused);
 CHECK(r.s[0].state()==SessionState::Playing&&r.s[1].state()==SessionState::Playing);
 r.partition=false;for(unsigned i=0;i<4;++i)r.advance(250);
 CHECK(!r.s[0].training().endpoint.paused&&!r.s[1].training().endpoint.paused);CHECK(r.s[0].sessionToken()==token);r.equal();
 auto oldStart=r.lastStart;auto oldEngine=r.lastEngine;
 r.partition=true;coop::Packet wrong{};auto w=coop::begin(wrong,coop::Type::Heartbeat,1,token+1,0);w.u32(0);CHECK(coop::finish(wrong,w));
 for(unsigned i=0;i<41;++i){r.advance(250);r.s[0].receive(mac[1],wrong.bytes,wrong.size,r.now);}
 CHECK(r.s[0].endReason()==SessionEnd::LinkLost&&r.s[1].endReason()==SessionEnd::LinkLost);
 r.partition=false;r.inject(1,oldStart);r.inject(0,oldEngine);r.advance(1000);
 CHECK(r.s[0].state()==SessionState::Ended&&r.s[1].state()==SessionState::Ended);
 CHECK(saved==packed(r.game[0])&&campaignRng==r.game[0].randomState);
 // Expiry is checked on receive too, before a late valid packet can revive it.
 Rig late;late.link();auto f=late.lastEngine;late.now+=Session::kLinkTimeoutMs;
 late.inject(0,f);CHECK(late.s[0].endReason()==SessionEnd::LinkLost);
 std::puts("PASS: short same-host reconnection repairs; 10-second loss, late data and wrong-nonce traffic never revive");
}
static void rejectedEventLivenessAndAckPartition(){
 for(unsigned scenario=0;scenario<2;++scenario){
  Rig r;r.link();r.partition=true;
  auto state=r.s[0].training().endpoint.state;
  if(!scenario)--state.actors[0].hp;else state.revision+=2;
  coop::Packet packet{};auto w=coop::begin(packet,coop::Type::Event,0,r.s[0].sessionToken(),state.revision);
  coop::Effect effect{};effect.peer=0;effect.actor=1;effect.target=10;effect.beforeHash=123;
  coop::writeEffect(w,effect);coop::writeState(w,state);CHECK(coop::finish(packet,w));
  for(unsigned i=0;i<20;++i){r.advance(500);r.s[1].receive(mac[0],packet.bytes,packet.size,r.now);}
  CHECK(r.s[1].state()==SessionState::Ended&&r.s[1].endReason()==SessionEnd::LinkLost);
 }
 Rig ack;ack.link();ack.engineDrops[unsigned(coop::Type::Ack)]=1000;
 auto& host=ack.s[0].training().endpoint;
 CHECK(host.submit(coop::Action::Attack,coop::firstEnemy(host.state),ack.now)==coop::Error::None);
 for(unsigned i=0;i<44;++i)ack.advance(250);
 CHECK(ack.s[0].state()==SessionState::Ended&&ack.s[0].endReason()==SessionEnd::LinkLost);
 std::puts("PASS: conflicting/stale Events do not renew liveness; permanent ACK-only partition ends within bounded recovery");
}
static void enemyPresentationCadence(){
 Rig r;r.link();auto before0=packed(r.game[0]),before1=packed(r.game[1]);
 auto& host=r.s[0].training().endpoint;auto& guest=r.s[1].training().endpoint;
 CHECK(host.submit(coop::Action::Guard,0,r.now)==coop::Error::None);r.advance(0);
 CHECK(guest.canPlay());CHECK(guest.submit(coop::Action::Guard,1,r.now)==coop::Error::None);r.advance(0);
 CHECK(host.state.turn==coop::kPlayerCapacity&&!host.awaitingAck);r.equal();
 for(unsigned enemy=0;enemy<coop::kEnemies;++enemy){
  CHECK(host.state.actors[host.state.turn].owner==coop::kNoPeer);
  const auto hash=coop::stateHash(host.state);auto expected=host.state;coop::Effect effect{};
  CHECK(coop::apply(expected,coop::kNoPeer,coop::enemyCommand(expected),effect,true)==coop::Error::None);
  const unsigned beatCount=r.heartbeats;
  r.advance(Session::kEnemyPresentationMs-1);
  CHECK(coop::stateHash(host.state)==hash&&r.heartbeats>beatCount);
  if(!enemy)r.engineDrops[unsigned(coop::Type::Ack)]=1;
  r.advance(1);CHECK(coop::stateHash(host.state)==coop::stateHash(expected));r.equal();
  if(!enemy){
   CHECK(host.awaitingAck);unsigned retries=host.retries;
   r.advance(coop::kRetryMs-1);CHECK(host.awaitingAck&&host.retries==retries);
   r.advance(1);CHECK(!host.awaitingAck&&host.retries>retries);r.equal();
  }
 }
 // The UI pacing gate applies only to automatic enemies. It neither delays a
 // legal player command nor changes damage, RNG, sequence, or hash semantics.
 CHECK(host.state.turn==0&&host.canPlay());const auto revision=host.state.revision;
 CHECK(host.submit(coop::Action::Guard,0,r.now)==coop::Error::None);CHECK(host.state.revision==revision+1);
 r.advance(0);CHECK(guest.canPlay());CHECK(guest.submit(coop::Action::Guard,1,r.now)==coop::Error::None);r.advance(0);
 auto hash=coop::stateHash(host.state);r.advance(Session::kEnemyPresentationMs-1);CHECK(coop::stateHash(host.state)==hash);
 CHECK(packed(r.game[0])==before0&&packed(r.game[1])==before1);
 // A real admitted low-HP guest can fall, leaving a local host command to
 // transition directly to an enemy. Its external submit must start a new hold.
 Rig lone;lone.s[0].leave(lone.now);lone.s[1].leave(lone.now);lone.frames.clear();
 lone.game[1]=hero(2,1);lone.game[1].p.hp=1;
 CHECK(lone.s[0].open(lone.game[0],mac[0],456,lone.now));CHECK(lone.s[1].open(lone.game[1],mac[1],457,lone.now));lone.flush();lone.link();
 auto& sole=lone.s[0].training().endpoint;auto& fragile=lone.s[1].training().endpoint;
 CHECK(sole.submit(coop::Action::Guard,0,lone.now)==coop::Error::None);lone.advance(0);
 CHECK(fragile.submit(coop::Action::Guard,1,lone.now)==coop::Error::None);lone.advance(0);
 for(unsigned i=0;i<coop::kEnemies;++i)lone.advance(Session::kEnemyPresentationMs);
 CHECK(sole.state.turn==0&&sole.state.actors[1].hp==0);lone.advance(1000);
 auto previous=sole.state.revision;CHECK(sole.submit(coop::Action::Guard,0,lone.now)==coop::Error::None);
 lone.advance(0);CHECK(sole.state.revision==previous+1&&sole.state.actors[sole.state.turn].owner==coop::kNoPeer);
 // Repeated same-state snapshots/ACKs must not keep extending this new hold.
 auto request=coop::Packet{};auto writer=coop::begin(request,coop::Type::SnapshotRequest,1,lone.s[0].sessionToken(),sole.state.revision);
 CHECK(coop::finish(request,writer));
 for(unsigned i=0;i<6;++i){lone.advance(100);lone.s[0].receive(mac[1],request.bytes,request.size,lone.now);lone.flush();CHECK(sole.state.revision==previous+1);}
 lone.advance(100);CHECK(sole.state.revision==previous+2);
 std::puts("PASS: all enemy turns hold700ms after confirmation; ACK retries/heartbeat and player commands remain immediate; state/hash/RNG unchanged");
}
static void fullTraining(){
 Rig r;auto before0=packed(r.game[0]),before1=packed(r.game[1]);uint32_t rng[2]={r.game[0].randomState,r.game[1].randomState};r.link();
 uint8_t waves=0;unsigned actions=0;
 while(r.s[0].state()==SessionState::Playing){
  auto& state=r.s[0].training().endpoint.state;waves|=uint8_t(1u<<state.wave);
  if(state.phase==coop::Phase::Playing&&state.actors[state.turn].owner<2){
   auto& endpoint=r.s[state.actors[state.turn].owner].training().endpoint;
   if(endpoint.canPlay()){
    if(actions==2)r.engineDrops[unsigned(coop::Type::Command)]=2;
    if(actions==4)r.engineDrops[unsigned(coop::Type::Ack)]=2;
    CHECK(endpoint.submit(coop::Action::Attack,coop::firstEnemy(endpoint.state),r.now)==coop::Error::None);++actions;
   }
  }
  r.advance(250);CHECK(actions<300);
 }
 CHECK(r.s[0].endReason()==SessionEnd::Won&&r.s[1].endReason()==SessionEnd::Won&&waves==7);
 r.equal();CHECK(r.s[0].training().active()&&r.s[1].training().active());
 auto finalHash=coop::stateHash(r.s[0].training().endpoint.state);
 auto start=r.lastStart;r.inject(1,start);for(unsigned i=0;i<60;++i)r.advance(250);
 CHECK(r.s[0].state()==SessionState::Ended&&r.s[1].state()==SessionState::Ended);
 CHECK(coop::stateHash(r.s[0].training().endpoint.state)==finalHash);
 CHECK(packed(r.game[0])==before0&&packed(r.game[1])==before1&&r.game[0].randomState==rng[0]&&r.game[1].randomState==rng[1]);
 r.s[0].leave(r.now);CHECK(!r.s[0].training().active());
 std::puts("PASS: two complete waves + boss, final replay/freeze, identical Save30 bytes and campaign RNG");
}
static void eligibilityAndWrap(){
 Session s;auto game=hero(0,5);auto copy=game;copy.phase=rpg::Phase::Hero;CHECK(!s.open(copy,mac[0],4,0));
 copy=game;copy.rageTurns=1;CHECK(!s.open(copy,mac[0],4,0));CHECK(!s.open(game,mac[0],0,0));CHECK(!s.open(game,Mac{},1,0));
 Rig wrap;wrap.now=UINT32_MAX-1000;wrap.s[0].leave(wrap.now);wrap.s[1].leave(wrap.now);wrap.frames.clear();
 CHECK(wrap.s[0].open(wrap.game[0],mac[0],456,wrap.now));CHECK(wrap.s[1].open(wrap.game[1],mac[1],457,wrap.now));wrap.flush();wrap.link();
 for(unsigned i=0;i<30;++i)wrap.advance(500);
 CHECK(wrap.s[0].state()==SessionState::Playing&&wrap.s[1].state()==SessionState::Playing);
 wrap.partition=true;wrap.advance(Session::kLinkTimeoutMs);CHECK(wrap.s[0].endReason()==SessionEnd::LinkLost);
 std::puts("PASS: invalid/active campaign gating and uint32 clock wrap");
}
int main(){
 static_assert(admission::kMagic!=coop::kMagic&&admission::kMagic!=club::MAGIC,"wire namespace collision");
 unsigned initialAllocations=allocations;
 basicAndRetry();discoveryJoinRetries();validationAndCollisions();declineTimeoutAndRestart();reconnectAndIrrecoverableLoss();rejectedEventLivenessAndAckPartition();enemyPresentationCadence();fullTraining();eligibilityAndWrap();
 CHECK(allocations==initialAllocations);std::printf("PASS: %u checks; zero dynamic allocations; sizeof(Session)=%zu\n",checks,sizeof(Session));
}
