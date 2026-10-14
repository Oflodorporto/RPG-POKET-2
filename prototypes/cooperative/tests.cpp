#include "CoopEndpoint.h"
#include "../../firmware/RPG_POKET_2/Persistence.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
using namespace coop;
static size_t allocations=0;static unsigned checks=0,scenarios=0;
void* operator new(std::size_t n){++allocations;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort();}}while(0)
static rpg::Game hero(uint8_t cls,uint8_t level,uint32_t seed){auto g=rpg::create(cls,seed);for(uint8_t l=1;l<level;++l){g.p.xp=rpg::xpNeeded(g);rpg::levelUp(g);}g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.race=cls;g.shirt=cls+2;g.trousers=cls+3;return g;}
static State initial(uint32_t seed=13){State s{};auto a=hero(0,5,111),b=hero(2,7,222);CHECK(initialize(s,a,b,seed));CHECK(validState(s));return s;}
static void clean(Endpoint&x){Packet p{};while(x.outgoing.pop(p)){};UiEvent u{};while(x.ui.pop(u)){};}
static void relay(Endpoint&a,Endpoint&b,uint32_t now){Packet p{};while(a.outgoing.pop(p))b.receive(a.local,p,now);}
static void sync(Endpoint&h,Endpoint&c,uint32_t now=0){for(unsigned i=0;i<4;++i){relay(h,c,now);relay(c,h,now);}CHECK(h.hasState&&c.hasState&&!h.paused&&!c.paused&&!h.awaitingAck);CHECK(stateHash(h.state)==stateHash(c.state));}
static void setup(Endpoint&h,Endpoint&c,uint32_t now=0){h.hostStart(initial(),0x778899AABBCCDDull,now);c.clientStart(h.nonce,now);sync(h,c,now);}
static Packet commandPacket(uint64_t nonce,const Command&c){Packet p{};auto w=begin(p,Type::Command,1,nonce,c.sequence);writeCommand(w,c);CHECK(finish(p,w));return p;}
static void rehash(Packet&p){uint32_t hash=digest(p.bytes,p.size-4);for(unsigned i=0;i<4;++i)p.bytes[p.size-4+i]=uint8_t(hash>>(8*i));}
static void testsModel(){
 State s=initial();CHECK(s.actors[0].level==5&&s.actors[1].level==7&&s.actors[2].kind==Kind::Empty);
 uint8_t expected[]={0,1,3,4,5};for(uint8_t slot:expected){CHECK(s.turn==slot);Command c{};c.sequence=slot<2?s.accepted[slot]+1:0;c.revision=s.revision;c.actor=slot;c.target=slot<2?slot:0;c.action=slot<2?Action::Guard:Action::Attack;Effect e{};CHECK(apply(s,slot<2?slot:kNoPeer,c,e,slot>=3)==Error::None);CHECK(validState(s));}CHECK(s.turn==0&&s.round==2);
 s=initial();Command c{1,0,0,3,Action::Attack};Effect e{};uint32_t hash=stateHash(s);
 c.actor=1;CHECK(apply(s,1,c,e)==Error::Turn);c.actor=0;c.target=1;CHECK(apply(s,0,c,e)==Error::Target);c.target=255;CHECK(apply(s,0,c,e)==Error::Invalid);c.target=3;c.revision=1;CHECK(apply(s,0,c,e)==Error::Revision);CHECK(stateHash(s)==hash);
 c.revision=0;c.target=0;c.action=Action::Life;CHECK(apply(s,0,c,e)==Error::Resource);c.action=Action::Mana;CHECK(apply(s,0,c,e)==Error::Resource);
 c.action=Action::Life;s.actors[0].hp-=4;uint8_t count=s.actors[0].life;CHECK(apply(s,0,c,e)==Error::None&&e.amount==4&&s.actors[0].life==count-1);
 s=initial();s.actors[0].mp=0;c.action=Action::Focus;c.target=3;CHECK(apply(s,0,c,e)==Error::Resource);
 s=initial();s.actors[3].hp=0;c.action=Action::Attack;CHECK(apply(s,0,c,e)==Error::Target);
 s=initial();s.revision=UINT32_MAX;c.revision=s.revision;CHECK(apply(s,0,c,e)==Error::Limit);s=initial();s.round=UINT16_MAX;c.revision=0;CHECK(apply(s,0,c,e)==Error::Limit);
 // Freeze original campaign/save bytes and RNG: adapter takes const references only.
 for(uint8_t cls=0;cls<4;++cls)for(uint8_t lv:{uint8_t(1),uint8_t(3),uint8_t(5),uint8_t(11),uint8_t(20)}){
  auto a=hero(cls,lv,100+cls),b=hero((cls+1)%4,3,22);uint8_t before[160],after[160];rpg::encodePersistent(a,88,0,before);uint32_t rng=a.randomState;
  CHECK(initialize(s,a,b,777));CHECK(s.actors[0].level==lv&&s.actors[0].hp==a.p.hp&&s.actors[0].attack==rpg::effectiveAttack(a));
  c={1,0,0,3,Action::Attack};CHECK(apply(s,0,c,e)==Error::None);rpg::encodePersistent(a,88,0,after);CHECK(std::memcmp(before,after,160)==0&&a.randomState==rng);
 }
 auto legacy=hero(0,1,5);legacy.dndProgression=false;legacy.p.level=99;auto b=hero(1,3,6);CHECK(initialize(s,legacy,b,5)&&s.actors[0].level==99&&validState(s));
 for(uint8_t bad:{uint8_t(0),uint8_t(100),uint8_t(255)}){legacy.p.level=bad;CHECK(!initialize(s,legacy,b,5));}
 auto invalid=hero(1,3,7);invalid.p.cls=255;CHECK(!initialize(s,invalid,b,4));invalid=hero(1,3,7);invalid.equipped[0]=255;CHECK(!initialize(s,invalid,b,4));
 ++scenarios;
}
static void testsWire(){
 State s=initial();Packet p{};auto w=begin(p,Type::Snapshot,0,42,s.revision);writeState(w,s);CHECK(finish(p,w)&&p.size==211);Header h{};CHECK(decode(p,h));auto r=payloadReader(p,h);State copy=readState(r);CHECK(r.ok&&r.n==kStateBytes&&validState(copy)&&stateHash(copy)==stateHash(s));
 for(size_t n=0;n<p.size;++n){Packet truncated=p;truncated.size=uint16_t(n);CHECK(!decode(truncated,h));}
 Packet large=p;large.size=241;CHECK(!decode(large,h));large.size=65535;CHECK(!decode(large,h));
 for(unsigned i=0;i<p.size;++i){Packet altered=p;altered.bytes[i]^=0x80;CHECK(!decode(altered,h));}
 Endpoint host,client;setup(host,client);uint32_t hash=stateHash(client.state),rng=client.state.rng;
 // Every framing discriminator also has a canonical-checksum negative control.
 for(unsigned offset:{0u,4u,5u,6u,7u,8u,20u}){Packet altered=p;altered.bytes[offset]^=0x80;rehash(altered);client.receive(0,altered,1);CHECK(stateHash(client.state)==hash&&client.state.rng==rng);}
 // Matching session, invalid state hp>max rejected atomically.
 Packet bad{};auto bw=begin(bad,Type::Snapshot,0,host.nonce,0);State invalid=host.state;invalid.actors[0].hp=uint16_t(invalid.actors[0].maxhp+1);writeState(bw,invalid);finish(bad,bw);client.receive(0,bad,1);CHECK(stateHash(client.state)==hash);
 auto cw=begin(bad,Type::Snapshot,0,host.nonce,0);writeState(cw,host.state);finish(bad,cw);client.receive(1,bad,2);CHECK(stateHash(client.state)==hash);client.receive(0,bad,3);CHECK(stateHash(client.state)==hash); // valid control
 CHECK(kMagic!=0x50434C31&&kWireVersion!=5);++scenarios;
}
static void testsCommands(){
 Endpoint h,c;setup(h,c);CHECK(h.submit(Action::Guard,0,10)==Error::None);sync(h,c,10);clean(h);clean(c);
 Command cmd{1,c.state.revision,1,3,Action::Attack};Packet p=commandPacket(h.nonce,cmd);h.receive(1,p,20);CHECK(h.state.accepted[1]==1&&h.state.revision==2);uint32_t after=stateHash(h.state),rng=h.state.rng;
 h.receive(1,p,21);CHECK(stateHash(h.state)==after&&h.state.rng==rng);Command changed=cmd;changed.target=4;h.receive(1,commandPacket(h.nonce,changed),22);CHECK(h.lastError==Error::Conflict&&stateHash(h.state)==after);
 sync(h,c,25);clean(h);clean(c);
 Command future{99,h.state.revision,1,3,Action::Attack};h.receive(1,commandPacket(h.nonce,future),30);CHECK(h.lastError==Error::Sequence&&stateHash(h.state)==after);
 future.sequence=2;future.revision=0;h.receive(1,commandPacket(h.nonce,future),31);CHECK(h.lastError==Error::Revision&&stateHash(h.state)==after);
 future.revision=h.state.revision;future.actor=255;h.receive(1,commandPacket(h.nonce,future),32);CHECK(h.lastError==Error::Invalid&&stateHash(h.state)==after);
 future.actor=1;h.receive(1,commandPacket(h.nonce,future),33);CHECK(h.lastError==Error::Turn&&stateHash(h.state)==after);
 // Duplicate confirmed event does not replay UI or advance RNG.
 setup(h,c);clean(h);clean(c);CHECK(h.submit(Action::Guard,0,10)==Error::None);Packet event{};CHECK(h.outgoing.pop(event));c.receive(0,event,10);unsigned visual=c.eventsApplied;uint32_t digest1=stateHash(c.state);c.receive(0,event,11);CHECK(c.eventsApplied==visual&&c.duplicateEvents==1&&stateHash(c.state)==digest1);
 // Future event with complete post-state still requests reconciliation, never speculates.
 State next=c.state;Command cc{1,next.revision,1,3,Action::Attack};Effect fx{};fx.beforeHash=0;CHECK(apply(next,1,cc,fx)==Error::None);Packet second{};auto w=begin(second,Type::Event,0,h.nonce,next.revision);writeEffect(w,fx);writeState(w,next);finish(second,w);
 Endpoint lagging;lagging.clientStart(h.nonce,0);Packet snap{};auto sw=begin(snap,Type::Snapshot,0,h.nonce,0);State start=initial();writeState(sw,start);finish(snap,sw);lagging.receive(0,snap,0);lagging.receive(0,second,20);CHECK(lagging.paused&&lagging.state.revision==0);
 ++scenarios;
}
static void testsDisconnect(){
 Endpoint h,c;setup(h,c);clean(h);clean(c);uint32_t before=stateHash(h.state);
 for(uint32_t t=100;t<=10000;t+=100){h.tick(t);c.tick(t);clean(h);clean(c);}CHECK(h.paused&&c.paused&&stateHash(h.state)==before&&h.state.phase==Phase::Playing&&c.state.phase==Phase::Playing&&!h.receipt.valid&&!c.receipt.valid);
 CHECK(h.submit(Action::Attack,3,10001)==Error::Paused); // no AI or local progress while partitioned
 for(uint32_t t=10020;t<12000;t+=20){h.tick(t);c.tick(t);relay(h,c,t);relay(c,h,t);}sync(h,c,12000);
 // Same host client reboot: only a fresh authoritative snapshot resumes; no local full heal.
 h.state.actors[1].hp-=3;c.clientStart(h.nonce,12010);relay(c,h,12010);sync(h,c,12010);CHECK(c.state.actors[1].hp==h.state.actors[1].hp);
 // A host reboot with new nonce cannot be mistaken for the old game.
 Endpoint replacement;replacement.hostStart(initial(),h.nonce+1,12020);relay(replacement,c,12020);CHECK(c.nonce==h.nonce&&c.state.actors[1].hp==h.state.actors[1].hp);
 // Millisecond wraparound elapsed-time arithmetic.
 setup(h,c,UINT32_MAX-600);clean(h);clean(c);h.tick(1400);c.tick(1400);CHECK(h.paused&&c.paused);++scenarios;
}
struct Flight{bool used=false;uint32_t due=0;uint8_t source=0;Packet packet{};};
struct Network{
 Flight in[128]{};uint32_t rng=1,dropped=0,duplicated=0,peak=0;bool chaos=false;
 uint32_t roll(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
 void enqueue(uint8_t source,const Packet&p,uint32_t now){if(chaos&&roll()%100<25){++dropped;return;}unsigned count=chaos&&roll()%100<20?2:1;if(count==2)++duplicated;for(unsigned n=0;n<count;++n){bool found=false;for(auto&f:in)if(!f.used){f={true,now+(chaos?roll()%180:0),source,p};found=true;break;}CHECK(found);}uint32_t used=0;for(auto&f:in)used+=f.used;peak=std::max(peak,used);}
 void run(Endpoint&h,Endpoint&c,uint32_t now){Packet p{};while(h.outgoing.pop(p))enqueue(0,p,now);while(c.outgoing.pop(p))enqueue(1,p,now);for(auto&f:in)if(f.used&&f.due<=now){Packet copy=f.packet;uint8_t source=f.source;f.used=false;(source==0?c:h).receive(source,copy,now);}}
};
static Action choose(const Endpoint&e){const Actor&a=e.state.actors[e.local];if(e.state.round==1)return Action::Guard;if(a.hp<a.maxhp/2&&a.life)return Action::Life;return a.mp>=2?Action::Focus:Action::Attack;}
static void takeTurn(Endpoint&e,uint32_t now){if(!e.canPlay())return;Action action=choose(e);uint8_t target=(action==Action::Guard||action==Action::Life||action==Action::Mana)?e.local:firstEnemy(e.state);CHECK(e.submit(action,target,now)==Error::None);}
struct RunResult{uint32_t hash=0,revision=0,dropped=0,duplicated=0;Phase phase=Phase::Playing;};
static RunResult simulate(uint32_t seed,bool chaotic,FILE*trace=nullptr,uint8_t clsA=0,uint8_t clsB=2){
 auto a=hero(clsA,5,111),b=hero(clsB,7,222);uint8_t saveA[160],saveB[160],after[160];rpg::encodePersistent(a,8,0,saveA);rpg::encodePersistent(b,9,1,saveB);
 State start{};CHECK(initialize(start,a,b,seed));Endpoint h,c;h.hostStart(start,0xBEEF00000000ull+seed,0);c.clientStart(h.nonce,0);Network net{};net.rng=seed;net.chaos=chaotic;bool ended=false;uint32_t lastRevision=0;
 for(uint32_t now=0;now<120000;now+=20){net.chaos=chaotic&&now<30000;h.tick(now);c.tick(now);net.run(h,c,now);takeTurn(h,now);takeTurn(c,now);CHECK(validState(h.state));if(c.hasState)CHECK(validState(c.state));UiEvent event{};
  while(h.ui.pop(event)){CHECK(event.revision==lastRevision+1);lastRevision=event.revision;if(trace){std::fprintf(trace,"{\"revision\":%u,\"actor_id\":%u,\"target_id\":%u,\"action\":%u,\"amount\":%u,\"critical\":%s,\"from_wave\":%u,\"wave\":%u,\"round\":%u,\"next_actor_id\":%u,\"phase\":%u,\"state_hash\":%u,\"actors\":[",event.revision,event.effect.actor,event.effect.target,unsigned(event.effect.action),event.effect.amount,event.effect.critical?"true":"false",event.effect.fromWave+1,event.wave+1,event.round,event.turn,unsigned(event.phase),event.hash);for(unsigned i=0;i<kActors;++i){const Actor&x=h.state.actors[i];std::fprintf(trace,"%s{\"slot\":%u,\"id\":%u,\"kind\":%u,\"species\":%u,\"class\":%u,\"level\":%u,\"hp\":%u,\"maxhp\":%u,\"mp\":%u,\"maxmp\":%u,\"race\":%u,\"shirt\":%u,\"trousers\":%u,\"guard\":%u}",i?",":"",i,x.id,unsigned(x.kind),x.species,x.cls,x.level,x.hp,x.maxhp,x.mp,x.maxmp,x.race,x.shirt,x.trousers,x.guard);}std::fputs("]}\n",trace);}}
  while(c.ui.pop(event)){};
  if(h.state.phase!=Phase::Playing&&c.hasState&&stateHash(h.state)==stateHash(c.state)&&!h.awaitingAck&&!c.pending&&!h.paused&&!c.paused){ended=true;break;}
 }
 CHECK(ended);CHECK(h.receipt.valid&&c.receipt.valid&&h.receipt.stateHash==c.receipt.stateHash);CHECK(h.outgoing.overflow==0&&c.outgoing.overflow==0&&h.ui.overflow==0&&c.ui.overflow==0);
 rpg::encodePersistent(a,8,0,after);CHECK(std::memcmp(saveA,after,160)==0);rpg::encodePersistent(b,9,1,after);CHECK(std::memcmp(saveB,after,160)==0);
 ++scenarios;return {stateHash(h.state),h.state.revision,net.dropped,net.duplicated,h.state.phase};
}
static void testTerminalLossAndQueue(){
 State s=initial();for(unsigned i=0;i<2;++i)s.actors[i].hp=1;s.turn=3;s.rng=777;
 Endpoint h,c;h.hostStart(s,33,0);c.clientStart(33,0);sync(h,c);for(uint32_t t=1;t<10000&&h.state.phase==Phase::Playing;++t){h.tick(t);relay(h,c,t);relay(c,h,t);}sync(h,c,10000);CHECK(h.state.phase==Phase::Lost&&h.receipt.valid&&c.receipt.valid);
 uint32_t terminal=stateHash(h.state);for(uint32_t t=10100;t<15000;t+=100){h.tick(t);c.tick(t);clean(h);clean(c);}CHECK(stateHash(h.state)==terminal&&h.state.phase==Phase::Lost); // timeout preserves known result
 Queue<Packet,8> q{};Packet p{};for(unsigned i=0;i<8;++i)CHECK(q.push(p));CHECK(!q.push(p)&&q.count==8&&q.overflow==1);for(unsigned i=0;i<8;++i)CHECK(q.pop(p));CHECK(!q.pop(p));
 ++scenarios;
}
static void testLossAndBackpressure(){
 Endpoint h,c;setup(h,c);clean(h);clean(c);Packet junk{};
 for(unsigned i=0;i<8;++i)CHECK(h.outgoing.push(junk));
 CHECK(h.submit(Action::Guard,0,10)==Error::None);CHECK(h.outgoing.overflow==1&&h.awaitingAck);
 clean(h);h.tick(210);relay(h,c,210);relay(c,h,210);CHECK(c.state.revision==1&&stateHash(h.state)==stateHash(c.state)&&!h.awaitingAck);
 // Explicit selective terminal-ACK loss: replays cannot create a second result.
 State s=initial(5);s.wave=2;spawnWave(s);s.actors[3].hp=1;
 h.hostStart(s,9999,0);c.clientStart(9999,0);sync(h,c);clean(h);clean(c);
 CHECK(h.submit(Action::Attack,3,10)==Error::None);CHECK(h.state.phase==Phase::Won);relay(h,c,10);clean(c);
 CHECK(c.receipt.valid&&!h.receipt.valid&&h.awaitingAck);uint32_t terminal=stateHash(c.state);unsigned displays=c.eventsApplied;
 for(uint32_t now=210;now<1210;now+=200){h.tick(now);relay(h,c,now);clean(c);CHECK(stateHash(c.state)==terminal&&c.eventsApplied==displays);}
 h.tick(1410);relay(h,c,1410);relay(c,h,1410);CHECK(h.receipt.valid&&c.receipt.valid&&h.receipt.stateHash==terminal);
 // Wrong digest and old revision ACKs never open the host's next turn.
 setup(h,c);clean(h);clean(c);CHECK(h.submit(Action::Guard,0,1)==Error::None);Packet ack{};auto w=begin(ack,Type::Ack,1,h.nonce,h.state.revision);w.u32(stateHash(h.state)^1);finish(ack,w);h.receive(1,ack,2);CHECK(h.awaitingAck);
 w=begin(ack,Type::Ack,1,h.nonce,0);w.u32(stateHash(h.state));finish(ack,w);h.receive(1,ack,3);CHECK(h.awaitingAck);
 // Visual backpressure discards only animations, never a command or confirmation.
 setup(h,c);for(uint32_t now=1;now<30000&&h.state.phase==Phase::Playing;++now){h.tick(now);c.tick(now);relay(h,c,now);relay(c,h,now);takeTurn(h,now);takeTurn(c,now);}sync(h,c,30000);CHECK(h.state.phase!=Phase::Playing&&h.ui.overflow>0&&c.ui.overflow>0&&h.receipt.valid&&c.receipt.valid);
 ++scenarios;
}
int main(int argc,char**argv){
 size_t before=allocations;testsModel();testsWire();testsCommands();testsDisconnect();testTerminalLossAndQueue();testLossAndBackpressure();
 auto base=simulate(123,false);auto duplicate=simulate(123,false);CHECK(base.hash==duplicate.hash&&base.revision==duplicate.revision&&base.phase==Phase::Won);
 uint32_t drops=0,dupes=0;for(uint32_t seed=1;seed<=96;++seed){auto chaos=simulate(seed,true,nullptr,uint8_t(seed%4),uint8_t((seed/4)%4));auto stable=simulate(seed,false,nullptr,uint8_t(seed%4),uint8_t((seed/4)%4));CHECK(chaos.hash==stable.hash&&chaos.revision==stable.revision&&chaos.phase==stable.phase);drops+=chaos.dropped;dupes+=chaos.duplicated;}
 CHECK(allocations==before); // Core + all network simulation execution uses no C++ heap allocations.
 if(argc>1){FILE*f=std::fopen(argv[1],"w");CHECK(f);auto result=simulate(123,false,f);std::fclose(f);std::printf("trace revision=%u hash=%u result=%u\n",result.revision,result.hash,unsigned(result.phase));}
 std::printf("PASS checks=%u scenarios=%u network_seeds=96 losses=%u duplicates=%u cpp_allocations=%zu\n",checks,scenarios,drops,dupes,allocations-before);
 std::printf("memory State=%zu Actor=%zu Endpoint=%zu two_endpoints=%zu Packet=%zu MaxWire=%zu SnapshotWire=%zu EventWire=%zu UiEvent=%zu\n",sizeof(State),sizeof(Actor),sizeof(Endpoint),2*sizeof(Endpoint),sizeof(Packet),kMaxPacket,kHeaderBytes+kStateBytes+4,kHeaderBytes+kStateBytes+16+4,sizeof(UiEvent));
}
