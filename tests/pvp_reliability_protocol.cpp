// Regression suite: two real Session/Duel endpoints, based on independent audit repros.
#define main original_protocol_main
#include "club_protocol.cpp"
#undef main

static void link(Network& n, uint32_t id=100) {
  n.discover();assert(n.a.invite(0,id,n.now));n.flush();n.b.accept(n.now);n.flush();
  assert(n.a.state==State::Linked&&n.b.state==State::Linked);
  n.startCombat();n.advance(0);assert(n.duel[0].ready&&n.duel[1].ready);
}
static void equal(const Network& n) {
  assert(n.duel[0].revision==n.duel[1].revision);
  assert(n.duel[0].turn==n.duel[1].turn);
  assert(n.duel[0].winner==n.duel[1].winner);
  for (unsigned i=0;i<2;++i) {
    const auto& a=n.duel[0].fighters[i];const auto& b=n.duel[1].fighters[i];
    assert(a.hp==b.hp&&a.mp==b.mp&&a.guard==b.guard);
  }
}
int main(){
  // Five minutes with a live session but no player command does not time out.
  Network idle;link(idle);
  for(int i=0;i<400;++i)idle.advance(750);
  assert(idle.a.state==State::Linked&&idle.b.state==State::Linked);
  assert(!idle.duel[0].failed&&!idle.duel[1].failed);
  assert(idle.duel[1].play(Action::Surrender,idle.now));idle.flush();equal(idle);
  assert(idle.duel[0].terminalConfirmed&&idle.duel[1].terminalConfirmed&&idle.duel[0].winner==0);
  std::cout<<"PASS: no invented idle deadline; non-active player can surrender after 300 seconds\n";

  // Burst loss below the timeout, and retransmissions, converge.
  Network loss;link(loss);loss.drops[int(Type::Move)]=7;
  assert(loss.duel[0].play(Action::Offensive,loss.now));loss.flush();
  for(int i=0;i<9;++i)loss.advance(400);
  equal(loss);assert(!loss.duel[0].waiting&&loss.duel[0].revision==1);
  loss.drops[int(Type::MoveAck)]=7;
  assert(loss.duel[1].play(Action::Offensive,loss.now));loss.flush();
  for(int i=0;i<9;++i)loss.advance(400);
  equal(loss);assert(!loss.duel[1].waiting&&loss.duel[0].revision==2);
  std::cout<<"PASS: seven consecutive command drops and seven ACK drops converge without double damage\n";

  // One side receives the final hit but none of the final ACKs arrive.
  Network final;link(final);int actions=0;
  while(!final.duel[0].finished){
    int actor=final.duel[0].turn;
    const auto& attacker=final.duel[0].fighters[actor];
    const auto& defender=final.duel[0].fighters[1-actor];
    const int expectedDamage=std::max(1,int(attacker.attack)-defender.defense/3);
    final.drops[int(Type::MoveAck)]=defender.hp<=expectedDamage?100:0;
    assert(final.duel[actor].play(Action::Attack,final.now));final.flush();
    ++actions;
    assert(actions<200);
  }
  equal(final);int winner=final.duel[0].winner;
  assert(!final.duel[winner].waiting&&final.duel[winner].terminalConfirmed);
  for(int i=0;i<25;++i)final.advance(400);
  assert(final.a.state==State::Linked&&final.b.state==State::Linked);
  assert(final.duel[winner].finished&&!final.duel[winner].failed&&final.duel[winner].terminalConfirmed);
  assert(!final.duel[1-winner].failed);
  std::cout<<"PASS: independent terminal receipt confirms real lethal outcome despite all terminal MoveAck packets lost\n";

  // Once declared lost, a restored radio path does not resume the old session.
  Network reconnect;link(reconnect);
  reconnect.a.tick(6000);reconnect.b.tick(6000);reconnect.now=6000;reconnect.flush();
  assert(reconnect.a.notice==Notice::Lost&&reconnect.b.notice==Notice::Lost);
  reconnect.advance(750);
  assert(reconnect.a.state==State::Notice&&reconnect.b.state==State::Notice);
  reconnect.a.dismiss();reconnect.b.dismiss();reconnect.advance(1500);
  assert(reconnect.a.invite(0,202,reconnect.now));reconnect.flush();reconnect.b.accept(reconnect.now);reconnect.flush();
  assert(reconnect.a.state==State::Linked&&reconnect.b.state==State::Linked);
  std::cout<<"PASS: 6-second link gap terminates session; new ID permits a new invitation, old match does not resume\n";

  // Use a truly valid stakeSession so identity/sequence cases reach their guards.
  Network validation;link(validation);
  Packet p{MAGIC,100,VERSION,uint8_t(Type::Move),1,3,"CAVALEIRO",0,{1,0,0}};
  p.stakeSession=100;
  uint8_t stranger[6]={2,3,4,5,6,7};
  validation.b.receive(stranger,p,0);assert(validation.duel[1].revision==0);
  p.classId=3;validation.b.receive(validation.mac[0],p,0);assert(validation.duel[1].revision==0);
  p.classId=1;p.level=4;validation.b.receive(validation.mac[0],p,0);assert(validation.duel[1].revision==0);
  p.level=3;p.reserved[0]=2;validation.b.receive(validation.mac[0],p,0);assert(validation.duel[1].revision==0);
  p.reserved[0]=1;validation.b.receive(validation.mac[0],p,0);assert(validation.duel[1].revision==1);
  const auto hp=validation.duel[1].fighters[1].hp;
  for(int i=0;i<100;++i)validation.b.receive(validation.mac[0],p,i);
  assert(validation.duel[1].revision==1&&validation.duel[1].fighters[1].hp==hp);
  std::cout<<"PASS: source/class/level/future-sequence rejected with otherwise valid v5 packet; 100 duplicate commands apply once\n";

  // If both acknowledgement paths are partitioned, do not award a prediction.
  Network partition;link(partition);int pendingWinner=-1;
  while(!partition.duel[0].finished){
    int actor=partition.duel[0].turn;
    const auto& a=partition.duel[0].fighters[actor];const auto& b=partition.duel[0].fighters[1-actor];
    if(b.hp<=std::max(1,int(a.attack)-b.defense/3)){
      partition.drops[int(Type::MoveAck)]=100;partition.drops[int(Type::Result)]=100;pendingWinner=actor;
    }
    assert(partition.duel[actor].play(Action::Attack,partition.now));partition.flush();
  }
  assert(pendingWinner>=0&&!partition.duel[pendingWinner].terminalConfirmed);
  for(int i=0;i<25;++i)partition.advance(400);
  assert(partition.duel[pendingWinner].failed&&!partition.duel[pendingWinner].terminalConfirmed);
  std::cout<<"PASS: complete terminal-response partition never invents a confirmed victory\n";

  // Both queue orders for simultaneous voluntary surrender converge to the
  // already agreed active player's request, without two winners.
  for(unsigned first=0;first<2;++first)for(unsigned parity=0;parity<2;++parity){
    Network both;link(both,100+parity);
    assert(both.duel[first].play(Action::Surrender,both.now));
    assert(both.duel[1-first].play(Action::Surrender,both.now));both.flush();equal(both);
    assert(both.duel[0].terminalConfirmed&&both.duel[1].terminalConfirmed&&both.duel[0].winner==int(1-parity));
  }
  // Surrender during an in-flight opponent action is re-based after that
  // command. Loss of its ACK cannot trap the voluntary surrender.
  for(bool surrenderFirst:{false,true})for(bool lostAck:{false,true}){
    Network race;link(race);
    if(surrenderFirst)assert(race.duel[1].play(Action::Surrender,race.now));
    race.drops[int(Type::MoveAck)]=lostAck?1:0;
    assert(race.duel[0].play(Action::Attack,race.now));
    if(!surrenderFirst)assert(race.duel[1].play(Action::Surrender,race.now));
    race.flush();for(int i=0;i<5;++i)race.advance(400);equal(race);
    assert(race.duel[0].terminalConfirmed&&race.duel[1].terminalConfirmed&&race.duel[0].winner==0);
  }
  // The same player's outstanding ordinary command must settle before their
  // surrender request; a valid opponent response can replace the lost ACK.
  for(bool opponentMoves:{false,true}){
    Network pending;link(pending);pending.drops[int(Type::MoveAck)]=1;
    assert(pending.duel[0].play(Action::Attack,pending.now));pending.flush();
    assert(pending.duel[0].waiting&&pending.duel[0].play(Action::Surrender,pending.now));
    if(opponentMoves){assert(pending.duel[1].play(Action::Attack,pending.now));pending.flush();}
    for(int i=0;i<5;++i)pending.advance(400);equal(pending);
    assert(pending.duel[0].winner==1&&pending.duel[0].terminalConfirmed&&pending.duel[1].terminalConfirmed);
  }
  // Receipt identity/state guards, with valid stakeSession and positive control.
  Network receipt;link(receipt);receipt.drops[int(Type::MoveAck)]=100;receipt.drops[int(Type::Result)]=100;
  assert(receipt.duel[1].play(Action::Surrender,receipt.now));receipt.flush();
  Packet result{};result.magic=MAGIC;result.session=result.stakeSession=100;result.version=VERSION;
  result.type=uint8_t(Type::Result);result.classId=1;result.level=3;strcpy(result.name,"CAVALEIRO");
  result.reserved[0]=1;result.reserved[2]=1;result.resultDigest=receipt.duel[0].stateDigest();
  auto expectPending=[&](){assert(!receipt.duel[1].finished&&receipt.duel[1].surrenderRequested);};
  receipt.b.receive(stranger,result,0);expectPending();
  for(int field=0;field<6;++field){auto bad=result;
    if(field==0)bad.classId=2;if(field==1)bad.level=4;if(field==2)bad.resultDigest^=1;
    if(field==3)bad.reserved[0]=2;if(field==4)bad.reserved[2]=2;if(field==5)bad.stakeSession=99;
    receipt.b.receive(receipt.mac[0],bad,0);expectPending();
  }
  receipt.b.receive(receipt.mac[0],result,0);equal(receipt);assert(receipt.duel[1].terminalConfirmed);
  auto digest=receipt.duel[1].stateDigest();for(int i=0;i<100;++i)receipt.b.receive(receipt.mac[0],result,i);
  assert(receipt.duel[1].stateDigest()==digest&&receipt.duel[1].revision==1);
  // Old devices are discoverable for an explicit update label, never admitted.
  Network old;Packet hello{};hello.magic=MAGIC;hello.version=4;hello.type=uint8_t(Type::Hello);
  hello.classId=1;hello.level=5;hello.available=1;strcpy(hello.name,"OLD");
  old.a.receive(old.mac[1],hello,0);assert(old.a.peers[0].used&&old.a.peers[0].version==4);
  assert(!old.a.invite(0,300,0)&&old.a.state==State::Listing);
  hello.type=uint8_t(Type::Invite);hello.session=hello.stakeSession=300;
  old.a.receive(old.mac[1],hello,0);assert(old.a.state==State::Listing);
  std::cout<<"PASS: simultaneous/in-flight surrender races, receipt poison/duplicates and explicit old-version refusal\n";
}
