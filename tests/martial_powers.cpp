#include "../firmware/RPG_POKET_2/Save.h"
#include "TestHero.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace rpg;
Game level(unsigned cls,unsigned lv){auto g=create(cls,47);g.p.xp=dndXp[lv-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;return g;}
Game boot(const Game& g){assert(valid(g));uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,7,b);Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&seq==7);encode(h,7,c);assert(!memcmp(b,c,SAVE_SIZE));return h;}
void reject(Game& g,Action a){uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,1,b);assert(act(g,a));encode(g,1,c);assert(!memcmp(b,c,SAVE_SIZE));}
int main(){
 auto g=level(2,1);g.p.hp=1;assert(!usePower(g,Action::SecondWind)&&g.p.hp>=3&&g.p.hp<=g.p.maxhp&&g.windSpent==1&&g.phase==Phase::Home);g=boot(g);assert(usePower(g,Action::SecondWind));assert(rest(g)&&!g.windSpent);
 begin(g,7);reject(g,Action::ActionSurge);reject(g,Action::RagePower);g.p.hp=1;auto mp=g.p.mp;assert(!act(g,Action::SecondWind)&&g.phase==Phase::Hero&&g.p.mp==mp);g=boot(g);reject(g,Action::SecondWind);
 g=level(2,2);begin(g,7);assert(!act(g,Action::ActionSurge)&&g.surgeSpent==1&&g.surgePending&&g.phase==Phase::Hero);g=boot(g);reject(g,Action::ActionSurge);assert(!act(g,Action::Attack)&&g.phase==Phase::Hero&&!g.surgePending);g=boot(g);assert(!act(g,Action::Attack)&&g.phase==Phase::Enemy);g=boot(g);enemy(g);assert(!g.surgeTurnUsed);reject(g,Action::ActionSurge);
 g=level(2,17);begin(g,7);assert(surgeUses(g)==2);act(g,Action::ActionSurge);reject(g,Action::ActionSurge);g.p.life=2;g.p.hp=1;assert(!act(g,Action::Life)&&g.phase==Phase::Hero&&!g.surgePending);assert(!act(g,Action::Defensive)&&g.phase==Phase::Enemy);enemy(g);assert(!act(g,Action::ActionSurge)&&g.surgeSpent==2);g=boot(g);
 // A killing extra action grants rewards once, clears the pending action, not spent uses.
 g=level(2,2);begin(g,0);act(g,Action::ActionSurge);g.enemyHp=1;act(g,Action::Attack);assert(g.phase==Phase::Won&&!g.surgePending&&!g.surgeTurnUsed&&g.surgeSpent==1);auto gold=g.p.gold;finish(g);assert(g.p.gold==gold);g=boot(g);home(g);assert(g.surgeSpent==1);
 for(unsigned lv=1;lv<=20;++lv){g=level(3,lv);unsigned expected=lv==20?255:lv>=17?6:lv>=12?5:lv>=6?4:lv>=3?3:2;assert(rageUses(g)==expected&&rageBonus(g)==(lv>=16?4:lv>=9?3:2));g=boot(g);}
 g=level(3,1);begin(g,7);assert(!act(g,Action::RagePower)&&g.rageTurns==3&&g.rageSpent==1&&g.phase==Phase::Hero);g=boot(g);reject(g,Action::RagePower);
 // Identical RNG verifies additive damage and physical-only resistance, including guard/crit.
 for(unsigned seed=1;seed<400;++seed){auto base=level(3,5);begin(base,7);base.randomState=seed;auto rage=base;rage.rageSpent=1;rage.rageTurns=3;act(base,Action::Attack);act(rage,Action::Attack);assert(base.randomState==rage.randomState&&base.dodge==rage.dodge);assert(rage.damage==base.damage+(base.dodge?0:rageBonus(rage)*attacksPerAction(rage)));
  for(unsigned id:{5u,7u,8u})for(unsigned guard:{0u,50u,75u}){base=level(3,5);begin(base,id);base.phase=Phase::Enemy;base.guard=guard;base.p.hp=base.p.maxhp=100;base.randomState=seed;rage=base;rage.rageSpent=1;rage.rageTurns=3;unsigned ceiling=incomingCeiling(rage,guard);enemy(base);enemy(rage);assert(rage.damage==(physicalEnemy(id)?base.damage/2:base.damage)&&rage.damage<=ceiling&&rage.rageTurns==2);}}
 g=level(3,1);begin(g,7);act(g,Action::RagePower);g.p.hp=g.p.maxhp=100;for(unsigned i=0;i<3;++i){g.phase=Phase::Enemy;enemy(g);assert(g.rageTurns==2-i);g=boot(g);}act(g,Action::RagePower);assert(g.rageSpent==2);g.phase=Phase::Fled;home(g);assert(!g.rageTurns&&g.rageSpent==2);begin(g,7);reject(g,Action::RagePower);
 g=level(3,19);g.rageSpent=6;g.p.xp=dndXpNeeded(19);levelUp(g);assert(g.p.level==20&&!g.rageSpent);g=boot(g);for(unsigned i=0;i<300;++i){g.phase=Phase::Home;begin(g,7);act(g,Action::RagePower);assert(g.rageTurns==3&&!g.rageSpent);g.phase=Phase::Fled;home(g);}g=boot(g);
 // Camp resources restore only on completion, even if HP and MP were already full.
 for(unsigned cls:{2u,3u}){g=level(cls,2);if(cls==2)g.windSpent=g.surgeSpent=1;else g.rageSpent=1;assert(!startCamp(g,false,false));g=boot(g);assert(g.windSpent||g.rageSpent);g.campStage=3;assert(finishCamp(g)&&!g.windSpent&&!g.surgeSpent&&!g.rageSpent);g=boot(g);}
 // Import save13 without any new resources; reserved bytes and impossible buffs are rejected.
 g=level(2,2);uint8_t b[SAVE_SIZE];encode(g,2,b);put16(b,4,13);put32(b,124,crc(b,124));Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&!h.windSpent&&!h.surgePending);h=boot(h);
 for(unsigned off=114;off<124;++off){encode(g,2,b);b[off]=255;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);}
 g=testHero(2,47);begin(g,7);reject(g,Action::SecondWind);reject(g,Action::ActionSurge);g=testHero(3,47);begin(g,7);reject(g,Action::RagePower);
 puts("PASS: Fighter heal/extra action, once per turn, level17 uses, rage levels/3 rounds/damage types, deterministic bounds, cap20 unlimited, camp, legacy rules and save14/read13.");
}
