#!/usr/bin/env python3
"""Compile the exact merged redraw predicate and exercise both animation paths."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
out=root/'build/package7-focused';out.mkdir(parents=True,exist_ok=True)
sketch=(root/'firmware/RPG_POKET_2/RPG_POKET_2.ino').read_text()
condition=sketch.split('  if(lcdReady&&(dirty||',1)[1].split('{drawAt=now;',1)[0]
condition='lcdReady&&(dirty||'+condition[:-1]
source=f'''#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main priorCadenceMain
#include "{root}/tests/title_controller.cpp"
#undef main
static bool due(uint32_t now){{return {condition};}}
int main(){{
 lcdReady=true;dirty=false;drawAt=100;combatFx=CombatFx{{}};menu=MenuState{{}};view=ViewState{{}};game=rpg::create(0,42);
 for(Page page:{{Page::CampSetup,Page::CampRest,Page::Ruins,Page::Home,Page::Battle}}){{view.page=page;assert(!due(219)&&due(220));}}
 view.page=Page::Clock;menu.clockValid=true;assert(!due(299)&&due(300));
 menu.clockValid=false;assert(!due(100000));menu.clockValid=true;menu.sessionStarted=true;game.eventStage=1;assert(!due(299)&&due(300));
 menu.clockValid=false;assert(!due(579)&&due(580));menu.clockValid=true;
 game.eventStage=0;view.page=Page::Settings;assert(!due(100000));dirty=true;assert(due(101));dirty=false;
 view.page=Page::CampSetup;drawAt=0xfffffff0u;assert(!due(103)&&due(104));view.page=Page::Clock;assert(!due(183)&&due(184));
 lcdReady=false;dirty=true;assert(!due(100000));
 puts("PASS package7 exact merged loop predicate: camp/gameplay 120 ms, idle clock 200 ms, letter cadence, dirty bypass, inactive pages, LCD guard and millis wrap.");
}}
'''
path=out/'cadence.cpp';path.write_text(source)
subprocess.run(['g++','-std=c++17','-O2','-UNDEBUG',str(path),'-o',str(out/'cadence')],cwd=root,check=True)
subprocess.run([str(out/'cadence')],cwd=root,check=True)
