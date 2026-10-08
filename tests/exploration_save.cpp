#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace rpg;
void roundtrip(Game& g){assert(valid(g));uint8_t bytes[SAVE_SIZE];encode(g,9,bytes);Game h;uint32_t seq;assert(decode(bytes,h,seq)==Decode::Ok&&seq==9);uint8_t after[SAVE_SIZE];encode(h,9,after);assert(!memcmp(bytes,after,SAVE_SIZE));g=h;}
int main(){
 unsigned kinds[4]={},mimics=0,chests=0;unsigned long gold=0;
 for(unsigned i=1;i<=100000;++i){Game g=create(i%4,i*7919);g.city=i%4;g.tutorial=true;g.p.level=20;g.p.maxmp=totalMana(g);g.p.mp=g.p.maxmp;
  assert(startDiscovery(g,i%2));roundtrip(g);
  if(!g.discovery){++kinds[0];continue;}
  ++kinds[g.discovery];unsigned rng=g.randomState;auto before=g;
  assert(!startDiscovery(g));assert(g.randomState==rng);assert(!eventSafe(g));
  if(g.discovery==2){++chests;if(g.discoverLoot==7)++mimics;}
  assert(!collectDiscovery(g));roundtrip(g);
  if(g.discovery==4){assert(g.enemyId==14+g.city);g.enemyHp=0;finish(g);roundtrip(g);auto rewarded=g;finish(g);assert(g.p.gold==rewarded.p.gold&&g.p.xp==rewarded.p.xp);assert(home(g));assert(!g.discovery);roundtrip(g);}
  else {gold+=g.p.gold;auto collected=g;assert(collectDiscovery(g));assert(g.p.gold==collected.p.gold&&g.scrap==collected.scrap);clearDiscovery(g);roundtrip(g);}
  assert(before.randomState==rng);
 }
 assert(kinds[0]>49000&&kinds[0]<51000);assert(kinds[1]>19000&&kinds[1]<21000);assert(kinds[2]>19000&&kinds[2]<21000);assert(kinds[3]>9000&&kinds[3]<11000);assert(mimics>700&&mimics<1300);
 Game g=create(0,1);g.scrap=9;assert(!sellScrap(g));assert(g.p.gold==18&&!g.scrap);g.scrap=3;assert(!sellScrap(g,true)&&!g.scrap&&g.p.gold==18);
 g.discovery=2;g.discoverLoot=4;g.discoverAmount=1;g.scrap=9;auto rng=g.randomState;assert(collectDiscovery(g));assert(g.discovery==2&&g.randomState==rng);clearDiscovery(g);
 g.discovery=1;g.discoverLoot=6;g.discoverAmount=1;g.owned=1;assert(!collectDiscovery(g)&&g.discovery==5&&g.discoverLoot==0&&g.p.gold==24);
 clearDiscovery(g);uint8_t b[SAVE_SIZE];encode(g,1,b);put16(b,4,17);memset(b+60,0,4);put32(b,124,crc(b,124));Game old;uint32_t seq;assert(decode(b,old,seq)==Decode::Ok&&!old.discovery&&!old.scrap&&old.p.gold==g.p.gold);
 for(unsigned byte=60;byte<=63;++byte){encode(old,1,b);b[byte]=255;put32(b,124,crc(b,124));assert(decode(b,g,seq)==Decode::Corrupt);}
 for(unsigned city=0;city<4;++city)for(auto outcome:{Phase::Won,Phase::Lost,Phase::Fled}){g=create(0,42);g.city=city;g.discovery=2;g.discoverLoot=7;g.discoverAmount=1;assert(!collectDiscovery(g));g.phase=outcome;g.enemyHp=outcome==Phase::Won?0:1;g.p.hp=outcome==Phase::Lost?0:1;roundtrip(g);assert(home(g));roundtrip(g);assert(!g.discovery);}
 printf("PASS: 100000 discoveries [%u,%u,%u,%u], %u/%u mimic chests; save18 resume, once-only rewards, limits, migration, sale/discard and all combat outcomes. Gold/find average %.2f\n",kinds[0],kinds[1],kinds[2],kinds[3],mimics,chests,double(gold)/(100000-kinds[0]));
}
