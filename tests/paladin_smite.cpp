#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/Progression.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace rpg;
Game hero(unsigned cls,unsigned level,unsigned seed){auto g=create(cls,seed);g.p.xp=dndXp[level-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;assert(begin(g,7));return g;}
void reject(Game g){uint8_t a[SAVE_SIZE],b[SAVE_SIZE];encode(g,1,a);assert(act(g,Action::DivineSmite));encode(g,1,b);assert(!memcmp(a,b,SAVE_SIZE));}
int main(){
 reject(hero(1,1,1));for(auto cls:{0u,2u,3u})reject(hero(cls,5,1));auto g=hero(1,2,1);g.p.mp=2;reject(g);g=hero(1,2,1);g.phase=Phase::Home;reject(g);g=hero(1,2,1);g.dndProgression=false;reject(g);
 unsigned hits=0,misses=0,criticals=0;
 for(auto level:{2u,5u,10u,11u,20u})for(unsigned seed=1;seed<500;++seed)for(auto id:{7u,5u}){
  auto smite=hero(1,level,seed);smite.enemyId=id;smite.enemyHp=60000;auto base=smite;auto mp=smite.p.mp;
  assert(!act(base,Action::Attack));assert(!usePower(smite,Action::DivineSmite));assert(smite.dodge==base.dodge&&smite.crit==base.crit);
  assert(!smite.oath&&!smite.channelSpent);assert(smite.phase==Phase::Enemy);
  if(smite.dodge){++misses;assert(!smite.damage&&smite.p.mp==mp);}else{++hits;unsigned dice=(2+unsigned(undead(id)))*(smite.crit?2:1);int delta=int(smite.damage)-int(base.damage);assert(delta>=int(dice)&&delta<=int(dice*8));assert(smite.p.mp==mp-3);if(smite.crit)++criticals;}
 }
 assert(hits&&misses&&criticals);
 // Passive is not a mana charge and does not exist for preserved legacy heroes.
 for(unsigned seed=1;seed<200;++seed){auto now=hero(1,11,seed);now.enemyHp=60000;auto old=now;old.p.level=10;unsigned mp=now.p.mp;assert(!act(now,Action::Attack));assert(!act(old,Action::Attack));assert(now.p.mp==mp);assert(now.dodge==old.dodge);if(!now.dodge){unsigned dice=2*(now.crit?2:1);assert(now.damage>=old.damage+dice&&now.damage<=old.damage+dice*8);}}
 // Save19 resumes the already-spent turn and cannot replay a hit/reward.
 g=hero(1,2,91);assert(!act(g,Action::DivineSmite));assert(valid(g));uint8_t bytes[SAVE_SIZE],after[SAVE_SIZE];encode(g,8,bytes);Game boot;uint32_t seq;assert(decode(bytes,boot,seq)==Decode::Ok&&seq==8);assert(act(boot,Action::DivineSmite));encode(boot,8,after);assert(!memcmp(bytes,after,SAVE_SIZE));
 bool found=false;g=hero(1,11,1);auto benefits=levelBenefits(g,11);for(unsigned i=0;i<benefits.count;++i)found|=strstr(benefits.lines[i],"aprimorada")!=nullptr;assert(found);
 printf("PASS: Paladin smite gates, %u hits/%u misses/%u criticals; undead bonus, mana only on hit, passive extra attacks, save19 no replay.\n",hits,misses,criticals);
}
