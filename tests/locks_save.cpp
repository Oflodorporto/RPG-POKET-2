#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace rpg;
void checkpoint(Game& g){assert(valid(g));uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,17,b);Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&seq==17);encode(h,17,c);assert(!memcmp(b,c,SAVE_SIZE));g=h;}
int main(){unsigned success[2]={},locked=0,trapped=0;
 for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls)for(unsigned method=0;method<2;++method)for(unsigned seed=1;seed<=200;++seed){Game g=create(cls,seed*7919);g.city=city;g.discovery=2;g.discoverLoot=0;g.discoverAmount=15;g.chestLock=1;g.chestTrap=seed%2;g.gazuas=2;g.tutorial=true;checkpoint(g);auto hp=g.p.hp;
  assert(collectDiscovery(g));auto rng=g.randomState;assert(!attemptLock(g,method));assert(g.randomState!=rng&&g.chestTries==1&&g.gazuas==(method?1:2));checkpoint(g);
  if(g.chestLock==2){++success[method];assert(!collectDiscovery(g)&&g.p.gold==15&&!g.chestLock);auto gold=g.p.gold;assert(collectDiscovery(g)&&g.p.gold==gold);}
  else {assert(g.p.hp>=1&&g.p.hp<=hp);if(method){assert(g.chestLock==1);assert(!attemptLock(g,true)&&g.chestTries==2&&!g.gazuas);checkpoint(g);assert(attemptLock(g,true));}else {assert(g.chestLock==3);auto failed=g;assert(attemptLock(g,true)&&g.randomState==failed.randomState&&g.gazuas==failed.gazuas);}}
  clearDiscovery(g);checkpoint(g);
 }
 for(unsigned seed=1;seed<=50000;++seed){auto g=create(0,seed*7919);startDiscovery(g);checkpoint(g);if(g.chestLock){++locked;trapped+=g.chestTrap;assert(g.discovery==2&&g.discoverLoot!=7);}}
 assert(locked>3400&&locked<4200&&trapped>750&&trapped<1150);
 Game g=create(2,42);g.p.gold=100;assert(!buyGazua(g)&&g.gazuas==1&&g.p.gold==96);g.gazuas=9;assert(buyGazua(g));g.gazuas=0;g.discovery=2;g.discoverLoot=1;g.discoverAmount=1;g.chestLock=1;auto rng=g.randomState;assert(attemptLock(g,true)&&g.randomState==rng&&!g.chestTries);assert(buyGazua(g));clearDiscovery(g);
 // An actual save18 discovery imports unchanged and unlocked; new19 fields default0.
 g.discovery=2;g.discoverLoot=0;g.discoverAmount=9;g.scrap=3;uint8_t b[SAVE_SIZE];encode(g,1,b);put16(b,4,18);put32(b,124,crc(b,124));Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&h.discovery==2&&h.discoverAmount==9&&h.scrap==3&&!h.chestLock&&!h.gazuas);assert(!collectDiscovery(h)&&h.p.gold==g.p.gold+9);
 // Reject inconsistent packed state even with a repaired CRC.
 clearDiscovery(g);for(unsigned byte:{60u,61u,62u,63u}){encode(g,1,b);b[byte]=255;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);}
 g.discovery=2;g.discoverLoot=0;g.discoverAmount=1;g.chestLock=1;g.chestTrap=true;g.p.hp=1;g.gazuas=1;assert(!attemptLock(g,true)&&g.p.hp==1);checkpoint(g);
 printf("PASS: 6400 lock attempts/all classes/cities; D20/bonus, consumable, traps, jam, once-only loot, save19 packed resume, save18 compatibility, caps/corruption; %u locks/%u traps in50000 explorations\n",locked,trapped);
}
