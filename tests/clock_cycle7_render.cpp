#include "SceneCanvas.h"
#include <chrono>
#include <climits>
static bool measuring=false;static unsigned allocations=0;
extern "C" void* __real_malloc(size_t);extern "C" void* __real_calloc(size_t,size_t);extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){if(measuring)++allocations;return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t z){if(measuring)++allocations;return __real_calloc(n,z);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(measuring)++allocations;return __real_realloc(p,n);}
struct Probe:Canvas {
  unsigned rows=0,writes=0;int first=0,last=320;
  void reset(bool face){rows=writes=0;first=face?clockCycle7::FACE_TOP:0;last=face?clockCycle7::FACE_BOTTOM:320;}
  void fillRect(int x,int y,int w,int h,uint16_t color){assert(x>=0&&x+w<=240&&y>=first&&y+h<=last&&w>0&&h>0);writes+=w*h;Canvas::fillRect(x,y,w,h,color);}
  void draw16bitRGBBitmap(int x,int y,uint16_t* b,int w,int h){assert(x==0&&w==240&&h==1&&y==first+int(rows)&&y<last);++rows;Canvas::draw16bitRGBBitmap(x,y,b,w,h);}
};
static void draw(Probe& c,rpg::Game& g,ViewState& v,bool partial){
  const auto before=g;uint8_t menuBefore[sizeof(menu)],viewBefore[sizeof(v)];memcpy(menuBefore,&menu,sizeof(menu));memcpy(viewBefore,&v,sizeof(v));
  c.reset(partial);allocations=0;measuring=true;
  if(partial)drawClock(c,g,menu.renderNow/120,true);else render(c,g,v,menu.renderNow/120);
  measuring=false;assert(!allocations);assert(c.rows==unsigned(c.last-c.first));
  assert(!memcmp(&g,&before,sizeof(g))&&!memcmp(menuBefore,&menu,sizeof(menu))&&!memcmp(viewBefore,&v,sizeof(v)));
}
int main(int argc,char** argv){
  assert(argc==2);const std::string out=argv[1];artMemory=nullptr;
  auto g=rpg::create(0,123456);ViewState v;v.page=Page::Clock;Probe full,partial;
  unsigned frames=0;
  for(bool connected:{false,true})for(bool cycle:{false,true})for(bool automatic:{false,true})for(unsigned hour=0;hour<24;++hour){
    menu=MenuState{};menu.clockValid=true;menu.connected=connected;menu.dayCycle=cycle;menu.clockAutomatic=automatic;menu.sessionStarted=true;
    menu.worldPeriod=worldClock::Period::Night; // Gameplay phase may legitimately be frozen.
    snprintf(menu.clockTime,sizeof(menu.clockTime),"%02u:30:17",hour);strcpy(menu.clockDate,"14/10/2026");
    menu.renderNow=1000;draw(partial,g,v,false);
    for(unsigned ms:{1200u,2000u,2048u,13200u,13600u,14200u,14400u,UINT_MAX,0u}){
      menu.renderNow=ms;draw(full,g,v,false);draw(partial,g,v,true);assert(!memcmp(full.pixels,partial.pixels,sizeof(full.pixels)));frames+=2;
      for(unsigned y=0;y<320;++y)for(unsigned x=0;x<240;++x)if(y<24||y>=282||x<20||x>=220)assert(full.pixels[y*240+x]==clockArt::pixel(x,y));
    }
  }
  // Unset time has no fictional sky, orbit, twinkle or meteor, including after reboot.
  menu.clockValid=false;menu.renderNow=1000;draw(full,g,v,false);auto baseline=full;
  menu.renderNow=13600;draw(full,g,v,false);assert(!memcmp(full.pixels,baseline.pixels,sizeof(full.pixels)));full.save(out+"/clock-unset.ppm");
  // Touch/date/seconds label regressions are also covered by clock77_render.
  menu.clockValid=true;strcpy(menu.clockDate,"14/10/2026");
  for(unsigned h:{0u,5u,6u,7u,12u,18u,19u,20u,23u}){
    snprintf(menu.clockTime,sizeof(menu.clockTime),"%02u:18:12",h);menu.renderNow=13600;draw(full,g,v,false);
    full.save(out+"/clock-hour-"+std::to_string(h)+".ppm");
  }
  // Genuine firmware-rendered frames: one-day timelapse and real-speed pulses.
  for(unsigned f=0;f<96;++f){unsigned second=f*900;snprintf(menu.clockTime,sizeof(menu.clockTime),"%02u:%02u:00",second/3600,second/60%60);menu.renderNow=f*200;draw(full,g,v,false);full.save(out+"/daycycle-"+std::to_string(f)+".ppm");}
  for(bool night:{false,true})for(unsigned f=0;f<50;++f){menu.renderNow=10000+f*200;snprintf(menu.clockTime,sizeof(menu.clockTime),"%02u:18:%02u",night?23:12,10+f/5);draw(full,g,v,false);full.save(out+(night?"/night-":"/day-")+std::to_string(f)+".ppm");}
  // User feedback time: 12 real seconds, no orbit/time acceleration.
  menu.sessionStarted=true;
  for(unsigned f=0;f<60;++f){
    menu.renderNow=f*200;snprintf(menu.clockTime,sizeof(menu.clockTime),"08:33:%02u",13+f/5);
    draw(full,g,v,false);full.save(out+"/morning-realtime-"+std::to_string(f)+".ppm");
  }
  for(const char* time:{"08:33:13","12:00:00","17:00:00","23:00:00"}){
    strcpy(menu.clockTime,time);menu.renderNow=2048;draw(full,g,v,false);
    full.save(out+"/visibility-phase-"+std::string(time,2)+".ppm");
  }
  strcpy(menu.clockTime,"08:33:13");menu.renderNow=0;draw(full,g,v,false);auto pulseLow=full;
  full.save(out+"/visibility-pulse-low.ppm");
  menu.renderNow=2048;draw(full,g,v,false);full.save(out+"/visibility-pulse-high.ppm");
  unsigned changed=0;auto pulseState=clockCycle7::state(menu.clockTime,true,0);
  for(int y=0;y<320;++y)for(int x=0;x<240;++x)if(full.pixels[y*240+x]!=pulseLow.pixels[y*240+x]){
    ++changed;assert(std::abs(x-pulseState.x)<=clockCycle7::MEDALLION_RADIUS&&std::abs(y-pulseState.y)<=clockCycle7::MEDALLION_RADIUS);
  }
  assert(changed>=300); // A visible area/contrast change, not the old 156-pixel color-only breath.
  printf("PASS: morning pulse min/max changes %u pixels, entirely within the medallion; actual-render preview is 60 frames at 200 ms.\n",changed);
  g.eventStage=1;draw(full,g,v,false);full.save(out+"/clock-letter.ppm");g.eventStage=0;
  const auto started=std::chrono::steady_clock::now();for(unsigned i=0;i<1000;++i){menu.renderNow=i*200;draw(full,g,v,true);}
  auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
  printf("PASS: %u differential full/face frames, no allocations or game/RNG/menu/view mutations, existing ornament intact, null art RAM, live-time setting independence, no unset animation; face render host mean %lld us (not device FPS).\n",frames,(long long)(us/1000));
}
