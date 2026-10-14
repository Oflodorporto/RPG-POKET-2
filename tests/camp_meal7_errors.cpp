#include "SceneCanvas.h"
#include "../firmware/RPG_POKET_2/Persistence.h"
#include <filesystem>
std::vector<uint8_t> saved(const rpg::Game& g){std::vector<uint8_t> bytes(rpg::PERSISTENT_SAVE_SIZE);rpg::encodePersistent(g,7,0,bytes.data());return bytes;}
void readOnly(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame){auto before=saved(g);auto m=menu;auto state=v;render(c,g,v,frame);assert(saved(g)==before&&!memcmp(&m,&menu,sizeof(m))&&!memcmp(&state,&v,sizeof(v)));}
int main(){
 std::filesystem::create_directories("build/screens/camp-meal7-errors");std::vector<uint8_t> art(ART_BYTES);artMemory=art.data();std::ifstream f("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(f);f.seekg(16);f.read((char*)artMemory,ART_BYTES);assert(rpg::crc(artMemory,ART_BYTES)==ART_CRC);Canvas before,after,expected;unsigned cases=0;
 for(bool fallback:{false,true}){
  menu=MenuState{};menu.art=fallback?ArtStatus::Fallback:ArtStatus::Ready;if(fallback)makeFallback();
  for(unsigned city=0;city<4;++city)for(unsigned identity=0;identity<16;++identity)for(unsigned error=0;error<4;++error){
   auto g=rpg::create(identity%4,17);g.race=identity/4;g.city=city;g.shirt=2;g.trousers=3;g.p.hp=1;g.p.mp=0;g.rations=error?1:0;menu.campRation=error==0;menu.campKit=error==1;
   auto original=saved(g);const char* message;
   if(error<2)message=rpg::startCamp(g,error==0,error==1);
   else if(error==2){g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;original=saved(g);message=rpg::startCamp(g,true,false);}
   else message="Nao foi possivel preparar acampamento";
   assert(message&&saved(g)==original);
   if(error==0)assert(!strcmp(message,"Sem racoes"));if(error==1)assert(!strcmp(message,"Sem kit de dormir"));
   ViewState v;v.page=Page::CampSetup;readOnly(before,g,v,12);v.message=message;readOnly(after,g,v,12);
   expected.fillScreen(UI_INK);expected.setTextSize(1);expected.setTextColor(UI_RED);expected.setCursor((240-int(strlen(message))*6)/2,195);expected.print(message);
   for(int y=0;y<320;++y)for(int x=0;x<240;++x){
    if(x>=3&&x<237&&y>=194&&y<206)assert(after.pixels[y*240+x]==expected.pixels[y*240+x]);
    else assert(after.pixels[y*240+x]==before.pixels[y*240+x]);
   }
   if(!fallback&&city==1&&identity==0)after.save("build/screens/camp-meal7-errors/setup-error-"+std::to_string(error)+".ppm");++cases;
  }
  // A failed save remains on the existing protected screen. Rendering a paid
  // pending camp cannot eat, reroll, retry the save, or debit another ration.
  auto g=rpg::create(0,17);g.city=1;g.p.hp=1;g.rations=2;assert(!rpg::startCamp(g,true,false));assert(g.rations==1&&g.campStage==1);ViewState v;v.page=Page::SaveError;v.message="Falha ao salvar";menu.campRation=true;readOnly(before,g,v,0);
  for(unsigned frame:{1u,12u,23u,48u}){menu.campRation=!menu.campRation;readOnly(after,g,v,frame);assert(!memcmp(before.pixels,after.pixels,sizeof(before.pixels)));assert(g.rations==1&&g.campStage==1);++cases;}
  if(!fallback)after.save("build/screens/camp-meal7-errors/save-error.ppm");
 }
 artMemory=nullptr;printf("PASS: %u missing-ration/kit/full-HP/long-error/save-error render cases; exact legible error line, every other pixel unchanged (hero/fire/bedroll/controls), both art modes, immutable save/RNG/debit/error semantics.\n",cases);
}
