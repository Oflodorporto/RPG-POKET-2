#include "ClubDuel.h"
#include <cassert>
#include <deque>
#include <iostream>
using namespace club;
struct Net {
  Duel d[2];
  struct Context {Net *net;int who;} ctx[2]={{this,0},{this,1}};
  struct Message {int who;uint16_t seq;uint8_t action;bool ack;};
  std::deque<Message> queue;
  bool dropMove=false,dropAck=false;
  uint32_t now=0;
  static void send(void *p,uint16_t seq,uint8_t action,bool ack){
    auto &c=*static_cast<Context *>(p);auto &n=*c.net;
    if(seq && ((ack && n.dropAck)||(!ack && n.dropMove))){(ack?n.dropAck:n.dropMove)=false;return;}
    n.queue.push_back({c.who,seq,action,ack});
  }
  Net(int c0=1,int c1=0,int level=3,uint32_t start=0):now(start){
    d[0].begin(24,true,c0,level,c1,level,send,&ctx[0],now);
    d[1].begin(24,false,c1,level,c0,level,send,&ctx[1],now);
    advance(0);assert(d[0].ready && d[1].ready);
  }
  void flush(){int count=0;while(!queue.empty()){
    assert(++count<100);auto m=queue.front();queue.pop_front();
    d[1-m.who].receive(m.seq,m.action,m.ack,now);
  }}
  void advance(uint32_t ms){now+=ms;d[0].tick(now);d[1].tick(now);flush();}
  void equal(){
    assert(d[0].revision==d[1].revision && d[0].turn==d[1].turn && d[0].winner==d[1].winner);
    for(int i=0;i<2;++i){assert(d[0].fighters[i].hp==d[1].fighters[i].hp);
      assert(d[0].fighters[i].mp==d[1].fighters[i].mp && d[0].fighters[i].guard==d[1].fighters[i].guard);}
  }
  void play(Action a){auto &x=d[d[0].turn];assert(x.play(a,now));flush();equal();}
};
int main(){
  for(int c0=0;c0<4;++c0)for(int c1=0;c1<4;++c1){
    Net n(c0,c1);assert(!n.d[1].play(Action::Attack,0));
    n.play(Action::Defensive);n.play(Action::Offensive);
    assert(n.d[0].fighters[0].guard==0);
    for(int i=0;i<200 && !n.d[0].finished;++i)n.play(Action::Attack);
    assert(n.d[0].finished && n.d[1].finished);
    assert(!n.d[0].play(Action::Attack,0));
  }
  for(int level:{1,3,99}){Net n(0,3,level);n.play(Action::Attack);n.equal();}
  Net lost;lost.dropMove=true;assert(lost.d[0].play(Action::Attack,0));lost.flush();
  assert(lost.d[0].waiting && lost.d[1].revision==0);lost.advance(400);lost.equal();
  assert(!lost.d[0].waiting);
  lost.dropAck=true;lost.play(Action::Attack);assert(lost.d[1].waiting);
  const auto hp=lost.d[0].fighters[0].hp;lost.advance(400);lost.equal();
  assert(!lost.d[1].waiting && lost.d[0].fighters[0].hp==hp);
  // A new valid opponent move implicitly acknowledges the previous move.
  lost.dropAck=true;lost.play(Action::Defensive);lost.play(Action::Defensive);
  assert(!lost.d[0].waiting);
  Net invalid;
  invalid.d[1].receive(9,0,false,0);invalid.d[0].receive(1,0,false,0);
  invalid.d[1].receive(1,99,false,0);assert(invalid.d[1].revision==0 && invalid.d[0].revision==0);
  invalid.play(Action::Attack);const auto before=invalid.d[1].fighters[1].hp;
  invalid.d[1].receive(1,0,false,0);invalid.flush();assert(invalid.d[1].fighters[1].hp==before);
  Net mana;mana.d[0].fighters[0].mp=0;mana.d[1].fighters[0].mp=0;
  assert(!mana.d[0].play(Action::Offensive,0));assert(!mana.d[0].play(Action::Defensive,0));
  mana.play(Action::Attack);
  Net surrender;surrender.play(Action::Surrender);assert(surrender.d[0].winner==1);
  // Losing the final ACK never applies the final action twice.
  Net finalAck;finalAck.dropAck=true;finalAck.play(Action::Surrender);
  assert(finalAck.d[0].waiting);finalAck.advance(400);
  assert(finalAck.d[0].finished && !finalAck.d[0].waiting && finalAck.d[1].revision==1);
  // Ready packets can arrive out of order and be retried independently.
  Duel readyTest;readyTest.begin(24,true,0,3,1,3,nullptr,nullptr,0);
  readyTest.receive(0,0,true,1);assert(!readyTest.ready);
  readyTest.receive(0,0,false,2);assert(readyTest.ready);
  assert(readyTest.play(Action::Attack,3));
  Net timeout;timeout.dropMove=true;timeout.d[0].play(Action::Attack,0);timeout.flush();
  timeout.now+=10000;timeout.d[0].tick(timeout.now);assert(timeout.d[0].failed);
  Net wrap(1,0,3,UINT32_MAX-200);wrap.dropMove=true;
  wrap.d[0].play(Action::Attack,wrap.now);wrap.flush();wrap.advance(400);wrap.equal();
  Duel unready;unready.begin(2,true,0,3,1,3,nullptr,nullptr,UINT32_MAX-200);
  assert(!unready.play(Action::Attack,0));unready.tick(14800);assert(unready.failed);
  Net draw;draw.d[0].revision=draw.d[1].revision=199;draw.play(Action::Defensive);
  assert(draw.d[0].finished && draw.d[0].winner==-1);
  std::cout<<"PASS: duel classes, levels, turns, skills, mana, surrender, duplicates, dropped moves/ACKs, timeout, wrap and draw\n";
}
