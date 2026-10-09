#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/EnemyInfo.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace rpg;
int main(){unsigned n=0;for(unsigned cls=0;cls<4;++cls)for(unsigned id=0;id<18;++id)for(unsigned beat=0;beat<3;++beat){auto g=create(cls,43);g.enemyId=id;g.enemyHp=encounterHp(g,id);g.phase=Phase::Hero;g.enemyBeat=beat;uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,2,b);assert(canInspect(g));for(unsigned line=0;line<3;++line)assert(strlen(enemyAdvice(g,line))<=36);auto bound=incomingCeiling(g,g.guard);auto copy=g;copy.phase=Phase::Enemy;enemy(copy);assert(copy.damage<=bound);encode(g,2,c);assert(!memcmp(b,c,sizeof(b)));g.phase=Phase::Enemy;assert(!canInspect(g));g.phase=Phase::Home;assert(!canInspect(g));++n;}puts("PASS: enemy info216 encounters, live ceilings/advice/class/type; read-only; turn gate.");}
