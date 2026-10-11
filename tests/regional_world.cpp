#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/Bestiary.h"
#include "TestHero.h"
#include "LegacySave.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
Game resumed(Game g){assert(valid(g));uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,45,b);Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&seq==45&&h.questId==g.questId&&h.questProgress==g.questProgress&&h.seenEnemies==g.seenEnemies);encode(h,45,c);assert(!memcmp(b,c,SAVE_SIZE));return h;}
Game local(unsigned city){auto g=testHero(2,87);g.city=city;g.guildMember=true;g.p.level=cityLevel(city);g.p.maxmp=g.p.mp=totalMana(g);return g;}
void win(Game& g,unsigned id){assert(begin(g,id));g.enemyHp=0;finish(g);g=resumed(g);assert(home(g));}
void travel(Game& g,unsigned dest,bool safe=true){assert(!prepareTrip(g,dest));g.tripRoll=safe?20:1;g.tripTotal=g.tripRoll+g.tripSurvival+g.tripLuck;g=resumed(g);assert(acceptTrip(g));if(!safe){g.enemyHp=0;finish(g);assert(home(g));}g=resumed(g);assert(arriveTrip(g));g=resumed(g);}
int main(){
 for(unsigned city=2;city<4;++city){auto g=local(city);unsigned day[29]={},night[29]={};for(unsigned n=0;n<400;++n){++day[regionalEnemy(g,n)];++night[regionalEnemy(g,n,true)];}unsigned count=0;for(unsigned i=0;i<29;++i)count+=day[i]>0;assert(count==3);assert(day[city==2?26:28]<night[city==2?26:28]);for(unsigned id=25;id<29;++id){g=local(city);assert(begin(g,id));g=resumed(g);assert(bestiaryKnown(g,id));}}
 for(unsigned city=0;city<4;++city)for(unsigned slot=0;slot<3;++slot){unsigned id=contractOffer(city,slot);auto g=local(city);assert(!acceptQuest(g,id));g=resumed(g);unsigned gold=contractGold(id,g.questLevel),xp=contractXp(g,id,g.questLevel);
  if(slot==0){auto target=contract(id).enemy<0?(city==0?4:5):contract(id).enemy;for(unsigned n=0;n<contract(id).count;++n)win(g,target);}
  if(slot==1){for(unsigned n=0;n<2;++n){bool found=false;for(unsigned attempt=0;attempt<300&&!found;++attempt){assert(startDiscovery(g));if(g.phase==Phase::Hero){g.phase=Phase::Fled;assert(home(g));continue;}if(g.discovery==6){g=resumed(g);auto rng=g.randomState;unsigned rations=g.rations;assert(!collectDiscovery(g)&&g.randomState==rng&&g.rations==rations&&g.discovery==7);g=resumed(g);assert(collectDiscovery(g));found=true;}clearDiscovery(g);}assert(found);g=resumed(g);}}
  if(slot==2){travel(g,contractDestination(id),city%2==0);assert(g.questProgress==1);}
  assert(questComplete(g));g=resumed(g);auto before=g.p.gold;auto level=g.questLevel;assert(!claimQuest(g)&&g.p.gold==before+gold&&!g.questId);g=resumed(g);before=g.p.gold;assert(claimQuest(g)&&g.p.gold==before);assert(gold==contractGold(id,level)&&xp==contractXp(g,id,level));
 }
 auto g=local(2);assert(!acceptQuest(g,10));g.city=3;win(g,25);assert(!g.questProgress);g.city=2;win(g,6);assert(!g.questProgress);win(g,25);assert(g.questProgress==1);g=resumed(g);
 g=local(3);assert(!acceptQuest(g,15));assert(!prepareTrip(g,2));g.tripRoll=1;g.tripTotal=1+g.tripSurvival+g.tripLuck;assert(acceptTrip(g));g.phase=Phase::Fled;assert(home(g)&&!g.tripStage&&!g.questProgress);g=resumed(g);travel(g,1);assert(!g.questProgress);
 g=local(0);assert(!acceptQuest(g,5));g.city=2;for(unsigned n=0;n<100;++n){assert(startDiscovery(g));assert(g.discovery!=6);if(g.phase==Phase::Hero){g.phase=Phase::Fled;home(g);}else clearDiscovery(g);}assert(!g.questProgress);
 g=local(3);g.seenEnemies=(1u<<29)-1;g=resumed(g);assert(bestiaryCount(g)==29);g.seenEnemies|=1u<<29;assert(!valid(g));
 uint8_t b[SAVE_SIZE];Game h;uint32_t seq;g=local(1);assert(!acceptQuest(g,2));g.questProgress=1;encode(g,1,b);legacyFormat(b,24);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Ok&&h.questId==2&&h.questProgress==1);encode(g,1,b);b[67]|=128;legacyFormat(b,24);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);encode(g,1,b);b[116]|=4;legacyFormat(b,24);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);encode(g,1,b);legacyFormat(b,29);assert(decode(b,h,seq)==Decode::Unsupported);
 unsigned battles=0,losses=0,totalTurns=0;
 for(unsigned city=2;city<4;++city)for(unsigned cls=0;cls<4;++cls)for(unsigned choice=0;choice<3;++choice)for(unsigned seed=1;seed<=20;++seed){
  g=create(cls,seed);g.city=city;g.p.xp=dndXp[cityLevel(city)-1];levelUp(g);g.p.gold=5000;g.p.life=6;g.p.mana=3;
  for(unsigned gear:{cls*3+3,15u,18u}){unsigned previous=g.city;g.city=3;assert(!buyGear(g,gear)&&!equipGear(g,gear));g.city=previous;}
  g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;unsigned id=city==2?(choice==0?6:24+choice):(choice==0?7:26+choice);assert(begin(g,id));unsigned turns=0;
  while(g.phase==Phase::Hero||g.phase==Phase::Enemy){assert(++turns<200);if(g.phase==Phase::Enemy)enemy(g);else {Action a=g.p.hp*2<g.p.maxhp&&g.p.life?Action::Life:cls==0&&!powerError(g,Action::ScorchingRay)?Action::ScorchingRay:Action::Attack;assert(!act(g,a));}}
  ++battles;losses+=g.phase!=Phase::Won;totalTurns+=turns;g=resumed(g);
 }
 assert(losses==0);printf("PASS: regional balance %u battles/4 classes at recommended levels, losses=%u, mean action steps=%.1f (tier3 gear,6HP potions)\n",battles,losses,double(totalTurns)/battles);
 puts("PASS:12 local contracts; combat/recovery/real escorts; wrong-region and fleeing safeguards; night composition;29 bestiary bits;save25/24 migration;once-only rewards");
}
