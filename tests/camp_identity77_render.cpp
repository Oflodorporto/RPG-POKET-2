#include "SceneCanvas.h"
#include <filesystem>
#include <new>
#include <set>

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
Envelope envelope(Page page){return page==Page::CampSetup?Envelope{38,108,58,66}:Envelope{18,114,84,96};}
constexpr Envelope restSubtitle={14,81,212,17};
bool inEnvelope(int x,int y,Envelope e){return x>=e.x&&x<e.x+e.w&&y>=e.y&&y<e.y+e.h;}
void exactSubtitle(const Canvas& scene,bool food,bool kit){
 const char* labels[2][2]={{"Sem comida... dormir no chao","Sem comida, mas com saco de dormir"},{"Carne e uma fogueira quentinha","Carne, fogueira e saco de dormir"}};
 const auto* label=labels[food][kit];const int x=(240-int(strlen(label))*6)/2;
 assert(x>=restSubtitle.x&&x+int(strlen(label))*6<=restSubtitle.x+restSubtitle.w);
 Canvas expected;expected.fillScreen(0x0843);expected.setTextColor(UI_GOLD);expected.setTextSize(1);expected.setCursor(x,85);expected.print(label);
 for(int y=restSubtitle.y;y<restSubtitle.y+restSubtitle.h;++y)for(int px=restSubtitle.x;px<restSubtitle.x+restSubtitle.w;++px)assert(scene.pixels[y*240+px]==expected.pixels[y*240+px]);
}
uint64_t heroHash(const HeroCanvas& c){uint64_t hash=1469598103934665603ULL;for(int i=0;i<c.width*c.height;++i){hash^=c.pixels[i];hash*=1099511628211ULL;}return hash;}
void immutable(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame){auto before=g;auto menuBefore=menu;auto viewBefore=v;uint8_t saveBefore[rpg::SAVE_SIZE],saveAfter[rpg::SAVE_SIZE];rpg::encode(g,73,saveBefore);drawing=true;render(c,g,v,frame);drawing=false;rpg::encode(g,73,saveAfter);assert(!memcmp(&before,&g,sizeof(g))&&!memcmp(&menuBefore,&menu,sizeof(menu))&&!memcmp(&viewBefore,&v,sizeof(v))&&!memcmp(saveBefore,saveAfter,sizeof(saveBefore)));}
}
int main(int argc,char** argv){
 const std::string root=argc>1?argv[1]:"build/screens/camp-identity77";std::filesystem::create_directories(root);
 std::vector<uint8_t> data(ART_BYTES+128,0xa5);artMemory=data.data()+64;std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 unsigned exactRasters=0,scenes=0,visibleShirt=0,visiblePants=0,fallbackMaskGaps=0,subtitleChecks=0;Canvas reference,actual,baseline;HeroCanvas hero;std::set<uint64_t> identities[2];
 for(bool fallback:{false,true}){
  menu=MenuState{};menu.art=fallback?ArtStatus::Fallback:ArtStatus::Ready;if(fallback)makeFallback();
  // Independent oracle: the normal personalSprite output, nearest-neighbor fit
  // into the two existing camp hero rectangles. Every race/class/color pair.
  for(unsigned race=0;race<4;++race)for(unsigned cls=0;cls<4;++cls)for(unsigned shirt=0;shirt<8;++shirt)for(unsigned pants=0;pants<8;++pants){
   auto g=rpg::create(cls,42);g.race=race;g.shirt=shirt;g.trousers=pants;reference.fillScreen(sentinel);personalSprite(reference,g,0,0,0);
   for(auto page:{Page::CampSetup,Page::CampRest}){auto e=envelope(page);hero.clear(e.w,e.h);drawing=true;campUi::hero(hero,g,0,0,e.w,e.h);drawing=false;
    for(int y=0;y<e.h;++y)for(int x=0;x<e.w;++x)assert(hero.pixels[y*e.w+x]==reference.pixels[(y*120/e.h)*240+x*112/e.w]);
    if(!fallback&&shirt==2&&pants==3)identities[page==Page::CampRest].insert(heroHash(hero));
    ++exactRasters;
   }
  }
  // Race and outfit only change pixels in the hero envelope; all scene, fire,
  // sleeping kit, text, progress, and interaction coordinates stay identical.
  for(auto page:{Page::CampSetup,Page::CampRest})for(unsigned city=0;city<4;++city)for(bool food:{false,true})for(bool kit:{false,true})for(unsigned frame:{0u,1u}){
   auto g=rpg::create(0,42);g.city=city;g.rations=6;g.sleepKit=true;g.campRation=food;g.campKit=kit;menu.campRation=food;menu.campKit=kit;ViewState v;v.page=page;v.campProgress=500;
   immutable(baseline,g,v,frame);++scenes;if(page==Page::CampRest){exactSubtitle(baseline,food,kit);++subtitleChecks;}
   for(unsigned race=0;race<4;++race){g.race=race;g.shirt=2;g.trousers=3;immutable(actual,g,v,frame);++scenes;auto e=envelope(page);unsigned changes=0;
    for(int y=0;y<320;++y)for(int x=0;x<240;++x){const bool different=actual.pixels[y*240+x]!=baseline.pixels[y*240+x];const bool approvedChange=inEnvelope(x,y,e)||(page==Page::CampRest&&inEnvelope(x,y,restSubtitle));if(!approvedChange)assert(!different);changes+=different;}
    assert(changes);if(page==Page::CampRest){exactSubtitle(actual,food,kit);++subtitleChecks;}
   }
  }
  // Both clothing channels remain visibly customizable for all 16 identities
  // at both sizes. Switching one channel must leave the other channel alone.
  for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race)for(auto page:{Page::CampSetup,Page::CampRest}){
   auto g=rpg::create(cls,42);g.race=race;const auto e=envelope(page);hero.clear(e.w,e.h);campUi::hero(hero,g,0,0,e.w,e.h);const auto before=hero;
   for(unsigned channel:{1u,2u}){g.shirt=channel==1?2:0;g.trousers=channel==2?3:0;hero.clear(e.w,e.h);campUi::hero(hero,g,0,0,e.w,e.h);unsigned visible=0;
    for(int y=0;y<e.h;++y)for(int x=0;x<e.w;++x){const unsigned index=(y*120/e.h)*112+x*112/e.w;const uint8_t mask=assetBytes(raceMask[race][cls][0])[index];const bool different=hero.pixels[y*e.w+x]!=before.pixels[y*e.w+x];if(different){assert(mask==channel);++visible;}}
    // The inherited coarse fallback masks omit these two channels. Ready
    // artwork must expose both channels for all 16 identities at both sizes.
    if(!visible){assert(fallback&&race==0&&((cls==1&&channel==2)||(cls==2&&channel==1)));++fallbackMaskGaps;}
    if(channel==1)visibleShirt+=visible;else visiblePants+=visible;
   }
  }
  // Native 240x320 production render previews, contrasting races/classes/colors.
  const unsigned shirts[]={2,5,3,4},pants[]={1,6,4,5};
  for(unsigned identity=0;identity<4;++identity)for(auto page:{Page::CampSetup,Page::CampRest}){
   auto g=rpg::create(identity,42);g.race=identity;g.shirt=shirts[identity];g.trousers=pants[identity];g.city=1;g.rations=6;g.sleepKit=true;g.campKit=true;g.campRation=true;menu.campKit=true;menu.campRation=true;ViewState v;v.page=page;v.campProgress=650;immutable(actual,g,v,0);++scenes;
   actual.save(root+"/"+(page==Page::CampSetup?"setup":"rest")+"-race"+std::to_string(identity)+"-class"+std::to_string(identity)+(fallback?"-fallback":"-ready")+".ppm");
  }
  if(!fallback)assert(rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 }
 assert(fallbackMaskGaps==4);assert(identities[0].size()==16&&identities[1].size()==16);for(unsigned i=0;i<64;++i)assert(data[i]==0xa5&&data[64+ART_BYTES+i]==0xa5);artMemory=nullptr;
 printf("PASS: %u exact camp hero rasters (4 races x 4 classes x 8 shirts x 8 trousers x 2 sizes x ready/fallback), 16 distinct silhouettes per size, %u read-only native scenes, visible shirt/trousers pixels %u/%u, %u exact subtitle strips across ration/kit choices and all 4 backgrounds; 2 inherited fallback channel omissions at both sizes; bounds/heap guards and art canaries; 16 native previews.\n",exactRasters,scenes,visibleShirt,visiblePants,subtitleChecks);
}
