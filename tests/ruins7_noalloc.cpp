#include "SceneCanvas.h"
static bool watch=false;static unsigned allocations=0;
extern "C" void* __real_malloc(size_t);extern "C" void* __real_calloc(size_t,size_t);extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){if(watch)++allocations;return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t a,size_t b){if(watch)++allocations;return __real_calloc(a,b);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(watch)++allocations;return __real_realloc(p,n);}
int main(){Canvas c;auto g=rpg::create(0,42);g.city=1;g.tutorial=true;ViewState v;v.page=Page::Ruins;unsigned frames=0;artMemory=nullptr;
 for(bool fallback:{false,true})for(auto period:{worldClock::Period::Dawn,worldClock::Period::Day,worldClock::Period::Dusk,worldClock::Period::Night})for(unsigned page=0;page<2;++page){menu.art=fallback?ArtStatus::Fallback:ArtStatus::Ready;menu.clockValid=menu.dayCycle=true;menu.worldPeriod=period;v.cityPage=page;watch=true;render(c,g,v,32);watch=false;assert(!allocations);++frames;}
 auto before=c;ruins7Ui::image(c,nullptr,220,56,10,3);ruins7Ui::image(c,ruins7Art::titlePixels,241,56,0,0);ruins7Ui::frame(c,{-1,0,112,44});ruins7Ui::frame(c,{0,0,10,10});assert(!memcmp(before.pixels,c.pixels,sizeof(c.pixels)));
 printf("PASS: %u real Ruins page/day/fallback renders without malloc/calloc/realloc; null SD art store; invalid sprite/frame bounds safely ignored.\n",frames);
}
