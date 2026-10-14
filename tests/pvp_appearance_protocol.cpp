#define main original_protocol_main
#include "club_protocol.cpp"
#undef main
int main(){
  unsigned styles=0;
  for(unsigned race=0;race<4;++race)for(unsigned shirt=0;shirt<8;++shirt)for(unsigned pants=0;pants<8;++pants){
    Network n;n.a.setAppearance(race,shirt,pants);n.b.setAppearance(3-race,7-shirt,7-pants);
    n.discover();assert(n.a.peers[0].race==3-race&&n.a.peers[0].shirt==7-shirt&&n.a.peers[0].trousers==7-pants);
    assert(n.b.peers[0].race==race&&n.b.peers[0].shirt==shirt&&n.b.peers[0].trousers==pants);
    assert(n.a.invite(0,24,0));n.flush();n.b.accept(0);n.flush();n.startCombat();n.advance(0);
    assert(n.a.opponent.race==3-race&&n.b.opponent.race==race);
    assert(n.a.opponent.shirt==7-shirt&&n.b.opponent.shirt==shirt);
    assert(n.a.opponent.trousers==7-pants&&n.b.opponent.trousers==pants);
    assert(n.duel[0].ready&&n.duel[1].ready);
    assert(n.duel[0].play(Action::Attack,0));assert(n.duel[0].confirmedRevision==0);n.flush();
    assert(n.duel[0].revision==1&&n.duel[0].confirmedRevision==1&&n.duel[1].confirmedRevision==1);
    assert(n.duel[0].stateDigest()==n.duel[1].stateDigest());++styles;
  }
  Packet hello{};hello.magic=MAGIC;hello.version=VERSION;hello.type=uint8_t(Type::Hello);
  hello.classId=1;hello.level=5;hello.available=1;strcpy(hello.name,"HERO");
  Network invalid;
  for(int field=0;field<4;++field){auto bad=hello;
    if(field==0)bad.race=4;if(field==1)bad.shirt=8;if(field==2)bad.trousers=8;if(field==3)bad.appearanceReserved=1;
    invalid.a.receive(invalid.mac[1],bad,0);assert(!invalid.a.peers[0].used);
  }
  invalid.a.receive(invalid.mac[1],hello,0);assert(invalid.a.peers[0].used);
  Network changed;changed.discover();assert(changed.a.invite(0,100,0));changed.flush();changed.b.accept(0);changed.flush();changed.startCombat();changed.advance(0);
  auto move=hello;move.type=uint8_t(Type::Move);move.session=move.stakeSession=100;move.level=3;move.reserved[0]=1;
  for(int field=0;field<3;++field){auto bad=move;
    if(field==0)bad.race=1;if(field==1)bad.shirt=1;if(field==2)bad.trousers=1;
    changed.b.receive(changed.mac[0],bad,0);assert(changed.duel[1].revision==0);
  }
  changed.b.receive(changed.mac[0],move,0);assert(changed.duel[1].revision==1);
  // Exact capped heal delta, independent of visible effect duration.
  Duel heal;heal.begin(100,true,3,5,0,5,nullptr,nullptr,0);heal.receive(0,0,false,0);heal.receive(0,0,true,0);
  heal.fighters[0].hp-=2;assert(heal.play(Action::Defensive,0));assert(heal.lastHealing==2&&heal.confirmedRevision==0);
  heal.receive(1,uint8_t(Action::Defensive),true,0);assert(heal.confirmedRevision==1);
  std::cout<<"PASS: "<<styles<<" complete paired appearance styles, bounds/identity poison, confirmed revisions and capped healing\n";
}
