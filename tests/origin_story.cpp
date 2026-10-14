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
   if(page<8){unsigned lines=story::wrapStory(story::originPage(g,page).text,18,[](unsigned,const char* line){assert(strlen(line)<=18);for(const char* p=line;*p;++p)assert((unsigned char)*p<128);});assert(lines<=8);}
  }
 }
 // The common path carries every early clue, even without optional conversations.
 auto opening=rpg::create(0,51);opening.originStory=true;
 assert(story::originCount(opening)==8);
 const auto& arrival=story::originPage(opening,3);assert(arrival.npc==story::Npc::None&&strstr(arrival.text,"nome raspado")&&strstr(arrival.text,"sino toca na torre vazia")&&strstr(arrival.text,"carroca"));
 const auto& messenger=story::originPage(opening,5);assert(messenger.npc==story::Npc::Messenger&&strstr(messenger.text,"mesma cinza")&&strstr(messenger.text,"raizes")&&strstr(messenger.text,"farol"));
 const auto& letter=story::originPage(opening,6);assert(letter.npc==story::Npc::Maelis&&strstr(letter.text,"Iria")&&strstr(letter.text,"Vespera")&&strstr(letter.text,"Elarin e Borin ficam"));
 const auto& guidance=story::originPage(opening,7);assert(guidance.npc==story::Npc::None&&strstr(guidance.text,"pode ouvir")&&strstr(guidance.text,"Conversar"));
 for(unsigned page=0;page<8;++page){const char* text=story::originPage(opening,page).text;assert(!strstr(text,"Anwen")&&!strstr(text,"Vaelor")&&!strstr(text,"Odran")&&!strstr(text,"Pacto"));}
 uint8_t openingBefore[rpg::SAVE_SIZE],openingAfter[rpg::SAVE_SIZE];rpg::encode(opening,19,openingBefore);
 for(unsigned page=0;page<16;++page)story::originPage(opening,page);
 rpg::encode(opening,19,openingAfter);assert(!memcmp(openingBefore,openingAfter,sizeof(openingBefore)));
 auto g=rpg::create(2,51);g.tutorial=true;g.p.gold=313;g.shirt=4;g.crystals=2;g.rations=3;
 uint8_t b[rpg::SAVE_SIZE];rpg::encode(g,19,b);rpg::legacyFormat(b,16);rpg::put32(b,124,rpg::crc(b,124));rpg::Game h;uint32_t seq;
 assert(rpg::decode(b,h,seq)==rpg::Decode::Ok&&!h.originStory&&!h.originPage&&h.tutorial&&h.p.gold==313&&h.crystals==2&&h.rations==3&&h.shirt==4);
 assert(story::originCount(h)==5&&story::originPage(h,0).npc==story::Npc::None);
 for(unsigned page=0;page<5;++page){assert(!strcmp(story::originPage(h,page).text,story::originPage(opening,page+3).text));assert(story::wrapStory(story::originPage(h,page).text,18,[](unsigned,const char* line){assert(strlen(line)<=18);})<=8);}
 assert(story::originPage(h,99).npc==story::Npc::None&&!strcmp(story::originPage(h,99).text,guidance.text));
 for(unsigned byte:{122u,123u}){rpg::encode(g,19,b);b[byte]=byte==122?2:9;rpg::put32(b,124,rpg::crc(b,124));assert(rpg::decode(b,h,seq)==rpg::Decode::Corrupt);}
 rpg::encode(g,19,b);b[123]=1;rpg::put32(b,124,rpg::crc(b,124));assert(rpg::decode(b,h,seq)==rpg::Decode::Corrupt);
 for(unsigned i=0;i<17;++i){assert(story::npcs[i].name&&story::npcs[i].role);assert(425+i*2+1<ART_COUNT);const auto& meta=assetMeta[426+i*2];assert(meta.w==64&&meta.h==64&&meta.elem==1);}
 assert(story::speakerNpc("MAELIS VOSS / CARTA")==story::Npc::Maelis&&story::speakerNpc("SABELA / NOVA URGENCIA")==story::Npc::Sabela&&story::speakerNpc("PROLOGO")==story::Npc::None);
 puts("PASS: 4 classes/4 races; mandatory clues/optional advice, ASCII 18-column page fit, historical Save28 origin roundtrip, save16 preservation, corrupt origin rejection and 17 portrait bindings");
}
