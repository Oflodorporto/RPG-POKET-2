#include "SceneCanvas.h"
#include "../firmware/RPG_POKET_2/ClockView.h"
#include <climits>

#ifdef RPG_TEST_CLOCK_ALLOCATIONS
static bool measuringClock=false;
static unsigned clockAllocations=0;
extern "C" void* __real_malloc(size_t);
extern "C" void* __real_calloc(size_t,size_t);
extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){if(measuringClock)++clockAllocations;return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t s){if(measuringClock)++clockAllocations;return __real_calloc(n,s);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(measuringClock)++clockAllocations;return __real_realloc(p,n);}
#endif

struct ClockCanvas:Canvas {
  struct Label {int x,y,size;uint16_t color;char text[64];};
  Label labels[32]{};unsigned count=0,rows=0;
  void reset(){count=rows=0;}
  void fillRect(int x,int y,int w,int h,uint16_t color){
    assert(x>=0&&y>=0&&w>0&&h>0&&w<=240-x&&h<=320-y);
    Canvas::fillRect(x,y,w,h,color);
  }
  void drawRect(int x,int y,int w,int h,uint16_t color){
    assert(x>=0&&y>=0&&w>0&&h>0&&w<=240-x&&h<=320-y);
    Canvas::drawRect(x,y,w,h,color);
  }
  void draw16bitRGBBitmap(int x,int y,uint16_t* bytes,int w,int h){
    assert(x==0&&y==int(rows)&&w==240&&h==1);++rows;
    Canvas::draw16bitRGBBitmap(x,y,bytes,w,h);
  }
  void print(const char* text){
    assert(count<32&&strlen(text)<64);
    auto& l=labels[count++];l.x=tx;l.y=ty;l.size=size;l.color=color;strcpy(l.text,text);
    Canvas::print(text);
  }
  bool has(const char* text)const{for(unsigned i=0;i<count;++i)if(!strcmp(labels[i].text,text))return true;return false;}
  uint32_t hash()const{return rpg::crc(reinterpret_cast<const uint8_t*>(pixels),sizeof(pixels));}
};

static void safelyDraw(ClockCanvas& c,const rpg::Game& g,const ViewState& v,unsigned frame){
  uint8_t gameBefore[sizeof(g)],menuBefore[sizeof(menu)],viewBefore[sizeof(v)];
  memcpy(gameBefore,&g,sizeof(g));memcpy(menuBefore,&menu,sizeof(menu));memcpy(viewBefore,&v,sizeof(v));
  c.reset();
#ifdef RPG_TEST_CLOCK_ALLOCATIONS
  clockAllocations=0;measuringClock=true;
#endif
  render(c,g,v,frame);
#ifdef RPG_TEST_CLOCK_ALLOCATIONS
  measuringClock=false;assert(clockAllocations==0);
#endif
  assert(c.rows==320);
  assert(!memcmp(gameBefore,&g,sizeof(g))&&!memcmp(menuBefore,&menu,sizeof(menu))&&!memcmp(viewBefore,&v,sizeof(v)));
  // Titles, time, date, letter/return message and notice stay in independent wells.
  for(unsigned i=0;i<c.count;++i){const auto& l=c.labels[i];int right=l.x+int(strlen(l.text))*6*l.size;
    if(l.y==24)assert(l.x>=60&&right<=180);
    else if(l.y==102||l.y==141||l.y==155)assert(l.x>=35&&right<=205&&l.y+8*l.size<=163);
    else if(l.y==198)assert(l.x>=46&&right<=194&&l.y+8*l.size<=214);
    else if(l.y==232||l.y==247)assert(l.x>=65&&right<=207&&l.y+8*l.size<=255);
    else if(l.y==235||l.y==250)assert(l.x>=25&&right<=215&&l.y+8*l.size<=258);
    else {assert(l.y==274&&l.x>=46&&right<=194&&l.y+8*l.size<=282);}
  }
  // No Wi-Fi HUD or other overlay may cover the ornamental trim on idle Clock.
  for(unsigned y=0;y<320;++y)for(unsigned x=0;x<240;++x){
    if(y<24||y>=282||x<20||x>=220)assert(c.pixels[y*240+x]==clockArt::pixel(x,y));
  }
}

int main(int argc,char** argv){
  assert(argc==2||argc==3);const std::string out=argv[1];
  assert(sizeof(clockArt::plane)==76800&&sizeof(clockArt::palette)==512);
  assert(clockArt::pixel(240,0)==0&&clockArt::pixel(0,320)==0&&clockArt::pixel(UINT_MAX,0)==0&&clockArt::pixel(0,UINT_MAX)==0);
  if(argc==3){std::ifstream file(argv[2],std::ios::binary);assert(file);
    for(unsigned y=0;y<320;++y)for(unsigned x=0;x<240;++x){uint8_t b[2];file.read(reinterpret_cast<char*>(b),2);assert(file.gcount()==2);assert(clockArt::pixel(x,y)==uint16_t(b[0]|unsigned(b[1])<<8));}
    assert(file.peek()==std::char_traits<char>::eof());
  }
  // Full clock art must remain usable with no PSRAM art allocation at all.
  artMemory=nullptr;ClockCanvas c;ViewState v;v.page=Page::Clock;v.touchFeedback=true;
  const char* times[]={"10:33:21","23:59:59","00:00:00"};
  const char* dates[]={"13/10/2026","31/12/2099","01/01/2026"};
  const unsigned brightness[]={1,35,80,100};unsigned frames=0;
  for(auto art:{ArtStatus::Ready,ArtStatus::Fallback,ArtStatus::Missing,ArtStatus::Invalid,ArtStatus::Memory})
  for(bool session:{false,true})for(unsigned event=0;event<3;++event)for(bool valid:{false,true})
  for(unsigned period=0;period<4;++period)for(unsigned frame:{0u,4u})for(unsigned t=0;t<3;++t){
    auto g=rpg::create(0,42);g.eventStage=event;menu=MenuState{};menu.art=art;
    menu.sessionStarted=session;menu.clockValid=valid;menu.worldPeriod=worldClock::Period(period);
    menu.brightness=brightness[period];menu.clockIdle=true;menu.connected=true;menu.signalBars=4;
    strcpy(menu.clockTime,times[t]);strcpy(menu.clockDate,dates[t]);
    safelyDraw(c,g,v,frame);++frames;
    const bool letter=session&&event==1;
    assert(c.has("EM DESCANSO")&&c.has("Carta da Guilda")==letter&&c.has("Toque para ler")==letter);
    assert(c.has("Toque para voltar")==!letter);
    if(!letter)assert(c.has(session?"ao jogo":"ao titulo"));
    assert(c.has("Ajuste hora / Wi-Fi")==!valid);
    if(valid){char hm[6],seconds[5];snprintf(hm,sizeof(hm),"%.5s",times[t]);snprintf(seconds,sizeof(seconds),"%.2s s",times[t]+6);
      assert(c.has(hm)&&c.has(seconds)&&c.has(dates[t])&&c.has(worldClock::name(worldClock::period(unsigned(times[t][0]-'0')*10+unsigned(times[t][1]-'0')))));
    }else assert(c.has("--:--")&&c.has("--")&&c.has("--/--/----")&&c.has("Sem hora")&&!c.has(dates[t]));
    if(!period&&!frame&&!t&&event==1){
      if(art==ArtStatus::Ready){
        if(session)c.save(out+(valid?"/clock-letter.ppm":"/clock-invalid-letter.ppm"));
        else if(valid)c.save(out+"/clock-title.ppm");
      }else if(art==ArtStatus::Fallback&&session&&valid)c.save(out+"/clock-fallback.ppm");
    }
    if(art==ArtStatus::Ready&&session&&event==0&&valid&&!period&&!frame&&!t)c.save(out+"/clock-return.ppm");
  }
  auto g=rpg::create(0,42);menu=MenuState{};menu.clockValid=true;menu.sessionStarted=true;
  strcpy(menu.clockTime,"23:59:59");strcpy(menu.clockDate,"31/12/2026");
  safelyDraw(c,g,v,0);const auto midnightBefore=c.hash();
  strcpy(menu.clockTime,"00:00:00");strcpy(menu.clockDate,"01/01/2027");
  safelyDraw(c,g,v,0);assert(c.has("00:00")&&c.has("00 s")&&c.has("01/01/2027")&&c.hash()!=midnightBefore);
  const auto baseline=c.hash();strcpy(menu.clockTime,"00:00:01");safelyDraw(c,g,v,0);assert(c.has("01 s")&&c.hash()!=baseline);
  c.save(out+"/clock-live-rollover.ppm");
  // Non-terminated fields cannot overrun fixed MenuState arrays or layout wells.
  memset(menu.clockTime,'8',sizeof(menu.clockTime));memset(menu.clockDate,'8',sizeof(menu.clockDate));
  safelyDraw(c,g,v,UINT_MAX);assert(c.has("88888")&&c.has("88 s")&&c.has("8888888888"));
  menu.clockValid=false;safelyDraw(c,g,v,UINT_MAX);assert(c.has("--:--")&&!c.has("88888"));
  printf("PASS: %u clock frames, all five art states without art RAM, live time/date/second rollover, letter and invalid-time independence, disjoint text wells, bounds, state immutability and flash reference.\n",frames+5);
}
