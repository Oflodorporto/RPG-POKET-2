#include "LegacySave.h"
#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
void roundtrip(Game& g){assert(valid(g));uint8_t b[SAVE_SIZE];encode(g,42,b);Game copy;uint32_t seq;assert(decode(b,copy,seq)==Decode::Ok&&seq==42);uint8_t c[SAVE_SIZE];encode(copy,42,c);assert(!memcmp(b,c,SAVE_SIZE));g=copy;}
int main(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city){auto g=testHero(cls,1);g.city=city;g.p.level=18;g.p.maxmp=totalMana(g);g.p.gold=10000;unsigned count=cityGearCount(g);assert(count>0&&count<=9);
  for(unsigned i=0;i<count;++i){uint8_t id=cityGearOffer(g,i);assert(cityStock(g,id)&&gearAllowed(id,cls,g.p.level));auto old=g.p.gold;assert(!buyGear(g,id)&&g.p.gold==old-cityGearPrice(g,id));}assert(!cityGearOffer(g,count));
  auto old=g.p.gold;assert(!buyGood(g)&&g.p.gold==old-goodPrice(city));assert(goodCount(g,localGood(city))==1);roundtrip(g);
  old=g.p.gold;assert(!buyPotion(g,true)&&g.p.gold==old-potionPrice(g,true));roundtrip(g);
 }
 auto g=testHero(0,1);g.p.gold=10000;assert(gearBuyError(g,3));g.city=2;assert(!cityStock(g,3));g.city=3;assert(cityStock(g,3));
 bool safe=false,failed=false,one=false,twenty=false;
 for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city)for(unsigned dest=0;dest<4;++dest)for(unsigned seed=1;seed<100;++seed){if(city==dest)continue;g=testHero(cls,seed);g.city=city;g.rations=g.charts=g.charms=1;assert(!prepareTrip(g,dest));assert(!g.rations&&!g.charts&&!g.charms);assert(g.tripSurvival==survival(g)+3&&g.tripLuck==luck(g)+3);roundtrip(g);
  auto result=g.tripRoll;auto randomState=g.randomState;assert(prepareTrip(g,dest)&&g.tripRoll==result&&g.randomState==randomState);bool pass=tripSafe(g);safe|=pass;failed|=!pass;one|=result==1;twenty|=result==20;if(result==1)assert(!pass);if(result==20)assert(pass);
  assert(acceptTrip(g));roundtrip(g);assert(!acceptTrip(g));
  if(!pass){assert(g.tripStage==2&&g.enemyId==g.tripEnemy);g.enemyHp=0;finish(g);assert(!g.ruinsWins&&!g.questProgress);roundtrip(g);assert(home(g)&&g.tripStage==3);roundtrip(g);}
  assert(arriveTrip(g)&&g.city==dest&&!g.tripStage);roundtrip(g);assert(!arriveTrip(g));
 }
 assert(safe&&failed&&one&&twenty);
 for(bool flee:{false,true}){g=testHero(0,1);for(unsigned seed=1;seed<100;++seed){g=testHero(0,seed);prepareTrip(g,3);if(!tripSafe(g))break;}acceptTrip(g);if(flee)g.phase=Phase::Fled;else {g.p.hp=0;finish(g);}roundtrip(g);home(g);assert(flee?g.tripStage==3:!g.tripStage);assert(g.city==0);roundtrip(g);}
 // v7 checkpoints import without altering race, outfit, guild, inventory or city.
 g=testHero(2,90);g.city=2;g.guildMember=true;g.shirt=6;g.trousers=7;uint8_t b[SAVE_SIZE];encode(g,18,b);legacyFormat(b,7);put16(b,6,96);put32(b,92,crc(b,92));Game copy;uint32_t seq;assert(decode(b,copy,seq)==Decode::Ok&&!copy.tripStage&&copy.city==2&&copy.shirt==6&&copy.guildMember);roundtrip(copy);
 for(unsigned city=0;city<4;++city){g=testHero(0,1);g.city=city;auto preview=g;auto expected=regionalEnemy(preview,random(preview));assert(explore(g));assert(g.enemyId==expected);assert(valid(g));}
 puts("PASS: 4752 persisted journeys; natural 1/20, survival/luck/supplies, no reroll, loss/flee/arrival, v7 migration, regional enemies, stocks and transaction prices.");
}
