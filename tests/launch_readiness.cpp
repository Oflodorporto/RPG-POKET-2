#include "../firmware/RPG_POKET_2/Save.h"
#include "TestHero.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace rpg;
Game boot(const Game& g){assert(valid(g));uint8_t b[SAVE_SIZE];encode(g,1,b);Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok);return h;}
int main(){
 // Preparation and healing are deterministic, and advance only when the enemy actually acts.
 for(unsigned cls=0;cls<4;++cls)for(unsigned id=0;id<14;++id){if(id>=10)continue;auto g=create(cls,17);g.ruinsWins=3;g.p.hp=g.p.maxhp=1000;begin(g,id);for(unsigned round=0;round<9;++round){auto intent=enemyIntent(g);auto before=g.randomState;auto hp=g.p.hp;unsigned bound=incomingCeiling(g,75);g.guard=75;g.phase=Phase::Enemy;g=boot(g);assert(enemyIntent(g)==intent);assert(enemy(g)&&g.enemyBeat==(round+1)%3&&g.damage<=bound);if(intent==Intent::Prepare||intent==Intent::Mend){assert(g.p.hp==hp&&g.randomState==before&&!g.damage&&!g.guard);}g=boot(g);}}
 for(unsigned seed=1;seed<500;++seed)for(unsigned id:{1u,3u,5u,7u,8u})for(unsigned beat=0;beat<3;++beat)for(unsigned guard:{0u,50u,75u}){auto g=create(3,seed);g.ruinsWins=3;g.p.hp=g.p.maxhp=1000;begin(g,id);g.enemyBeat=beat;g.phase=Phase::Enemy;g.guard=guard;g.rageSpent=1;g.rageTurns=3;auto before=g.randomState;unsigned bound=incomingCeiling(g,guard);assert(g.randomState==before);enemy(g);assert(g.damage<=bound);}
 auto g=create(0,47);g.p.hp=g.p.maxhp=100;begin(g,5);g.enemyBeat=2;g.phase=Phase::Enemy;auto mp=g.p.mp;g.guard=75;enemy(g);assert(g.p.mp==mp);g.phase=Phase::Enemy;g.enemyBeat=2;g.guard=0;g.randomState=47;enemy(g);assert(g.dodge||g.p.mp==mp-2);
 g=create(2,47);begin(g,2);g.enemyBeat=1;g.enemyHp=5;g.phase=Phase::Enemy;enemy(g);assert(g.enemyHp==9);g=boot(g);
 auto old=testHero(2,47);begin(old,3);for(unsigned i=0;i<5;++i){old.phase=Phase::Enemy;old.p.hp=old.p.maxhp=100;enemy(old);assert(!old.enemyBeat&&enemyIntent(old)==Intent::Strike);}old=boot(old);
 // D20 preview includes the exact supplies and natural1/20, with no spending or RNG.
 for(unsigned city=0;city<4;++city)for(unsigned dest=0;dest<4;++dest)for(unsigned bits=0;bits<8;++bits){if(city==dest)continue;g=create(0,47);g.city=city;g.rations=bits&1;g.charts=(bits>>1)&1;g.charms=(bits>>2)&1;auto before=g;unsigned chance=tripSafety(g,dest);assert(chance>=5&&chance<=95&&!memcmp(&before,&g,sizeof(g)));assert(!prepareTrip(g,dest));unsigned safe=0;for(unsigned die=1;die<=20;++die){g.tripRoll=die;g.tripTotal=die+g.tripSurvival+g.tripLuck;safe+=tripSafe(g);}assert(safe*5==chance);}
 g=create(2,47);g.p.level=4;g.p.maxmp=totalMana(g);g.p.mp=g.p.maxmp;g.owned=1u<<6;g.equipped[0]=7;auto before=g;auto preview=gearPreview(g,8);assert(effectiveAttack(preview)==effectiveAttack(g)+2&&!memcmp(&g,&before,sizeof(g))&&!gearOwns(g.owned,8));assert(gearPreview(g,2).equipped[0]==7);
 // Save14 imports without enemy cadence; future/reserved bytes remain protected.
 uint8_t b[SAVE_SIZE];g=create(2,47);encode(g,1,b);put16(b,4,14);put32(b,124,crc(b,124));Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&!h.enemyBeat);h=boot(h);
 encode(g,1,b);b[119]=3;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);encode(g,1,b);b[122]=2;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);
 puts("PASS: intentions/cadence/restart, windup/healing/drain, deterministic danger bounds, legacy battles, exact read-only D20 chance, equipment comparison and save15/read14.");
}
