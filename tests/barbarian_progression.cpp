#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/Progression.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace rpg;
Game hero(unsigned lv,unsigned seed){auto g=create(3,seed);g.p.xp=dndXp[lv-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;assert(begin(g,7));return g;}
Game boot(Game g){assert(valid(g));uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,7,b);Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&seq==7);encode(h,7,c);assert(!memcmp(b,c,SAVE_SIZE));return h;}
int main(){
 unsigned criticals=0,misses=0;
 for(unsigned lv:{8u,9u,12u,13u,16u,17u,20u})for(auto action:{Action::Attack,Action::Offensive})for(unsigned seed=1;seed<=1000;++seed){
  auto g=hero(lv,seed);g.enemyHp=60000;auto ref=g;clearFeedback(ref);bool skill=action==Action::Offensive;if(skill)ref.p.mp-=skillCost(3,false);
  int damage=rollDamage(ref,effectiveAttack(ref),enemySpec(ref.enemyId).def);if(!skill)damage*=attacksPerAction(ref);if(skill)damage*=2;ref.dodge=random(ref)%100<6;
  auto dice=brutalDice(g);unsigned extra=0;if(ref.crit&&!ref.dodge){for(unsigned i=0;i<dice;++i)extra+=1+random(ref)%6;++criticals;assert(extra>=dice&&extra<=dice*6);}if(ref.dodge)++misses;
  assert(!act(g,action));assert(g.crit==ref.crit&&g.dodge==ref.dodge&&g.randomState==ref.randomState&&g.p.mp==ref.p.mp);assert(g.damage==(ref.dodge?0:damage+extra));assert(g.phase==Phase::Enemy);
 }
 assert(criticals&&misses);
 for(unsigned cls=0;cls<4;++cls)for(unsigned lv=1;lv<=20;++lv){auto g=create(cls,19);g.p.xp=dndXp[lv-1];levelUp(g);assert(brutalDice(g)==(cls==3&&lv>=9?(lv>=17?3:lv>=13?2:1):0));assert(persistentRage(g)==(cls==3&&lv>=15));g.dndProgression=false;assert(!brutalDice(g)&&!persistentRage(g));}
 for(unsigned lv:{14u,15u,20u})for(unsigned id:{5u,7u})for(unsigned beat=0;beat<3;++beat){auto g=hero(lv,71);g.enemyId=id;g.enemyHp=enemySpec(id).hp;assert(!act(g,Action::RagePower));auto spent=g.rageSpent;g.p.hp=g.p.maxhp=1000;
  for(unsigned i=0;i<8;++i){g.phase=Phase::Enemy;g.enemyBeat=beat;assert(enemy(g));assert(g.rageTurns==(lv<15?(i<3?2-i:0):3));assert(g.rageSpent==spent);g=boot(g);}
  for(auto ending:{Phase::Won,Phase::Lost,Phase::Fled}){auto end=g;end.phase=ending;assert(home(end)&&!end.rageTurns&&end.rageSpent==spent);end=boot(end);}
 }
 // Existing save19 active countdown remains legal; resumed high-level rage persists.
 auto g=hero(15,32);g.rageSpent=1;g.rageTurns=1;g=boot(g);g.phase=Phase::Enemy;assert(enemy(g)&&g.rageTurns==1);g=boot(g);
 // Actual defeat and victory clear the effect, with no duplicate reward on retry.
 g=hero(15,42);g.p.hp=1;g.rageSpent=1;g.rageTurns=3;g.phase=Phase::Enemy;for(unsigned s=1;s<100;++s){auto end=g;end.randomState=s;enemy(end);if(end.phase==Phase::Lost){assert(!end.rageTurns);end=boot(end);break;}}
 g=hero(17,42);g.enemyHp=1;g.rageSpent=1;g.rageTurns=3;assert(!act(g,Action::Attack)&&g.phase==Phase::Won&&!g.rageTurns);g=boot(g);auto gold=g.p.gold,xp=g.p.xp,rng=g.randomState;assert(act(g,Action::Attack));finish(g);assert(g.p.gold==gold&&g.p.xp==xp&&g.randomState==rng);
 for(unsigned lv:{9u,13u,15u,17u}){auto gains=levelBenefits(g,lv);bool found=false;for(unsigned i=0;i<gains.count;++i)found|=strstr(gains.lines[i],lv==15?"persiste":"Critico brutal")!=nullptr;assert(found);}
 printf("PASS: 14000 Barbarian attacks, %u criticals/%u misses; d6 thresholds, no dice on miss, persistent rage15 cadence/reboot/endings, class/legacy/save19/rewards.\n",criticals,misses);
}
