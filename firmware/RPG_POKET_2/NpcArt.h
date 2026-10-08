#pragma once
#include "AssetCatalog.h"
#include <string.h>
namespace story {
enum class Npc:uint8_t {Nara,Maelis,Borin,Elarin,Grum,Iria,Caelen,Vaelor,Sabela,Tomas,Nilsa,Seraphine,Dargan,Liora,Odran,Anwen,Messenger,None=255};
struct NpcInfo {const char* name;const char* role;};
constexpr NpcInfo npcs[]={
 {"Nara Veld","Dona do Refugio"},{"Maelis Voss","Mestra da Guilda"},{"Borin Caldaferrea","Ferreiro"},{"Elarin Folhacinza","Boticario"},
 {"Grum Pedrafranca","Arbitro e treinador"},{"Iria Sorel","Arqueologa"},{"Caelen Vesper","Intendente"},{"Arconte Vaelor","Eco da Cripta"},
 {"Sabela Marebrava","Capita do porto"},{"Tomas Valevento","Mercador"},{"Nilsa Bronzamar","Construtora"},{"Seraphine Alvor","Guardia dos juramentos"},
 {"Dargan Setemartelos","Ferreiro real"},{"Liora Valcer","Aliada em Aurora"},{"Odran Valcer","Regente"},{"Anwen","Testemunha apagada"},{"O mensageiro","Correio da Guilda"}
};
inline Npc cityNpc(unsigned city,unsigned person){constexpr Npc ids[4][3]={{Npc::Nara,Npc::Elarin,Npc::Borin},{Npc::Iria,Npc::Caelen,Npc::Vaelor},{Npc::Sabela,Npc::Tomas,Npc::Nilsa},{Npc::Liora,Npc::Seraphine,Npc::Dargan}};return ids[city%4][person%3];}
inline Npc speakerNpc(const char* name){
 const char* prefixes[]={"NARA","MAELIS","BORIN","ELARIN","GRUM","IRIA","CAELEN","ARCONTE","SABELA","TOMAS","NILSA","SERAPHINE","DARGAN","LIORA","ODRAN","ANWEN","O MENSAGEIRO"};
 for(unsigned i=0;i<17;++i)if(!strncmp(name,prefixes[i],strlen(prefixes[i])))return Npc(i);return Npc::None;
}
}
// Art is validated and cached in PSRAM at boot; drawing never accesses SD.
template<class Canvas> void npcPortrait(Canvas& c,story::Npc npc,int x,int y,unsigned size=64){
 if(npc==story::Npc::None||unsigned(npc)>=17)return;
 unsigned id=425+unsigned(npc)*2;const auto* pal=reinterpret_cast<const uint16_t*>(assetBytes(id));const auto* pixels=assetBytes(id+1);uint16_t line[64];size=std::min(64u,size);
 for(unsigned row=0;row<size;++row){for(unsigned col=0;col<size;++col)line[col]=pal[pixels[(row*64/size)*64+col*64/size]];c.draw16bitRGBBitmap(x,y+row,line,size,1);}
 c.drawRect(x-1,y-1,size+2,size+2,0xd5aa);
}
