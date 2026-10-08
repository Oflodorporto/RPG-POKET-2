#include "TestHero.h"
#include "ClubProtocol.h"
#include "ClubDuel.h"
#include <cassert>
#include <cstring>
#include <deque>
#include <iostream>
using namespace club;

struct Network {
  Session a,b;
  Duel duel[2];
  bool combat=false;
  uint8_t mac[2][6]={{2,0,0,0,0x12,0x34},{2,0,0,0,0x56,0x78}};
  struct Context {Network *network;int source;} contexts[2]={{this,0},{this,1}};
  struct Delivery {int source;uint8_t destination[6];Packet packet;};
  std::deque<Delivery> queue;
  int drops[10]={};
  uint32_t now=0;
  static void send(void *context,const uint8_t *destination,const Packet &p){
    auto &c=*static_cast<Context *>(context);
    if(c.network->drops[p.type]>0){--c.network->drops[p.type];return;}
    Delivery d{c.source,{},p};std::memcpy(d.destination,destination,6);
    c.network->queue.push_back(d);
  }
  Network(){
    a.begin(mac[0],"CAVALEIRO",1,3,send,&contexts[0],now);
    b.begin(mac[1],"MAGO",0,8,send,&contexts[1],now);
  }
  static void moveSend(void *p,uint16_t seq,uint8_t action,bool ack){
    auto &c=*static_cast<Context *>(p);(c.source==0?c.network->a:c.network->b).sendMove(seq,action,ack);
  }
  static void moveReceive(void *p,uint16_t seq,uint8_t action,bool ack,uint32_t now){
    auto &c=*static_cast<Context *>(p);c.network->initDuels();c.network->duel[c.source].receive(seq,action,ack,now);
  }
  void initDuels(){
    if(!combat)return;
    for(int i=0;i<2;++i){auto &s=i==0?a:b;
      if(s.state==State::Linked && !duel[i].active)
        duel[i].begin(s.sessionId(),s.isInitiator(),i==0?1:0,i==0?3:8,s.opponent.classId,s.opponent.level,moveSend,&contexts[i],now);
    }
  }
  void startCombat(){combat=true;a.setMoveReceiver(moveReceive,&contexts[0]);b.setMoveReceiver(moveReceive,&contexts[1]);initDuels();}
  void flush(){
    int count=0;
    while(!queue.empty()){
      assert(++count<100);
      const auto d=queue.front();queue.pop_front();int target=1-d.source;
      if(d.destination[0]==255 || std::memcmp(d.destination,mac[target],6)==0)
        (target==0?a:b).receive(mac[d.source],d.packet,now);
    }
  }
  void advance(uint32_t step){now+=step;a.tick(now);b.tick(now);flush();initDuels();
    if(combat){duel[0].tick(now);duel[1].tick(now);flush();}}
  void discover(){advance(0);assert(a.peers[0].used && b.peers[0].used);}
};

int main(){
  // Funds admission runs once per accepted invitation, not once per retry.
  Network paid;paid.discover();int debits=0;
  paid.a.setAdmission([](void*,uint32_t,const Peer&){return false;},nullptr);
  assert(!paid.a.invite(0,987,0) && paid.a.state==State::Listing && paid.queue.empty());
  paid.a.setAdmission([](void *p,uint32_t,const Peer&){++*static_cast<int*>(p);return true;},&debits);
  assert(paid.a.invite(0,987,0));paid.flush();assert(debits==1);
  paid.b.setAdmission([](void*,uint32_t,const Peer&){return false;},nullptr);
  paid.b.accept(0);assert(paid.b.state==State::Incoming);
  paid.advance(750);assert(debits==1);
  paid.b.setAdmission([](void *p,uint32_t,const Peer&){++*static_cast<int*>(p);return true;},&debits);
  paid.b.accept(paid.now);paid.flush();assert(debits==2 && paid.a.state==State::Linked);
  paid.advance(750);paid.b.accept(paid.now);assert(debits==2);
  Network unavailable;unavailable.b.setAvailable(false);unavailable.discover();
  assert(!unavailable.a.invite(0,988,0));
  Network n;n.discover();
  Standing ranking;ranking.points=240;ranking.wins=7;ranking.losses=2;ranking.draws=1;ranking.slot=2;
  n.b.setStanding(ranking);n.advance(1500);
  assert(n.a.peers[0].standing.points==240 && n.a.peers[0].standing.wins==7 && n.a.peers[0].standing.slot==2);
  assert(n.a.peers[0].level==8 && std::strcmp(n.a.peers[0].name,"MAGO")==0);
  assert(!n.a.invite(-1,1,n.now) && !n.a.invite(6,1,n.now) && !n.a.invite(0,0,n.now));
  assert(n.a.invite(0,123,n.now));n.flush();assert(n.b.state==State::Incoming);
  n.b.accept(n.now);n.flush();assert(n.a.state==State::Linked && n.b.state==State::Linked);
  for(int i=0;i<20;++i)n.advance(750);
  assert(n.a.state==State::Linked && n.b.state==State::Linked);
  n.a.cancel(n.now);n.flush();assert(n.b.state==State::Notice && n.b.notice==Notice::Cancelled);

  Network arena;arena.discover();arena.a.invite(0,100,0);arena.flush();arena.b.accept(0);arena.flush();
  arena.startCombat();arena.advance(0);assert(arena.duel[0].ready && arena.duel[1].ready);
  Packet forged{MAGIC,100,VERSION,(uint8_t)Type::Move,1,3,"CAVALEIRO",0,{1,0,0}};
  uint8_t stranger[6]={2,3,4,5,6,7};
  arena.b.receive(stranger,forged,0);assert(arena.duel[1].revision==0);
  forged.session=101;arena.b.receive(arena.mac[0],forged,0);assert(arena.duel[1].revision==0);
  forged.session=100;forged.classId=3;arena.b.receive(arena.mac[0],forged,0);assert(arena.duel[1].revision==0);
  arena.drops[(int)Type::Move]=1;
  assert(arena.duel[0].play(Action::Attack,arena.now));arena.flush();arena.advance(400);
  assert(arena.duel[1].revision==1 && !arena.duel[0].waiting);
  arena.drops[(int)Type::MoveAck]=1;
  assert(arena.duel[1].play(Action::Surrender,arena.now));arena.flush();arena.advance(400);
  assert(arena.duel[0].winner==0 && arena.duel[1].winner==0 && !arena.duel[1].waiting);

  // Every handshake leg can be lost without reporting a fabricated opponent.
  for(Type missing:{Type::Invite,Type::Accept,Type::Confirm,Type::Ready}){
    Network lossy;lossy.discover();lossy.drops[(int)missing]=1;
    lossy.a.invite(0,555,lossy.now);lossy.flush();
    for(int i=0;i<8;++i){
      if(lossy.b.state==State::Incoming)lossy.b.accept(lossy.now);
      lossy.advance(750);
    }
    assert(lossy.a.state==State::Linked && lossy.b.state==State::Linked);
  }
  // Lost refusal is repeated, never presented again as a fresh invitation.
  Network refusal;refusal.discover();refusal.a.invite(0,77,0);refusal.flush();
  refusal.drops[(int)Type::Reject]=1;refusal.b.decline(0);refusal.flush();refusal.advance(750);
  assert(refusal.a.notice==Notice::Refused && refusal.b.state==State::Listing);

  Network timeout;timeout.discover();timeout.a.invite(0,88,0);timeout.flush();
  timeout.advance(30000);assert(timeout.a.state==State::Notice);
  assert(timeout.a.notice==Notice::Expired && timeout.b.state==State::Notice);

  Network simultaneous;simultaneous.discover();
  simultaneous.a.invite(0,99,0);simultaneous.b.invite(0,99,0);simultaneous.flush();
  assert(simultaneous.a.notice==Notice::Busy && simultaneous.b.notice==Notice::Busy);

  Network invalid;invalid.discover();
  Packet hello{MAGIC,0,VERSION,(uint8_t)Type::Hello,0,3,"MAGO",1,{0,0,0}};
  uint8_t third[6]={2,1,2,3,4,5};
  auto bad=hello;bad.version=9;invalid.a.receive(third,bad,0);assert(!invalid.a.peers[1].used);
  bad=hello;bad.version=2;invalid.a.receive(third,bad,0);assert(!invalid.a.peers[1].used);
  bad=hello;bad.standing.points=100000;invalid.a.receive(third,bad,0);assert(!invalid.a.peers[1].used);
  bad=hello;bad.standing.slot=3;invalid.a.receive(third,bad,0);assert(!invalid.a.peers[1].used);
  bad=hello;bad.classId=99;invalid.a.receive(third,bad,0);assert(!invalid.a.peers[1].used);
  bad=hello;std::memset(bad.name,'X',12);invalid.a.receive(third,bad,0);assert(!invalid.a.peers[1].used);
  invalid.a.receive(invalid.mac[0],hello,0);assert(!invalid.a.peers[1].used);
  invalid.a.invite(0,45,0);invalid.flush();
  bad=hello;bad.type=(uint8_t)Type::Accept;bad.session=45;
  invalid.a.receive(third,bad,0);assert(invalid.a.state==State::Outgoing);
  bad.session=46;invalid.a.receive(invalid.mac[1],bad,0);assert(invalid.a.state==State::Outgoing);
  invalid.a.tick(6001);assert(!invalid.a.peers[0].used);

  Network lost;lost.discover();lost.a.invite(0,10,0);lost.flush();lost.b.accept(0);lost.flush();
  lost.a.tick(6001);assert(lost.a.state==State::Notice && lost.a.notice==Notice::Lost);

  Network wrap;wrap.now=UINT32_MAX-1000;
  wrap.a.begin(wrap.mac[0],"CAVALEIRO",1,3,Network::send,&wrap.contexts[0],wrap.now);
  wrap.b.begin(wrap.mac[1],"MAGO",0,8,Network::send,&wrap.contexts[1],wrap.now);
  wrap.discover();
  wrap.a.invite(0,123,wrap.now);wrap.flush();wrap.advance(30000);
  assert(wrap.a.notice==Notice::Expired);
  std::cout<<"PASS: discovery, invitation, acceptance, refusal, cancellation, retries, timeout, collision, validation, disconnect, timer wrap and end-to-end duel\n";
}
