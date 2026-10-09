#include "LegacySave.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/ClassProgression.h"
#include "TestHero.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace rpg;
void roundtrip(Game& g){assert(valid(g));uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,1,b);Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok);encode(h,1,c);assert(!memcmp(b,c,SAVE_SIZE));g=h;}
int main(){
 for(unsigned cls=0;cls<4;++cls){auto g=create(cls,42);assert(g.p.level==1&&g.dndProgression&&g.p.xp==0&&totalExperience(g)==0&&xpNeeded(g)==300);roundtrip(g);
  for(unsigned lv=2;lv<=20;++lv){auto hp=g.p.maxhp;unsigned needed=xpNeeded(g);g.p.xp=needed-1;levelUp(g);assert(g.p.level==lv-1);g.p.xp=needed;levelUp(g);assert(g.p.level==lv&&g.p.xp==0&&totalExperience(g)==dndXp[lv-1]&&g.p.maxhp==hp+hpPerLevel(g));roundtrip(g);assert(*classMilestone(cls,lv));}
  g.p.xp=100000;levelUp(g);assert(g.p.level==20&&g.p.xp==0);roundtrip(g);
  assert(advancementPoints(g)==(cls==2?14:10));auto before=g.p.maxhp;assert(!learnTough(g)&&g.p.maxhp==before+40);assert(learnTough(g));roundtrip(g);
  before=g.p.maxhp;unsigned con=g.attributes[2];assert(!improveAttribute(g,2));assert(!improveAttribute(g,2));assert(g.attributes[2]==con+2&&g.p.maxhp==before+20);roundtrip(g);
  while(advancementPoints(g)){unsigned i=0;while(g.attributes[i]>=20)++i;assert(!improveAttribute(g,i));roundtrip(g);}auto spent=g.advancementSpent;assert(improveAttribute(g,0)&&g.advancementSpent==spent);
 }
 auto g=create(1,11);assert(begin(g,0));auto mp=g.p.mp;auto seed=g.randomState;assert(act(g,Action::Offensive)&&g.p.mp==mp&&g.randomState==seed&&g.phase==Phase::Hero);
 g=create(0,11);assert(explore(g)&&g.enemyHp==6&&g.enemyId<2);auto state=g.randomState;unsigned bound=incomingCeiling(g);assert(g.randomState==state);for(unsigned seed=1;seed<500;++seed){auto copy=g;copy.randomState=seed;copy.phase=Phase::Enemy;copy.p.hp=copy.p.maxhp=100;assert(enemy(copy)&&copy.damage<=bound);}
 auto old=testHero(2,42);uint8_t b[SAVE_SIZE];encode(old,10,b);legacyFormat(b,11);put32(b,124,crc(b,124));uint32_t seq;Game h;assert(decode(b,h,seq)==Decode::Ok&&!h.dndProgression&&h.p.level==3&&h.p.maxhp==20);roundtrip(h);
 for(unsigned cls=1;cls<4;++cls){g=create(cls,1);g.p.level=5;assert(attacksPerAction(g)==2);}g=create(2,1);g.p.level=11;assert(attacksPerAction(g)==3);g.p.level=20;assert(attacksPerAction(g)==4);
 // Incomplete spending cannot turn a single remaining point into a whole feat.
 g=create(0,1);g.p.xp=2700;levelUp(g);assert(g.p.level==4);assert(!improveAttribute(g,0));assert(learnTough(g));roundtrip(g);
 puts("PASS: D&D XP1..20, four classes, level1 creation, HP, cap, 6 attributes, class ASI milestones, Tough, no double spend, save12/read11, skill lock, low-level encounters and risk bound.");
}
