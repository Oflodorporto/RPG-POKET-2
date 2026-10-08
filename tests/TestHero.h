#pragma once
#include "../firmware/RPG_POKET_2/Rules.h"
namespace rpg {
inline Game testHero(uint8_t cls,uint32_t seed){auto g=create(cls,seed);g.dndProgression=false;g.p.level=3;const unsigned hp[]={18,22,20,24};g.p.hp=g.p.maxhp=hp[g.p.cls];g.p.mp=g.p.maxmp=totalMana(g);g.p.life=0;g.rations=0;return g;}
}
