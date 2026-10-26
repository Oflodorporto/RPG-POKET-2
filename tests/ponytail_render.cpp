#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main inheritedRenderChecks
#ifdef PONYTAIL_BASELINE
#include "../../repository/tests/render.cpp"
#include "../../repository/firmware/RPG_POKET_2/Persistence.h"
#else
#include "render.cpp"
#include "../firmware/RPG_POKET_2/Persistence.h"
#endif
#undef main
#pragma GCC diagnostic pop
#ifndef PONYTAIL_BASELINE
#include "../firmware/RPG_POKET_2/Coop/CoopView.h"
#endif
struct Labels:Canvas {
 std::vector<std::string> labels;
 void print(const char* s){labels.emplace_back(s);Canvas::print(s);}
 bool has(const char* s)const{for(const auto& label:labels)if(label==s)return true;return false;}
};
int main(int argc,char** argv){
 assert(argc==2);std::string out=argv[1];artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);
 std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 Labels canvas;unsigned frames=0;
 for(bool modern:{false,true})for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls)for(bool full:{false,true}){
  auto g=modern?rpg::create(cls,42+slot):rpg::testHero(cls,42+slot);
  if(full){g.owned=(1u<<rpg::GEAR_COUNT)-1;rpg::equipGear(g,cls*3+1);rpg::equipGear(g,13);rpg::equipGear(g,16);}
  ViewState v;v.page=Page::BagGear;unsigned pages=full?3:1;
  for(unsigned page=0;page<pages;++page)for(unsigned choice=0;choice<6;++choice){
   v.gearIndex=page;v.choice=choice;uint8_t before[160],after[160];auto original=g;rpg::encodePersistent(g,9,slot,before);
   canvas.labels.clear();render(canvas,g,v);rpg::encodePersistent(g,9,slot,after);assert(!memcmp(before,after,160)&&!memcmp(&g,&original,sizeof(g)));++frames;
   assert(canvas.has("Equipar")&&canvas.has("Vender")&&canvas.has("Voltar"));
#ifndef PONYTAIL_BASELINE
   if(full){assert(canvas.has(page<2?"T1 +2":"T1 +3"));assert(canvas.has(page<2?"T2 +4":"T2 +6"));assert(canvas.has(page<2?"T3 +6":"T3 +9"));}
#endif
   if(modern&&!slot&&!cls&&full&&!choice)canvas.save(out+"/equipment-page-"+std::to_string(page+1)+".ppm");
  }
 }
#ifndef PONYTAIL_BASELINE
 struct Portrait:Canvas{void drawPixel(int x,int y,uint16_t color){pixel(x,y,color);}} physical;
 struct Probe:coopplatform::LandscapeCanvas<Portrait>{using coopplatform::LandscapeCanvas<Portrait>::LandscapeCanvas;
  void print(const char* s){assert(getCursorX()>=0&&getCursorY()>=0&&getCursorX()+int(strlen(s))*6<=320&&getCursorY()+8<=240);coopplatform::LandscapeCanvas<Portrait>::print(s);}
 } landscape(physical);coopview::ViewModel v;v.cleanupPending=true;coopplatform::Training t;
 auto before=t.endpoint.state;coopview::draw(landscape,v,t);assert(!memcmp(&before,&t.endpoint.state,sizeof(before)));physical.save(out+"/cleanup-portrait.ppm");
 assert(coopview::hitTest(v,160,200)==coopview::Control::Exit&&coopview::hitTest(v,280,10)==coopview::Control::None&&coopview::hitTest(v,214,10)==coopview::Control::None);
 auto clubGame=rpg::testHero(0,42);auto unchangedClub=clubGame;ViewState clubView;clubView.page=Page::Club;arena.cleanupPending=true;
 canvas.labels.clear();render(canvas,clubGame,clubView);assert(!memcmp(&unchangedClub,&clubGame,sizeof(clubGame)));
 assert(canvas.has("RADIO EM RECUPERACAO")&&canvas.has("Tentar novamente"));canvas.save(out+"/pvp-cleanup.ppm");arena.cleanupPending=false;
 for(unsigned f=0;f<6;++f){unsigned opaque=0;for(unsigned i=0;i<1024;++i)opaque+=(equipmentArt::alpha[f][i/8]>>(i%8))&1;assert(opaque>0&&opaque<1024);}
#endif
 free(artMemory);artMemory=nullptr;printf("PASS: %u equipment frames, 18 items/tiers, empty/full, three slots, old/new four classes, all pages/selections, controls and unchanged Save30/Game; cleanup render/hit map\n",frames);
}
