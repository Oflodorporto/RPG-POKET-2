#include "SceneCanvas.h"
#include <new>
#include <chrono>
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
struct BoundCanvas:Canvas {unsigned calls=0;void fillRect(int x,int y,int w,int h,uint16_t col){assert(x>=0&&y>=0&&x+w<=240&&y+h<=320);++calls;Canvas::fillRect(x,y,w,h,col);}};
static uint64_t hash(const Canvas& c){uint64_t h=1469598103934665603ULL;for(auto p:c.pixels){h^=p;h*=1099511628211ULL;}return h;}
int main(){
 unsigned values=0,edges[12][12]{};
 for(auto f:d20Ui::faces){assert(f.a<12&&f.b<12&&f.c<12&&f.a!=f.b&&f.b!=f.c&&f.a!=f.c);assert(f.value>=1&&f.value<=20);assert(!(values&(1u<<f.value)));values|=1u<<f.value;for(auto e:{std::pair<unsigned,unsigned>{f.a,f.b},{f.b,f.c},{f.c,f.a}})++edges[std::min(e.first,e.second)][std::max(e.first,e.second)];}
 assert(values==0x1ffffe);unsigned edgeCount=0;for(unsigned a=0;a<12;++a)for(unsigned b=a+1;b<12;++b)if(edges[a][b]){assert(edges[a][b]==2);++edgeCount;}assert(edgeCount==30&&12+20-edgeCount==2);
 std::set<uint64_t> finals;unsigned dieFrames=0,maxCalls=0;
 const auto start=std::chrono::steady_clock::now();
 for(unsigned result=1;result<=20;++result){
  int front=-1000,back=1000;unsigned frontValue=0,backValue=0;
  for(auto f:d20Ui::faces){auto a=d20Ui::vertex(result,f.a,true,0),b=d20Ui::vertex(result,f.b,true,0),c=d20Ui::vertex(result,f.c,true,0);int z=a.z+b.z+c.z;if(z>front){front=z;frontValue=f.value;}if(z<back){back=z;backValue=f.value;}}
  assert(frontValue==result&&frontValue+backValue==21);
  for(unsigned radius:{31u,39u,46u}){std::set<uint64_t> rolling;BoundCanvas c;
   for(unsigned step=0;step<16;++step){c.fillScreen(0x1234);c.calls=0;drawing=true;d20Ui::draw(c,120,139,radius,result,step==15,step*120);drawing=false;maxCalls=std::max(maxCalls,c.calls);assert(c.calls<6000);uint64_t h=hash(c);if(step<15)rolling.insert(h);else if(radius==46)finals.insert(h);++dieFrames;}
   assert(rolling.size()==15);auto final=hash(c);c.fillScreen(0x1234);drawing=true;d20Ui::draw(c,120,139,radius,result,true,UINT32_MAX);drawing=false;assert(hash(c)==final);
  }
 }
 assert(finals.size()==20);
 const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
 std::vector<uint8_t> memory(ART_BYTES+128,0xa5);artMemory=memory.data()+64;std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read((char*)artMemory,ART_BYTES);assert(pack.gcount()==ART_BYTES);Canvas c;unsigned scenes=0;
 for(bool fallback:{false,true}){menu=MenuState{};menu.art=fallback?ArtStatus::Fallback:ArtStatus::Ready;if(fallback)makeFallback();
  for(unsigned result=1;result<=20;++result)for(unsigned context=0;context<4;++context){auto g=rpg::create(result%4,17);g.race=result%4;g.shirt=result%8;g.trousers=(result+3)%8;g.rations=6;g.sleepKit=true;ViewState v;
   if(context<2){assert(!rpg::prepareDailyTrip(g,3));g.tripRoll=result;g.tripTotal=result+3;g.daily.roll=result;g.daily.total=result+5;g.daily.difficulty=13;g.daily.phase=context?rpg::DailyPhase::NightRoll:rpg::DailyPhase::RoadRoll;v.page=context?Page::CampRoll:Page::TravelRoll;}
   else if(context==2){g.campRoll=result;g.campRation=true;g.campKit=true;v.page=Page::CampRoll;}
   else {g.tripStage=1;g.tripRoll=result;g.tripTotal=result+3;v.page=Page::TravelRoll;}
   for(unsigned step=0;step<16;++step){menu.rollStarted=UINT32_MAX-500;menu.renderNow=menu.rollStarted+step*120;menu.rollReady=step==15;auto before=g;auto beforeMenu=menu;auto beforeView=v;uint8_t saveBefore[rpg::SAVE_SIZE],saveAfter[rpg::SAVE_SIZE];rpg::encode(g,7,saveBefore);drawing=true;render(c,g,v,step);drawing=false;rpg::encode(g,7,saveAfter);assert(!memcmp(&g,&before,sizeof(g))&&!memcmp(&menu,&beforeMenu,sizeof(menu))&&!memcmp(&v,&beforeView,sizeof(v))&&!memcmp(saveBefore,saveAfter,sizeof(saveBefore)));++scenes;}
  }
 }
 for(unsigned i=0;i<64;++i)assert(memory[i]==0xa5&&memory[ART_BYTES+64+i]==0xa5);
 // Actual integration, including the larger authored sprite and city offsets.
 // Leg at1000 and Tent describe the identical stop; only tent pixels may change
 // above the HUD. This guards the placement branch, labels and route anchor.
 pack.clear();pack.seekg(16);pack.read((char*)artMemory,ART_BYTES);assert(pack.gcount()==ART_BYTES);menu=MenuState{};menu.art=ArtStatus::Ready;
 unsigned tentScenes=0,cityStops=0;Canvas beforeTent,afterTent,expected;
 for(unsigned from=0;from<4;++from)for(unsigned to=0;to<4;++to)if(from!=to){auto g=rpg::create(0,17);g.city=from;g.rations=6;assert(!rpg::prepareDailyTrip(g,to));ViewState v;v.page=Page::Travel;
  for(unsigned day=1;day<=g.daily.totalDays;++day){g.daily.day=day;g.daily.phase=rpg::DailyPhase::Leg;menu.journey.progress=1000;render(beforeTent,g,v,0);auto p=dailyUi::routePosition(g,1000).position();g.daily.phase=rpg::DailyPhase::Tent;menu.journey.progress=0;auto stop=dailyUi::routePosition(g,0).position();assert(p.x==stop.x&&p.y==stop.y);auto saved=g;render(afterTent,g,v,0);assert(!memcmp(&g,&saved,sizeof(g)));
   int x=p.x-tentMarker::Width/2,y=p.y+18-tentMarker::Height;bool city=false;
   for(auto place:places)if(p.x==place.x&&p.y==place.y){city=true;x=p.x+18;if(x+tentMarker::Width>240)x=p.x-17-tentMarker::Width;y=p.y+4-tentMarker::Height;break;}
   assert(x>=0&&x+tentMarker::Width<=240&&y>=53&&y+tentMarker::Height<=218);expected=beforeTent;drawing=true;tentMarker::draw(expected,x,y);drawing=false;
   for(int py=0;py<218;++py)for(int px=0;px<240;++px)assert(expected.pixels[py*240+px]==afterTent.pixels[py*240+px]);
   if(city){++cityStops;for(int py=p.y+8;py<std::min(218,p.y+21);++py)for(int px=std::max(0,p.x-27);px<std::min(240,p.x+28);++px)assert(beforeTent.pixels[py*240+px]==afterTent.pixels[py*240+px]);}
   ++tentScenes;
  }
 }
 assert(tentScenes==40&&cityStops==20);
 printf("PASS: %u actual marker integration scenes / %u city stops; exact anchors, no city-label changes, full marker above HUD.\n",tentScenes,cityStops);
 printf("PASS: real 12-vertex/30-edge/20-face D20; labels1..20 unique, opposite21 and correct front face for all results; %u animation/bounds/allocation frames, max%u draw calls; %u read-only ready/fallback native scenes across4contexts including timer wrap. Host isolated audit %lldus (not device FPS).\n",dieFrames,maxCalls,scenes,(long long)elapsed);
}
