#include "../firmware/RPG_POKET_2/Persistence.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace rpg;
static unsigned count(const CombatTrace& t){unsigned n=0;for(unsigned i=0;i<t.count;++i)n+=t.steps[i].kind==TraceStep::Fatigue;return n;}
static void hitAndRecover(Game g,unsigned fatigue){
 assert(g.fatigue==fatigue&&g.phase==Phase::Hero);g.enemyHp=60000;
 auto fresh=g;fresh.fatigue=0;CombatTrace base,receipt;assert(!act(fresh,Action::Attack,&base)&&!act(g,Action::Attack,&receipt));
 unsigned penalty=fatigue>70?(fatigue>=90?2:1):0;
 assert(receipt.net==(base.net?std::max(1,int(base.net)-int(penalty)):0));
 assert(count(receipt)==unsigned(penalty&&base.net));assert(g.fatigue==fatigue&&g.randomState==fresh.randomState);
 const auto captured=receipt;
 // Reducing fatigue later cannot reinterpret the previous receipt.
 recoverFatigue(g,fatigue-70);assert(g.fatigue==70&&!fatigueCombatPenalty(g));
 assert(!memcmp(&receipt,&captured,sizeof(receipt)));
 g.phase=Phase::Hero;g.enemyHp=60000;assert(!act(g,Action::Attack,&receipt));assert(!count(receipt));
 g.phase=Phase::Home;g.fatigue=100;assert(rest(g)&&g.fatigue==0&&!fatigueCombatPenalty(g));
 assert(begin(g,2));assert(!act(g,Action::Attack,&receipt));assert(!count(receipt));
}
int main(){unsigned cases=0;
 for(unsigned cls=0;cls<4;++cls)for(unsigned seed=1;seed<=32;++seed){
  for(unsigned start:{66u,67u,86u,87u}){auto g=create(cls,seed);g.city=1;g.fatigue=start;assert(explore(g));hitAndRecover(g,start+4);++cases;}
  // Existing movement rules add one only for actual displacement. Approaching
  // the enemy starts combat without another charge; use that resolved value.
  for(unsigned start:{69u,70u,88u,89u}){auto g=create(cls,seed);g.city=1;g.crystals=1;g.fatigue=start;assert(!enterDungeon(g,0));dungeonFace(g,1);
   assert(!dungeonMove(g,1,0,0)&&g.fatigue==start+1&&g.phase==Phase::Home);
   assert(dungeonEnemyAhead(g)==0);auto xy=g.dungeonXY;
   assert(!dungeonMove(g,1,0,0)&&g.phase==Phase::Hero&&g.dungeonXY==xy&&g.fatigue==start+1);
   hitAndRecover(g,start+1);++cases;
  }
 }
 printf("PASS fatigue7 crossings: %u exploration (+4) and movement (+1) cases crossing 70/71/89/90; combat uses resolution-time fatigue, old receipt immutable, recovery/rest remove deduction on next hit\n",cases);
}
