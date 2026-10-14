#include "SceneCanvas.h"
#include "../firmware/RPG_POKET_2/Persistence.h"
#include <filesystem>
#include <new>
#include <set>
#include <chrono>

static bool drawing=false;
extern "C" void* __real_malloc(size_t);
extern "C" void* __real_calloc(size_t,size_t);
extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){assert(!drawing);return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t s){assert(!drawing);return __real_calloc(n,s);}
extern "C" void* __wrap_realloc(void* p,size_t n){assert(!drawing);return __real_realloc(p,n);}
void* operator new(size_t n){assert(!drawing);if(auto p=malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{free(p);}void operator delete(void* p,size_t)noexcept{free(p);}
namespace {
constexpr uint16_t sentinel=0x1234;
struct HeroCanvas {
 uint16_t pixels[84*96];int width,height;
 void clear(int w,int h){width=w;height=h;std::fill(std::begin(pixels),std::end(pixels),sentinel);}
 void fillRect(int x,int y,int w,int h,uint16_t color){assert(x>=0&&y>=0&&x+w<=width&&y+h<=height);for(int dy=0;dy<h;++dy)for(int dx=0;dx<w;++dx)pixels[(y+dy)*width+x+dx]=color;}
};
struct Envelope {int x,y,w,h;};
Envelope envelope(Page p){return p==Page::CampSetup?Envelope{46,102,78,78}:Envelope{54,138,84,96};}
uint64_t hash(const HeroCanvas& c){uint64_t h=1469598103934665603ULL;for(int i=0;i<c.width*c.height;++i){h^=c.pixels[i];h*=1099511628211ULL;}return h;}
void immutable(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 auto before=g;auto beforeMenu=menu;auto beforeView=v;uint8_t a[rpg::PERSISTENT_SAVE_SIZE],b[rpg::PERSISTENT_SAVE_SIZE];
 rpg::encodePersistent(g,73,0,a);drawing=true;render(c,g,v,frame);drawing=false;rpg::encodePersistent(g,73,0,b);
 assert(!memcmp(&before,&g,sizeof(g))&&!memcmp(&beforeMenu,&menu,sizeof(menu))&&!memcmp(&beforeView,&v,sizeof(v))&&!memcmp(a,b,sizeof(a)));
}
void exactPlaque(const Canvas& actual,const char* name){
 char text[19];snprintf(text,sizeof(text),"%s",name);if(strlen(name)>18)text[15]=text[16]=text[17]='.';
 Canvas expected;expected.fillScreen(0x0843);expected.drawRect(60,62,120,17,UI_GOLD);int x=64+(112-int(strlen(text))*6)/2;
 expected.setTextSize(1);expected.setTextColor(UI_INK);expected.setCursor(x,67);expected.print(text);expected.setTextColor(UI_GOLD);expected.setCursor(x,66);expected.print(text);
 for(int y=62;y<79;++y)for(int x=60;x<180;++x)assert(actual.pixels[y*240+x]==expected.pixels[y*240+x]);
}
void exactHero(const Canvas& actual,const rpg::Game& g,Page page,bool meal,unsigned frame){
 HeroCanvas hero;auto e=envelope(page);hero.clear(e.w,e.h);drawing=true;campUi::hero(hero,g,0,0,e.w,e.h,meal,frame);drawing=false;
 unsigned shown=0;for(int y=0;y<e.h;++y)for(int x=0;x<e.w;++x)if(hero.pixels[y*e.w+x]!=sentinel){assert(actual.pixels[(e.y+y)*240+e.x+x]==hero.pixels[y*e.w+x]);++shown;}assert(shown>150);
}
}
int main(int argc,char** argv){
 const std::string root=argc>1?argv[1]:"build/screens/camp-identity77";std::filesystem::create_directories(root);
 std::vector<uint8_t> data(ART_BYTES+128,0xa5);artMemory=data.data()+64;std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 unsigned rasters=0,scenes=0,headPixels=0,colorCases=0,animationChecks=0;Canvas actual,base;HeroCanvas hero;std::set<uint64_t> identities[2];
 for(bool fallback:{false,true}){
  menu=MenuState{};menu.art=fallback?ArtStatus::Fallback:ArtStatus::Ready;if(fallback)makeFallback();
  for(unsigned race=0;race<4;++race)for(unsigned cls=0;cls<4;++cls){
   auto g=rpg::create(cls,42);g.race=race;const auto& rig=campUi::rigs[race][cls];
   // Exact, independent source-head oracle. Face/race/hair/clothing identity
   // comes from the original idle artwork, not a generic camp replacement.
   int dx=49-rig.cx,dy=84-rig.hip;
   for(int sy=rig.top;sy<std::min(rig.neck,rig.armY-7);++sy)for(int sx=rig.left;sx<rig.right;++sx){
    auto source=personalPixel(g,0,sy*112+sx);if(source==campUi::key)continue;
    assert(campUi::seatedPixel(g,false,0,sx+dx,sy+dy)==source);++headPixels;
   }
   for(unsigned shirt=0;shirt<8;++shirt)for(unsigned pants=0;pants<8;++pants)for(auto page:{Page::CampSetup,Page::CampRest}){
    g.shirt=shirt;g.trousers=pants;auto e=envelope(page);
    for(bool meal:{false,true}){hero.clear(e.w,e.h);drawing=true;campUi::hero(hero,g,0,0,e.w,e.h,meal,12);drawing=false;++rasters;
     unsigned visible=0;for(int i=0;i<e.w*e.h;++i)visible+=hero.pixels[i]!=sentinel;assert(visible>150);
     if(!fallback&&shirt==2&&pants==3&&meal)identities[page==Page::CampRest].insert(hash(hero));
    }
   }
   // Both recolor channels remain visible in every ready identity. Changing
   // an unrelated equipment/stat field cannot accidentally recolor the hero.
   for(auto page:{Page::CampSetup,Page::CampRest}){
    auto e=envelope(page);g.shirt=g.trousers=0;hero.clear(e.w,e.h);campUi::hero(hero,g,0,0,e.w,e.h,true,0);auto original=hero;
    for(unsigned channel:{1u,2u}){g.shirt=channel==1?2:0;g.trousers=channel==2?3:0;hero.clear(e.w,e.h);campUi::hero(hero,g,0,0,e.w,e.h,true,0);
     if(!fallback)assert(hash(hero)!=hash(original));
     ++colorCases;
    }
    g.shirt=2;g.trousers=3;hero.clear(e.w,e.h);campUi::hero(hero,g,0,0,e.w,e.h,true,0);auto first=hero;
    std::set<uint64_t> poses;for(unsigned frame=0;frame<24;++frame){hero.clear(e.w,e.h);drawing=true;campUi::hero(hero,g,0,0,e.w,e.h,true,frame);drawing=false;poses.insert(hash(hero));++animationChecks;}
    assert(poses.size()>=8);
    hero.clear(e.w,e.h);campUi::hero(hero,g,0,0,e.w,e.h,false,0);auto noFood=hero;campUi::hero(hero,g,0,0,e.w,e.h,false,12);assert(hash(hero)==hash(noFood)&&hash(first)!=hash(noFood));
   }
  }
  // Complete production scenes across all races, classes, food/kit choices,
  // location contexts, and both art modes. Drawing cannot touch a saved byte.
  for(unsigned city=0;city<4;++city)for(unsigned identity=0;identity<16;++identity)for(bool food:{false,true})for(bool kit:{false,true})for(auto page:{Page::CampSetup,Page::CampRest}){
   auto g=rpg::create(identity%4,42);g.race=identity/4;g.city=city;g.shirt=2+g.race;g.trousers=1+g.p.cls;g.rations=6;g.sleepKit=true;g.campStage=page==Page::CampRest?3:0;g.campRation=food;g.campKit=kit;
   menu.campRation=food;menu.campKit=kit;ViewState v;v.page=page;v.campProgress=500;immutable(actual,g,v,12);++scenes;exactPlaque(actual,placeName(city));exactHero(actual,g,page,food,12);
  }
  // Roll/reload uses the same cleared plaque, and never draws/eats a meal.
  for(unsigned city=0;city<4;++city){auto g=rpg::create(0,42);g.city=city;g.campStage=1;g.campRoll=20;g.campRation=true;ViewState v;v.page=Page::CampRoll;immutable(actual,g,v,12);exactPlaque(actual,placeName(city));++scenes;}
  for(const char* name:{"", "Ruinas", "Carvalho", "Bosque das Montanhas", "Uma regiao com um nome extraordinariamente longo"}){actual.fillScreen(0xffff);drawing=true;panelCampLocation(actual,name);drawing=false;exactPlaque(actual,name);}
  // The setup is a preview; a rest uses the committed ration, even when menu
  // toggles are stale after resume. Empty inventory cannot preview eating.
  for(bool saved:{false,true})for(bool selected:{false,true})for(unsigned stock:{0u,1u}){
   auto g=rpg::create(0,42);g.city=1;g.rations=stock;g.campRation=saved;menu.campRation=selected;menu.campKit=false;ViewState v;v.page=Page::CampSetup;immutable(actual,g,v,12);exactHero(actual,g,v.page,selected&&stock,12);++scenes;
   g.campStage=3;v.page=Page::CampRest;immutable(actual,g,v,12);exactHero(actual,g,v.page,saved,12);++scenes;
   g.campStage=0;immutable(actual,g,v,12);exactHero(actual,g,v.page,false,12);++scenes;
  }
  // Native 240x320 artifacts. Include all 16 identities and the full hand-to-
  // mouth sequence for selected varied races/classes/colors, with no scaling.
  const unsigned shirts[]={2,5,3,4},pants[]={1,6,4,5};
  for(unsigned identity=0;identity<16;++identity)for(auto page:{Page::CampSetup,Page::CampRest}){
   auto g=rpg::create(identity%4,42);g.race=identity/4;g.shirt=shirts[g.race];g.trousers=pants[g.p.cls];g.city=1;g.rations=6;g.sleepKit=true;g.campKit=true;g.campRation=true;g.campStage=page==Page::CampRest?3:0;menu.campKit=true;menu.campRation=true;ViewState v;v.page=page;v.campProgress=650;
   for(unsigned frame=0;frame<(fallback?1u:24u);++frame){immutable(actual,g,v,frame);actual.save(root+"/"+(page==Page::CampSetup?"setup":"rest")+"-race"+std::to_string(g.race)+"-class"+std::to_string(g.p.cls)+(fallback?"-fallback":"-ready")+"-frame"+std::to_string(frame)+".ppm");++scenes;}
  }
  if(!fallback)assert(rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 }
 assert(identities[0].size()==16&&identities[1].size()==16);for(unsigned i=0;i<64;++i)assert(data[i]==0xa5&&data[64+ART_BYTES+i]==0xa5);artMemory=nullptr;
 printf("PASS: %u seated hero rasters, %u exact source-head pixels, %u recolor cases, %u meal animation checks, %u immutable production scenes; all 16 identities; setup/committed-rest/no-food semantics; 4 locations + long labels; ready/fallback; render bounds, heap guards and art canaries; native animation frames.\n",rasters,headPixels,colorCases,animationChecks,scenes);
}
