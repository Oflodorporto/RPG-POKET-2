#!/usr/bin/env python3
"""Exercise the unchanged local-time adapter and the actual sketch paint path."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/clock-cycle7';OUT.mkdir(parents=True,exist_ok=True)

def run(name,source,extra=()):
    path=OUT/(name+'.cpp');path.write_text(source)
    command=['g++','-std=c++17','-O2','-UNDEBUG','-I'+str(ROOT/'tests/settings_fake'),str(path),*extra,'-o',str(OUT/name)]
    subprocess.run(command,cwd=ROOT,check=True)
    subprocess.run([str(OUT/name)],cwd=ROOT,check=True)

run('clock-local-time',f'''#define main old_settings_test
#include "{ROOT}/tests/settings_adapter.cpp"
#undef main
#include <climits>
int main(){{
 initSettings();
 const auto civil=worldClock::epoch(2026,12,31,23,59,0);
 for(int offset=-12;offset<=14;++offset){{
  assert(saveClockConfig(false,offset,true));localClock.set(civil,1000);now=1000;tickClock();
  time_t expected=civil+offset*3600;tm local;gmtime_r(&expected,&local);char text[9];strftime(text,sizeof(text),"%H:%M:%S",&local);
  assert(!strcmp(menu.clockTime,text));auto face=clockCycle7::state(menu.clockTime,menu.clockValid,now);
  assert(face.valid&&face.second==unsigned(local.tm_hour*3600+local.tm_min*60+local.tm_sec));
 }}
 assert(saveClockConfig(false,-3,true));
 localClock.set(worldClock::epoch(2026,12,31,23,59,-3)+59,0xfffffff0u);now=0xfffffff0u;tickClock();
 assert(!strcmp(menu.clockTime,"23:59:59")&&!strcmp(menu.clockDate,"31/12/2026"));
 auto previous=clockCycle7::state(menu.clockTime,true,now);
 now=984;tickClock();assert(!strcmp(menu.clockTime,"00:00:00")&&!strcmp(menu.clockDate,"01/01/2027"));
 auto next=clockCycle7::state(menu.clockTime,true,now);assert(previous.x==next.x&&previous.y==next.y);
 WiFi.state=0;menu.connected=false;now+=7200000;tickClock();assert(!strcmp(menu.clockTime,"02:00:00")&&menu.clockValid);
 localClock.set(worldClock::epoch(2026,10,14,12,0,-3),now);menu.worldPeriod=worldClock::Period::Night;tickClock(false);
 assert(menu.worldPeriod==worldClock::Period::Night&&clockCycle7::state(menu.clockTime,true,now).period==worldClock::Period::Day);
 assert(saveClockConfig(false,14,false));tickClock();assert(menu.clockValid&&!menu.dayCycle);
 localClock=worldClock::Clock{{}};localClock.manual=true;tickClock();assert(!menu.clockValid&&!strcmp(menu.clockTime,"--:--:--"));
 puts("PASS: actual tickClock, all 27 offset settings, offline/manual advance, midnight/year rollover across millis wrap, deferred world period and unset restart.");
}}
''')

sketch=(ROOT/'firmware/RPG_POKET_2/RPG_POKET_2.ino').read_text()
paint='void paint(uint32_t now){'+sketch.split('void paint(uint32_t now){',1)[1].split('\nbool readRegs(',1)[0]
run('clock-paint-path',f'''#include "{ROOT}/tests/SceneCanvas.h"
#include <climits>
struct Frame:Canvas {{}} frameBuffer;
struct Lcd:Canvas {{unsigned rows=0,calls=0;int first=-1,last=0;
 void reset(){{rows=calls=0;first=-1;last=0;}}
 void draw16bitRGBBitmap(int x,int y,uint16_t* b,int w,int h){{assert(x==0&&w==240&&h>0&&h<=16);if(first<0)first=y;else assert(y==last);last=y+h;rows+=h;++calls;Canvas::draw16bitRGBBitmap(x,y,b,w,h);}}
}} lcd;
struct UpdateSpiLock {{}};
rpg::Game game;ViewState view;CombatFx combatFx;bool buffered=true,dirty=false;uint32_t frameMax=0,ticks=0;
uint32_t millis(){{return ticks;}}unsigned polls=0;void sampleTouch(){{++polls;}}
{paint}
static void check(unsigned now,unsigned rows,unsigned first){{lcd.reset();polls=0;paint(now);assert(lcd.rows==rows&&lcd.first==int(first)&&polls==lcd.calls);Canvas expected;render(expected,game,view,now/120);assert(!memcmp(lcd.pixels,expected.pixels,sizeof(lcd.pixels)));}}
int main(){{
 static uint8_t scenery[ART_BYTES]{{}};artMemory=scenery;
 game=rpg::create(0,42);view.page=Page::Clock;menu.clockValid=true;menu.sessionStarted=true;strcpy(menu.clockTime,"23:18:12");strcpy(menu.clockDate,"14/10/2026");
 check(1000,320,0);check(1200,144,40);check(13600,144,40);
 dirty=true;strcpy(menu.clockTime,"23:18:13");check(14000,320,0);dirty=false;check(14200,144,40);
 // Any state change forces a complete screen, preventing stale date/notice/letter regions.
 game.eventStage=1;check(14400,320,0);game.eventStage=0;dirty=true;check(15000,320,0);dirty=false;
 menu.clockValid=false;check(15400,320,0);menu.clockValid=true;dirty=true;check(16000,320,0);dirty=false;
 view.page=Page::Clock;check(UINT_MAX,144,40);check(0,144,40);
 // Exit/return redraw and unsupported-buffer path keep their existing renderer.
 dirty=true;view.page=Page::Settings;check(1000,320,0);view.page=Page::Clock;dirty=false;check(1100,320,0);buffered=false;lcd.reset();paint(1200);Canvas expected;render(expected,game,view,10);assert(!memcmp(lcd.pixels,expected.pixels,sizeof(lcd.pixels)));
 puts("PASS: actual sketch paint: full entry/status/date/letter, exact differential 144-row pulse region, millis wrap, touch stripe checkpoints and direct-LCD fallback.");
}}
''')
# Guard the actual loop integration, not just the helper in isolation.
assert 'view.page==Page::Clock&&clockCycle7::animationDue(menu.clockValid,menu.sessionStarted&&game.eventStage==1,now,drawAt)' in sketch
assert '(view.page==Page::Clock&&game.eventStage==1)' not in sketch
print('PASS: clock cadence is only enabled on the idle page; normal gameplay cadence remains unchanged.')

run('clock-idle-controller',f'''#define main old_title_test
#include "{ROOT}/tests/title_controller.cpp"
#undef main
int main(){{
 for(bool session:{{false,true}})for(unsigned event:{{0u,1u}})for(Page page:{{Page::Title,Page::Home,Page::Menu,Page::Settings}}){{
  menu=MenuState{{}};menu.sessionStarted=session;view=ViewState{{}};view.page=page;game=rpg::create(0,42);game.eventStage=event;lastActivity=100;now=60099;
  auto original=game;auto blobs=nvs.blobs;assert(!tickIdleClock(now,false));now=60100;assert(tickIdleClock(now,false)&&view.page==Page::Clock&&menu.clockIdle&&clockReturn==page);
  assert(!tickIdleClock(now+500000,false));tapped(120,120);
  assert(!menu.clockIdle&&view.page==(session&&event==1?Page::Letter:page));
  assert(!memcmp(&game,&original,sizeof(game))&&nvs.blobs==blobs);
  if(session&&event==1)assert(menu.eventReturn==int(page));
 }}
 view.page=Page::Home;lastActivity=0xffff8000u;assert(!tickIdleClock(0xffff8001u,true));
 assert(!tickIdleClock(0xffff8001u+59999u,false));assert(tickIdleClock(0xffff8001u+60000u,false));
 puts("PASS: unchanged actual idle/tap controller: title/game/settings return, guild letter routing, repeated idle and wrap, saves/RNG untouched.");
}}
''')
