#include "CoopEndpoint.h"
#include <cstdio>
#include <cstring>

namespace {
using namespace coop;
unsigned passed=0, failed=0;
void check(bool condition,const char* label) {
  std::printf("%s %s\n",condition?"PASS":"FAIL",label);
  condition?++passed:++failed;
}
State initial(uint8_t turn=0) {
  State s{};
  const rpg::Game a=rpg::create(0,7), b=rpg::create(2,8);
  if(!initialize(s,a,b,3)) std::abort();
  s.turn=turn;
  return s;
}
Packet snapshot(const State& s,uint64_t nonce=42) {
  Packet p{};auto w=begin(p,Type::Snapshot,0,nonce,s.revision);
  writeState(w,s);if(!finish(p,w))std::abort();return p;
}
Packet command(const Command& c) {
  Packet p{};auto w=begin(p,Type::Command,1,42,c.sequence);
  writeCommand(w,c);if(!finish(p,w))std::abort();return p;
}
Packet event(const State& s,const Effect& e) {
  Packet p{};auto w=begin(p,Type::Event,0,42,s.revision);
  writeEffect(w,e);writeState(w,s);if(!finish(p,w))std::abort();return p;
}
unsigned pump(Endpoint& h,Endpoint& c,uint32_t now,bool dropCommands=false) {
  unsigned commands=0;
  for(unsigned n=0;n<100;++n) {
    bool moved=false;Packet p{};
    while(h.outgoing.pop(p)){c.receive(0,p,now);moved=true;}
    while(c.outgoing.pop(p)) {
      Header header{};if(!decode(p,header))std::abort();
      if(header.type==Type::Command){++commands;if(dropCommands){moved=true;continue;}}
      h.receive(1,p,now);moved=true;
    }
    if(!moved)return commands;
  }
  std::abort();
}
void start(Endpoint& h,Endpoint& c,const State& s) {
  h.hostStart(s,42,0);c.clientStart(42,0);pump(h,c,0);
  if(h.paused||c.paused||h.awaitingAck||stateHash(h.state)!=stateHash(c.state))std::abort();
}
}

int main() {
  using namespace coop;
  {
    State s=initial();Packet p=snapshot(s);Header h{};
    check(validState(s)&&stateHash(s)!=0&&decode(p,h),"valid initial state and snapshot");
    check(p.size==kHeaderBytes+kStateBytes+4,"exact snapshot byte count");
    bool rejectsAll=true;
    for(uint16_t n=0;n<p.size;++n){Packet q=p;q.size=n;Header a{};rejectsAll&=!decode(q,a);}
    check(rejectsAll,"all truncated snapshot lengths rejected");
    rejectsAll=true;
    for(uint16_t n=0;n<p.size;++n)for(unsigned bit=0;bit<8;++bit){Packet q=p;q.bytes[n]^=uint8_t(1u<<bit);Header a{};rejectsAll&=!decode(q,a);}
    check(rejectsAll,"all single-bit snapshot corruptions rejected");
  }
  {
    Endpoint h,c;start(h,c,initial(1));
    check(c.submit(Action::Attack,3,10)==Error::None,"client accepts current-turn intent");
    Packet original{};if(!c.outgoing.pop(original))std::abort();
    h.receive(1,original,10);pump(h,c,10);
    const uint32_t hash=stateHash(h.state),revision=h.state.revision;
    h.receive(1,original,20);pump(h,c,20);
    check(h.state.revision==revision&&stateHash(h.state)==hash&&stateHash(c.state)==hash,"duplicate accepted command never rerolls");
    Command changed{};changed.sequence=1;changed.revision=0;changed.actor=1;changed.target=4;
    h.receive(1,command(changed),21);
    check(h.lastError==Error::Conflict&&stateHash(h.state)==hash,"duplicate sequence with conflicting intent rejected");
    State future=c.state;future.revision+=2;
    Effect e{};e.beforeHash=stateHash(c.state);e.peer=1;e.actor=2;e.target=c.state.actors[3].id;e.fromWave=c.state.wave;
    c.receive(0,event(future,e),30);
    check(c.paused&&stateHash(c.state)==hash,"future event pauses without state mutation");
    const Packet stale=snapshot(initial(1));c.receive(0,stale,31);
    check(stateHash(c.state)==hash,"stale snapshot cannot roll state back");
  }
  {
    Endpoint h,c;start(h,c,initial(1));
    const uint32_t hash=stateHash(h.state);
    h.tick(2000);c.tick(2000);
    check(h.paused&&c.paused&&stateHash(h.state)==hash&&stateHash(c.state)==hash&&!h.receipt.valid&&!c.receipt.valid,"disconnect pauses without combat outcome");
    pump(h,c,2000);
    check(!h.paused&&!c.paused&&stateHash(h.state)==stateHash(c.state),"unchanged state reconnect converges");
  }
  {
    Endpoint h,c;start(h,c,initial(1));
    c.submit(Action::Attack,3,10);unsigned dropped=pump(h,c,10,true);
    for(uint32_t now=210;now<=1810;now+=200){h.tick(now);c.tick(now);dropped+=pump(h,c,now,true);}
    unsigned restored=0;
    for(uint32_t now=2010;now<=5010;now+=200){h.tick(now);c.tick(now);restored+=pump(h,c,now,false);}
    std::printf("INFO retry recovery: dropped=%u laterCommands=%u clientPending=%u revision=%u\n",dropped,restored,c.pending,h.state.revision);
    check(!c.pending,"exhausted command retry can recover or explicitly cancel after resync");
  }
  {
    State impossible=initial();impossible.phase=Phase::Won;
    for(unsigned i=kPlayerCapacity;i<kActors;++i)impossible.actors[i].hp=0;
    check(!validState(impossible),"victory before final boss wave is rejected");
    Endpoint c;c.clientStart(42,0);c.receive(0,snapshot(impossible),10);
    check(!c.receipt.valid,"impossible initial-wave victory cannot mint a terminal receipt");
    State impossibleSequence=initial();impossibleSequence.accepted[1]=UINT32_MAX;
    check(!validState(impossibleSequence),"accepted sequence cannot exceed total revision");
  }
  {
    Endpoint h,c;start(h,c,initial());
    State next=c.state;Effect e{};Command cmd{};cmd.sequence=1;cmd.revision=0;cmd.actor=0;cmd.target=3;
    if(apply(next,0,cmd,e)!=Error::None)std::abort();
    e.beforeHash=stateHash(c.state);
    e.amount=65535;e.actor=2;e.peer=1;
    c.receive(0,event(next,e),10);
    UiEvent u{};bool displayed=false;while(c.ui.pop(u))displayed|=u.effect.amount==65535;
    check(!displayed,"semantically impossible effect is rejected before UI presentation");
  }
  {
    State terminal=initial();terminal.phase=Phase::Lost;terminal.turn=3;
    terminal.actors[0].hp=terminal.actors[1].hp=0;
    Endpoint c;c.clientStart(42,0);c.receive(0,snapshot(terminal),10);
    State later=initial();later.revision=1;c.receive(0,snapshot(later),20);
    check(c.state.phase==Phase::Lost&&c.receipt.valid&&c.receipt.result==Phase::Lost,"terminal result cannot reopen under same session nonce");
  }
  {
    Endpoint c;c.clientStart(42,0);
    Packet q{};uint32_t rnd=0x12345678;
    for(unsigned n=0;n<20000;++n) {
      rnd^=rnd<<13;rnd^=rnd>>17;rnd^=rnd<<5;q.size=uint16_t(rnd%300);
      for(auto& b:q.bytes){rnd^=rnd<<13;rnd^=rnd>>17;rnd^=rnd<<5;b=uint8_t(rnd);}
      c.receive(uint8_t(n%3),q,n);
    }
    check(!c.hasState&&!c.receipt.valid,"20000 random malformed packets cannot establish state");
  }
  std::printf("REVIEW %u passed, %u failed\n",passed,failed);
  return failed?1:0;
}
