#define main included_presentation_main
#include "pvp_presentation_render.cpp"
#undef main
#include "../firmware/RPG_POKET_2/Persistence.h"
int main(int argc,char** argv){
 assert(argc==3);artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));std::ifstream f(argv[1],std::ios::binary);assert(f);f.seekg(16);f.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(f.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);menu=MenuState{};menu.art=ArtStatus::Ready;
 Probe c;unsigned cases=0;for(unsigned actor=0;actor<2;++actor)for(unsigned missing: {0u,1u,2u,7u,30u}){
  Net n(3,3,20);if(actor)n.play(club::Action::Defensive);
  for(auto& d:n.duel)d.fighters[actor].hp-=missing;
  const unsigned expected=std::min<unsigned>(missing,n.duel[0].fighters[actor].maxHP/4);
  n.play(club::Action::Defensive);assert(n.duel[0].lastHealing==expected&&n.duel[1].lastHealing==expected&&n.visual[0].actor==actor&&n.visual[1].actor==actor);
  for(unsigned side=0;side<2;++side){n.install(side);arena.visual.tick(n.now+600);uint8_t before[rpg::PERSISTENT_SAVE_SIZE],after[rpg::PERSISTENT_SAVE_SIZE];rpg::encodePersistent(n.games[side],42,0,before);auto random=n.games[side].randomState;draw(c,n.games[side]);rpg::encodePersistent(n.games[side],42,0,after);assert(!memcmp(before,after,sizeof(before))&&random==n.games[side].randomState&&rpg::get16(after,4)==30&&rpg::get32(after,144)==1);
   char label[30];if(expected)snprintf(label,sizeof(label),"+%u HP",expected);else strcpy(label,"GUARDA");assert(c.has(label)&&!c.has("+0 HP"));for(unsigned i=0;i<c.count;++i)if(!strcmp(c.labels[i].text,label))assert((c.labels[i].x<120)==(actor==side));
   for(unsigned fighter=0;fighter<2;++fighter){auto& who=n.duel[side].fighters[fighter];snprintf(label,sizeof(label),"HP %u/%u",who.hp,who.maxHP);assert(c.has(label));snprintf(label,sizeof(label),"MP %u/%u",who.mp,who.maxMP);assert(c.has(label));}
   for(unsigned i=0;i<c.count;++i){auto& lab=c.labels[i];assert(lab.x>=0&&lab.x+int(strlen(lab.text))*6*lab.size<=240&&lab.y>=0&&lab.y+8*lab.size<=320);}
   ++cases;if(missing==1)c.save(std::string(argv[2])+"/independent-heal-actor-"+std::to_string(actor)+"-side-"+std::to_string(side)+".ppm");
  }
 }
 printf("PASS: %u independent actor/side/capped-heal render cases; correct number+side, symmetric HP/MP, exact Save30/epoch1 bytes and RNG unchanged, labels bounded, zero wrapped allocations\n",cases);free(artMemory);artMemory=nullptr;
}
