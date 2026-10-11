#pragma once
#include "../firmware/RPG_POKET_2/Save.h"
namespace rpg {inline void legacyFormat(uint8_t* b,unsigned version){if(version<20){b[47]&=31;b[50]&=1;b[54]=0;}if(version<28){unsigned kind=(b[97]>>3)&7,progress=b[95]&7;b[96]&=7;if(version==27){b[97]=(b[97]&7)|(kind<<3)|(progress<<5);b[95]=0;}else {b[97]&=version>=26?7:3;b[94]=b[95]=0;}}put16(b,4,version);}}
