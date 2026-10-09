#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/Progression.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace rpg;
Game mage(unsigned level,unsigned seed){auto g=create(0,seed);g.p.xp=dndXp[level-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;assert(begin(g,7));return g;}
int main(){
 unsigned compared=0,emptyRays=0;
 for(unsigned seed=1;seed<=1000;++seed)for(auto a:{Action::MagicMissile,Action::BurningHands,Action::ScorchingRay,Action::Fireball}){
  auto now=mage(10,seed);now.enemyHp=60000;auto old=now;old.p.level=9;assert(!usePower(now,a)&&!usePower(old,a));assert(now.randomState==old.randomState&&now.p.mp==old.p.mp);unsigned bonus=abilityMod(now.attributes[3]);
  if(a==Action::MagicMissile)assert(now.damage==old.damage+bonus);
  else if(a==Action::ScorchingRay){assert(now.damage==old.damage+(old.damage?bonus:0));if(!old.damage)++emptyRays;}
  else assert(now.damage==old.damage+bonus||now.damage==old.damage+bonus/2||now.damage==old.damage+(bonus+1)/2);
  ++compared;
 }
 assert(emptyRays);
 for(unsigned cls=0;cls<4;++cls)for(unsigned lv=1;lv<=20;++lv)for(auto a:{Action::MagicMissile,Action::BurningHands,Action::ScorchingRay,Action::ShieldSpell,Action::Fireball,Action::DivineSmite}){auto g=create(cls,71);g.p.xp=dndXp[lv-1];levelUp(g);bool mastery=cls==0&&lv>=18&&(a==Action::MagicMissile||a==Action::ScorchingRay);assert(powerCost(g,a)==(mastery?0:powerCost(a)));g.dndProgression=false;assert(!masteredSpell(g,a)&&!evocationBonus(g,a)&&powerCost(g,a)==powerCost(a));}
 // Free spells still consume the attack turn, maintain their dice and level gates.
 for(auto a:{Action::MagicMissile,Action::ScorchingRay}){auto g=mage(18,91);g.p.mp=0;assert(!act(g,a)&&g.p.mp==0&&g.phase==Phase::Enemy);assert(valid(g));uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,3,b);Game boot;uint32_t seq;assert(decode(b,boot,seq)==Decode::Ok);assert(act(boot,a));encode(boot,3,c);assert(!memcmp(b,c,SAVE_SIZE));auto before=mage(17,91);before.p.mp=0;encode(before,3,b);assert(act(before,a));encode(before,3,c);assert(!memcmp(b,c,SAVE_SIZE));}
 auto g=mage(18,41);g.p.mp=0;assert(act(g,Action::ShieldSpell)&&act(g,Action::Fireball));
 // No bonus on the staff or protective spell. Intelligence changes the bonus.
 g=mage(10,42);assert(!evocationBonus(g,Action::Attack)&&!evocationBonus(g,Action::ShieldSpell));for(unsigned score=8;score<=20;++score){g.attributes[3]=score;assert(evocationBonus(g,Action::Fireball)==unsigned(std::max(0,abilityMod(score))));}
 g=mage(18,42);g.enemyHp=1;g.p.mp=0;assert(!act(g,Action::MagicMissile)&&g.phase==Phase::Won);auto xp=g.p.xp,gold=g.p.gold,rng=g.randomState;finish(g);assert(act(g,Action::MagicMissile));assert(g.p.xp==xp&&g.p.gold==gold&&g.randomState==rng);
 for(unsigned lv:{10u,18u}){auto gains=levelBenefits(g,lv);bool found=false;for(unsigned i=0;i<gains.count;++i)found|=strstr(gains.lines[i],lv==10?"Evocacao":"Maestria")!=nullptr;assert(found);}
 printf("PASS: %u evocation comparisons, miss-only rays=%u; INT once/cast, save reduction, mastery18/costs/turns, legacy and save19 no replay.\n",compared,emptyRays);
}
