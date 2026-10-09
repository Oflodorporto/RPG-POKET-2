#pragma once
#include "../firmware/RPG_POKET_2/Save.h"
namespace rpg {inline void legacyFormat(uint8_t* b,unsigned version){if(version<20){b[47]&=31;b[50]&=1;b[54]=0;}put16(b,4,version);}}
