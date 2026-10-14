#include "SceneCanvas.h"
#include "../firmware/RPG_POKET_2/Persistence.h"
int main(int argc,char** argv){
 assert(argc==2);artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 unsigned frames=0;Canvas c;for(unsigned seed:{1u,7u,42u,65535u}){
  auto g=rpg::create(0,seed);g.city=1;g.crystals=1;g.tutorial=true;assert(!rpg::enterDungeon(g));
  for(unsigned f=0;f<2;++f){rpg::dungeonSetFloor(g,f);g.dungeonLoot=f?8:0;
   for(unsigned i=0;i<81;++i){if(rpg::dungeonWall(g,i%9,i/9)||rpg::dungeonEnemyAt(g,i%9,i/9)>=0)continue;g.dungeonXY=(i%9)|((i/9)<<4);
    for(unsigned d=0;d<4;++d){rpg::dungeonFace(g,d);auto before=g;ViewState v;v.page=Page::Dungeon;v.message="Explore e encontre o selo";render(c,g,v);assert(!memcmp(&g,&before,sizeof(g)));++frames;
      if(i==64&&d==1)c.save(std::string(argv[1])+"/ruins-seed-"+std::to_string(seed)+"-floor-"+std::to_string(f)+".ppm");}
   }
  }
 }
 free(artMemory);artMemory=nullptr;printf("PASS procedural6 render: %u real240x320 generated-map frames; four seeds/two floors/all safe cells/all headings, immutable Game/RNG\n",frames);
}
