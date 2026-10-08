#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
Game checkpoint(const Game& g){uint8_t b[SAVE_SIZE];assert(valid(g));encode(g,12,b);Game copy;uint32_t seq;assert(decode(b,copy,seq)==Decode::Ok&&seq==12);return copy;}
int main(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city)for(unsigned kit=0;kit<2;++kit)for(unsigned food=0;food<2;++food)for(unsigned roll=1;roll<=20;++roll){
  auto g=testHero(cls,100+roll);g.city=city;g.sleepKit=kit;g.rations=2;g.p.hp=1;g.p.mp=0;
  assert(!startCamp(g,food,kit)&&g.rations==2-food);g.campRoll=roll;assert(prepareTrip(g,(city+1)%4));assert(enterDungeon(g));g=checkpoint(g);
  bool safe=campSafe(g);if(roll==1)assert(!safe);if(roll==20)assert(safe);auto hp=g.p.hp,mp=g.p.mp;
  assert(acceptCamp(g)&&g.campStage==(safe?3:2)&&g.p.hp==hp&&g.p.mp==mp);g=checkpoint(g);
  if(!safe){assert(g.enemyId==campEnemy(g));g.enemyHp=0;finish(g);g=checkpoint(g);assert(resolveCamp(g)&&g.campStage==3);g=checkpoint(g);}
  auto beforeHP=g.p.hp,beforeMP=g.p.mp,maxHP=g.p.maxhp,maxMP=g.p.maxmp;
  assert(finishCamp(g)&&g.p.hp==(food?maxHP:std::min<unsigned>(maxHP,beforeHP+maxHP/2))&&g.p.mp==(food?maxMP:std::min<unsigned>(maxMP,beforeMP+maxMP/2))&&g.rations==2-food&&g.sleepKit==kit);
  auto healedHP=g.p.hp,healedMP=g.p.mp;assert(!finishCamp(g)&&g.p.hp==healedHP&&g.p.mp==healedMP);g=checkpoint(g);
 }
 for(auto phase:{Phase::Lost,Phase::Fled}){auto g=testHero(0,12);g.p.hp=1;g.rations=1;assert(!startCamp(g,true,false));g.campRoll=1;assert(acceptCamp(g));if(phase==Phase::Lost){g.p.hp=0;finish(g);}else g.phase=Phase::Fled;g=checkpoint(g);assert(resolveCamp(g)&&!g.campStage&&g.p.hp==1&&g.rations==0);g=checkpoint(g);}
 auto kit=testHero(0,12);kit.p.gold=79;assert(buySleepKit(kit));kit.p.gold=80;assert(!buySleepKit(kit)&&kit.sleepKit&&!kit.p.gold);assert(buySleepKit(kit));kit=checkpoint(kit);
 for(unsigned cls=0;cls<4;++cls){bool seen[4]={};for(unsigned seed=1;seed<=256;++seed){auto g=testHero(cls,seed);g.city=1;g.crystals=1;assert(!enterDungeon(g));g.dungeonFlags=3;g.dungeonXY=0x27;g.dungeonLoot=8;g.dungeonEnemies=64;
   auto before=g.owned;auto gold=g.p.gold;assert(dungeonCollect(g)&&g.dungeonLoot&128);unsigned category=0;
   if(g.owned!=before){uint8_t id=0;for(uint8_t k=1;k<=GEAR_COUNT;++k)if(gearOwns(g.owned,k))id=k;assert(id&&gearFamily(id)!=cls&&gearTier(id)==2&&!g.equipped[0]);category=1+(gearFamily(id)+4-cls-1)%4;assert(category<4);}
   else assert(g.p.gold==gold+150);seen[category]=true;g=checkpoint(g);auto randomState=g.randomState;assert(!dungeonCollect(g)&&g.randomState==randomState);}
  for(bool outcome:seen)assert(outcome);
  auto g=testHero(cls,12);g.city=1;g.crystals=1;enterDungeon(g);g.dungeonFlags=3;g.dungeonXY=0x27;g.dungeonLoot=8;g.dungeonEnemies=64;
  for(unsigned other=0;other<4;++other)if(other!=cls)g.owned|=1u<<(gearOffer(other,1)-1);auto owned=g.owned;assert(dungeonCollect(g)&&g.owned==owned&&g.p.gold==150);g=checkpoint(g);
 }
 auto sale=testHero(0,3);sale.owned=1u<<(gearOffer(1,1)-1);auto price=sellPrice(sale,gearOffer(1,1));assert(!sellGear(sale,gearOffer(1,1))&&sale.p.gold==price&&!sale.owned);assert(sellGear(sale,gearOffer(1,1))&&sale.p.gold==price);sale=checkpoint(sale);
 sale.owned=1;sale.equipped[0]=1;assert(sellGear(sale,1)&&sale.owned==1);sale.p.gold=999999;sale.equipped[0]=0;assert(sellGear(sale,1)&&sale.owned==1&&sale.p.gold==999999);
 uint8_t bytes[SAVE_SIZE];auto legacy=testHero(0,17);encode(legacy,1,bytes);put16(bytes,4,9);put16(bytes,6,96);put32(bytes,92,crc(bytes,92));Game copy;uint32_t seq;assert(decode(bytes,copy,seq)==Decode::Ok&&!copy.sleepKit&&!copy.campStage);
 bytes[90]=16;put32(bytes,92,crc(bytes,92));assert(decode(bytes,copy,seq)==Decode::Corrupt);put16(bytes,4,10);put16(bytes,6,96);put32(bytes,92,crc(bytes,92));assert(decode(bytes,copy,seq)==Decode::Ok&&copy.sleepKit);bytes[90]=32;put32(bytes,92,crc(bytes,92));assert(decode(bytes,copy,seq)==Decode::Corrupt);put16(bytes,4,16);assert(decode(bytes,copy,seq)==Decode::Unsupported);
 puts("PASS: 1280 camps, D20 natural1/20, food/kit, save10/import9, post-battle rest, loss/flee interruption, healing once, 1024 random chests/all four outcomes/no duplicates, sales and caps");
 return 0;
}
