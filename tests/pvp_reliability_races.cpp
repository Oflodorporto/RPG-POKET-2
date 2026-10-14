// Bounded lossy/reordered two-Session stress. No hardware/RF claim.
#define main original_protocol_main
#include "club_protocol.cpp"
#undef main
#include <random>

static void linkRace(Network& n,uint32_t id){
  n.discover();assert(n.a.invite(0,id,n.now));n.flush();n.b.accept(n.now);n.flush();
  n.startCombat();n.advance(0);assert(n.duel[0].ready&&n.duel[1].ready);
}
static void deliver(Network& n,size_t index,bool duplicate){
  auto d=n.queue[index];n.queue.erase(n.queue.begin()+index);unsigned target=1-d.source;
  auto &s=target==0?n.a:n.b;
  s.receive(n.mac[d.source],d.packet,n.now);
  if(duplicate)s.receive(n.mac[d.source],d.packet,n.now);
}
int main(){
  unsigned runs=0,packets=0;
  for(unsigned seed=1;seed<=2000;++seed){
    Network n;linkRace(n,100+(seed&1));std::mt19937 random(seed);
    unsigned mode=seed%5;
    for(unsigned step=0;step<160;++step){
      // Both endpoints remain independently driven. Commands are selected only
      // from locally legal UI actions; surrender can coincide with in-flight
      // ordinary commands, ACKs, duplicate commands or another surrender.
      for(unsigned who=0;who<2;++who){auto &d=n.duel[who];
        if(step==5 && ((mode<2&&who==mode)||mode==2))d.play(Action::Surrender,n.now);
        if(step==9 && mode==3)d.play(Action::Surrender,n.now);
        if(!d.finished&&!d.waiting&&!d.surrenderRequested&&d.turn==who&&random()%3==0){
          Action action=Action(random()%3);if(!d.play(action,n.now))d.play(Action::Attack,n.now);
        }
      }
      // Up to 1.2 seconds of reorder/loss; thereafter fair delivery. No test
      // intentionally violates the documented ten-second command deadline.
      for(unsigned count=0;count<12&&!n.queue.empty();++count){
        size_t index=step<24?random()%n.queue.size():0;
        if(step<24 && random()%7==0)n.queue.erase(n.queue.begin()+index);
        else {deliver(n,index,random()%4==0);++packets;}
      }
      n.now+=50;n.a.tick(n.now);n.b.tick(n.now);n.duel[0].tick(n.now);n.duel[1].tick(n.now);
      if(n.duel[0].terminalConfirmed&&n.duel[1].terminalConfirmed)break;
    }
    n.flush();
    // After temporary loss both sides must have the same confirmed result,
    // with no local predicted-only victory and no divergent terminal state.
    assert(n.duel[0].terminalConfirmed&&n.duel[1].terminalConfirmed);
    assert(!n.duel[0].failed&&!n.duel[1].failed);
    assert(n.duel[0].stateDigest()==n.duel[1].stateDigest());
    assert(n.duel[0].winner==n.duel[1].winner);
    ++runs;
  }
  std::cout<<"PASS: "<<runs<<" lossy/reordered two-Session races; "<<packets<<" packet deliveries with duplicates\n";
}
