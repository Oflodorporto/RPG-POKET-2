#include "LegacySave.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/OriginStory.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race){auto g=rpg::create(cls,51);g.originStory=true;g.race=race;
  for(unsigned page=0;page<=8;++page){g.originPage=page;assert(rpg::valid(g));uint8_t b[rpg::SAVE_SIZE];rpg::encode(g,19,b);rpg::Game h;uint32_t seq;
   assert(rpg::decode(b,h,seq)==rpg::Decode::Ok&&h.originStory&&h.originPage==page&&h.race==race&&h.p.cls==cls);
   if(page<8){unsigned lines=story::wrapStory(story::originPage(g,page).text,18,[](unsigned,const char* line){assert(strlen(line)<=18);});assert(lines<=9);}
  }
 }
 auto g=rpg::create(2,51);g.tutorial=true;g.p.gold=313;g.shirt=4;g.crystals=2;g.rations=3;
 uint8_t b[rpg::SAVE_SIZE];rpg::encode(g,19,b);rpg::legacyFormat(b,16);rpg::put32(b,124,rpg::crc(b,124));rpg::Game h;uint32_t seq;
 assert(rpg::decode(b,h,seq)==rpg::Decode::Ok&&!h.originStory&&!h.originPage&&h.tutorial&&h.p.gold==313&&h.crystals==2&&h.rations==3&&h.shirt==4);
 assert(story::originCount(h)==5&&story::originPage(h,0).npc==story::Npc::None);
 for(unsigned byte:{122u,123u}){rpg::encode(g,19,b);b[byte]=byte==122?2:9;rpg::put32(b,124,rpg::crc(b,124));assert(rpg::decode(b,h,seq)==rpg::Decode::Corrupt);}
 rpg::encode(g,19,b);b[123]=1;rpg::put32(b,124,rpg::crc(b,124));assert(rpg::decode(b,h,seq)==rpg::Decode::Corrupt);
 for(unsigned i=0;i<17;++i){assert(story::npcs[i].name&&story::npcs[i].role);assert(425+i*2+1<ART_COUNT);const auto& meta=assetMeta[426+i*2];assert(meta.w==64&&meta.h==64&&meta.elem==1);}
 assert(story::speakerNpc("MAELIS VOSS / CARTA")==story::Npc::Maelis&&story::speakerNpc("SABELA / NOVA URGENCIA")==story::Npc::Sabela&&story::speakerNpc("PROLOGO")==story::Npc::None);
 puts("PASS: 4 classes/4 races; page fit, save17 roundtrip, save16 preservation, corrupt origin rejection and 17 portrait bindings");
}
