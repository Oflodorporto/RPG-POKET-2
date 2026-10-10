#pragma once
#include "../firmware/RPG_POKET_2/GuildEvents.h"
namespace rpg {
// Regression fixture for letters written before kinds/scaled rewards existed.
inline bool offerLegacyEvent(Game& g,uint32_t day,unsigned hour){if(!offerEvent(g,day,hour))return false;g.eventKind=g.eventProgress=g.eventLevel=0;return true;}
}
