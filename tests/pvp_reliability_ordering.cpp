// Independent two-endpoint repro; ordinary attacks reach terminal, no HP edits.
#define main supplied_protocol_main
#include "club_protocol.cpp"
#undef main
#include <vector>
static void link(Network& n){n.discover();assert(n.a.invite(0,100,n.now));n.flush();n.b.accept(n.now);n.flush();n.startCombat();n.advance(0);assert(n.duel[0].ready&&n.duel[1].ready);}
static void deliver(Network& n,const Network::Delivery& d){(d.source==0?n.b:n.a).receive(n.mac[d.source],d.packet,n.now);}
int main(){
  for(bool cancelFirst:{false,true})for(bool onlyReceipt:{false,true}){
    Network n;link(n);int actor=-1;
    while(!n.duel[0].finished){
      actor=n.duel[0].turn;assert(n.duel[actor].play(Action::Attack,n.now));
      if(!n.duel[actor].finished){n.flush();continue;}
      auto finalMove=n.queue.front();n.queue.pop_front();assert(finalMove.packet.type==uint8_t(Type::Move));deliver(n,finalMove);
      break;
    }
    assert(n.duel[actor].finished&&n.duel[actor].waiting&&!n.duel[actor].terminalConfirmed);
    assert(n.duel[1-actor].terminalConfirmed&&n.duel[0].stateDigest()==n.duel[1].stateDigest());
    std::vector<Network::Delivery> proof;
    while(!n.queue.empty()){auto d=n.queue.front();n.queue.pop_front();if(d.packet.type==uint8_t(Type::Result)||(!onlyReceipt&&d.packet.type==uint8_t(Type::MoveAck)))proof.push_back(d);}
    // Honest terminal receiver closes after the existing 11-second radio grace.
    // Sender's loop has been paused during this interval (as in the supplied
    // ACK->Cancel regression), so both orders are in the same received batch.
    n.now+=11000;(actor==0?n.b:n.a).cancel(n.now);
    assert(n.queue.size()==1&&n.queue.front().packet.type==uint8_t(Type::Cancel));auto cancel=n.queue.front();n.queue.pop_front();
    if(cancelFirst)deliver(n,cancel);for(auto d:proof)deliver(n,d);if(!cancelFirst)deliver(n,cancel);
    assert(n.duel[actor].terminalConfirmed);
    std::printf("PASS: natural lethal action, %s, %s -> terminalConfirmed=%d, both combat digests still equal\n",onlyReceipt?"receipt only":"ACK + receipt",cancelFirst?"Cancel then proof":"proof then Cancel",n.duel[actor].terminalConfirmed);
  }
  return 0;
}
