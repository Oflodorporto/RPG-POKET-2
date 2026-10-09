#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/Progression.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <fstream>
using namespace rpg;
int main(int argc,char** argv){
 for(unsigned cls=0;cls<4;++cls)for(unsigned level=1;level<=20;++level){auto g=create(cls,41);g.p.xp=dndXp[level-1];levelUp(g);uint8_t before[SAVE_SIZE],after[SAVE_SIZE];encode(g,7,before);auto gains=levelBenefits(g,level);assert(gains.count<=5);for(unsigned i=0;i<gains.count;++i)assert(strlen(gains.lines[i])<=36);unsigned next=nextBenefitLevel(g);assert(!next||next>level);if(next)assert(levelBenefits(g,next).count);assert(!nextAttributeLevel(g)||nextAttributeLevel(g)>level);
  for(unsigned a=unsigned(Action::MagicMissile);a<=unsigned(Action::RagePower);++a){auto action=Action(a);unsigned owner=action==Action::RagePower?3:action==Action::SecondWind||action==Action::ActionSurge?2:a>=unsigned(Action::LayHands)?1:0;if(owner==cls&&powerLevel(action)==level){bool found=false;for(unsigned i=0;i<gains.count;++i)found|=!strcmp(gains.lines[i],powerName(action));assert(found);}}
  encode(g,7,after);assert(!memcmp(before,after,SAVE_SIZE));if(level<20){auto nextGame=g;unsigned hp=g.p.maxhp,mp=g.p.maxmp;nextGame.p.xp=xpNeeded(g);levelUp(nextGame);assert(nextGame.p.level==level+1&&nextGame.p.maxhp-hp==hpPerLevel(nextGame)&&nextGame.p.maxmp-mp==1);}
  auto points=advancementPoints(g);for(unsigned i=0;i<7;++i){auto copy=g;auto err=i==6?learnTough(copy):improveAttribute(copy,i);if(!err){assert(advancementPoints(copy)==points-(i==6?2:1));uint8_t bytes[SAVE_SIZE];encode(copy,4,bytes);Game boot;uint32_t seq;assert(decode(bytes,boot,seq)==Decode::Ok&&boot.advancementSpent==copy.advancementSpent);}encode(g,7,after);assert(!memcmp(before,after,SAVE_SIZE));}
 }
 Game old;assert(!old.dndProgression&&!nextBenefitLevel(old)&&!nextAttributeLevel(old)&&!levelBenefits(old,4).count);
 std::ofstream report;if(argc>1){report.open(argv[1]);assert(report);report<<"class,level,city,policy,battles,wins,losses,turns,winner_xp,winner_gold,ideal_wins_next_level\n";}
 unsigned fights=0,losses=0;
 for(unsigned cls=0;cls<4;++cls)for(unsigned level=1;level<=20;++level)for(unsigned city=0;city<4;++city)for(unsigned policy=0;policy<2;++policy){unsigned wins=0,lost=0,turns=0,xp=0,gold=0;
  for(unsigned seed=1;seed<=64;++seed){auto g=create(cls,seed*397);g.p.xp=dndXp[level-1];levelUp(g);g.city=city;g.oath=cls==1&&level>=3?1:0;g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;assert(explore(g));unsigned count=0;
   while((g.phase==Phase::Hero||g.phase==Phase::Enemy)&&count<100){if(g.phase==Phase::Enemy){assert(enemy(g));continue;}++count;Action a=Action::Attack;if(policy){if(cls==0){a=level>=5?Action::Fireball:level>=3?Action::ScorchingRay:Action::MagicMissile;if(powerError(g,a))a=Action::Attack;}else if(cls==1){if(g.p.hp<g.p.maxhp/2&&!powerError(g,Action::LayHands))a=Action::LayHands;else if(!g.sacredTurns&&!powerError(g,Action::SacredWeapon))a=Action::SacredWeapon;else if(level>=2&&g.p.mp>=skillCost(cls,false))a=Action::Offensive;}else if(cls==2){if(g.p.hp<g.p.maxhp/2&&!powerError(g,Action::SecondWind))a=Action::SecondWind;else if(!powerError(g,Action::ActionSurge))a=Action::ActionSurge;}else if(!powerError(g,Action::RagePower))a=Action::RagePower;}
    assert(!act(g,a));
   }
   assert(g.phase==Phase::Won||g.phase==Phase::Lost);turns+=count;++fights;if(g.phase==Phase::Won){++wins;xp+=g.gainXp;gold+=g.gainGold;}else {++lost;++losses;}
  }
  if(report)report<<cls<<','<<level<<','<<city<<','<<policy<<",64,"<<wins<<','<<lost<<','<<turns<<','<<xp<<','<<gold<<','<<(level==20?0:xp?double(dndXpNeeded(level))*wins/xp:0)<<'\n';
 }
 printf("PASS: live milestones/XP/previews for4 classes x20 levels; no mutation, save19 roundtrip, legacy preservation; %u benchmark battles, %u losses (fresh resources, no equipment).\n",fights,losses);
}
