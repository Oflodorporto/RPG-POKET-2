#pragma once
#include <stdint.h>
namespace rpg {
// Catalog, prices and level requirements adapted from the frozen Heltec Gear.h.
constexpr uint8_t GEAR_COUNT=18;
inline bool gearId(uint8_t id){return id>=1&&id<=GEAR_COUNT;}
inline uint8_t gearFamily(uint8_t id){return gearId(id)?(id-1)/3:255;}
inline uint8_t gearTier(uint8_t id){return gearId(id)?(id-1)%3+1:0;}
inline uint8_t gearSlot(uint8_t id){uint8_t f=gearFamily(id);return f<4?0:f<6?f-3:255;}
inline uint8_t gearLevel(uint8_t id){uint8_t t=gearTier(id);return t==1?1:t==2?4:t==3?8:255;}
inline uint16_t gearPrice(uint8_t id){return 30*gearTier(id)*gearTier(id);}
inline uint8_t gearBonus(uint8_t id){return gearTier(id)*(gearSlot(id)==0?2:3);}
inline bool gearAllowed(uint8_t id,uint8_t cls,uint8_t level){return gearId(id)&&cls<4&&(gearFamily(id)>=4||gearFamily(id)==cls)&&level>=gearLevel(id);}
inline bool gearOwns(uint32_t owned,uint8_t id){return gearId(id)&&(owned&(1u<<(id-1)));}
inline const char* gearName(uint8_t id){static const char* n[]={
  "CAJADO DE CARVALHO","CAJADO DO TROVAO","CAJADO ARCANO",
  "ESPADA DE FERRO","ESPADA DA GUARDA","ESPADA SOLAR",
  "LANCA DE FERRO","LANCA DO VENTO","LANCA REAL",
  "MACHADO DE FERRO","MACHADO FEROZ","MACHADO TITAN",
  "COLETE DE COURO","COTA DE MALHA","ARMADURA REAL",
  "AMULETO SIMPLES","AMULETO LUNAR","AMULETO ASTRAL"};return gearId(id)?n[id-1]:"VAZIO";}
inline const char* gearStat(uint8_t id){return gearSlot(id)==0?"ATAQUE":gearSlot(id)==1?"DEFESA":"MP MAX";}
inline uint8_t gearOffer(uint8_t cls,uint8_t index){return cls<4&&index<9?(index<3?cls*3+index+1: index+10):0;}
inline uint8_t gearOwnedCount(uint32_t owned){uint8_t n=0;for(uint8_t id=1;id<=GEAR_COUNT;++id)if(gearOwns(owned,id))++n;return n;}
inline uint8_t gearOwnedAt(uint32_t owned,uint8_t index){for(uint8_t id=1;id<=GEAR_COUNT;++id)if(gearOwns(owned,id)&&index--==0)return id;return 0;}
}
