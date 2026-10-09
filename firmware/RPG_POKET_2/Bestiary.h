#pragma once
#include "Rules.h"
namespace rpg {
inline bool bestiaryKnown(const Game& g,unsigned id){return id<20&&(g.seenEnemies&(1u<<id));}
inline unsigned bestiaryCount(const Game& g){unsigned n=0;for(unsigned i=0;i<20;++i)n+=bestiaryKnown(g,i);return n;}
inline uint8_t bestiaryFirst(const Game& g){for(unsigned i=0;i<20;++i)if(bestiaryKnown(g,i))return i;return 0;}
inline uint8_t bestiaryStep(const Game& g,unsigned id,int direction){for(unsigned i=0;i<20;++i){id=(id+20+(direction<0?-1:1))%20;if(bestiaryKnown(g,id))return id;}return 0;}
}
