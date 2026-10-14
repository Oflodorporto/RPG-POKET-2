#include "../firmware/RPG_POKET_2/ClockCycle7.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <climits>
int main(){
  char text[9];clockCycle7::State previous;
  for(unsigned second=0;second<86400;++second){
    snprintf(text,sizeof(text),"%02u:%02u:%02u",second/3600,second/60%60,second%60);
    auto s=clockCycle7::state(text,true,2048);
    assert(s.valid&&s.second==second&&s.period==worldClock::period(second/3600));
    assert(s.x>=31&&s.x<=209&&s.y>=52&&s.y<=170);
    const double angle=double(second)*6.283185307179586/86400;
    assert(std::abs(s.x-(120-89*std::sin(angle)))<1.1&&std::abs(s.y-(111+59*std::cos(angle)))<1.1);
    if(second)assert(std::abs(s.x-previous.x)<=1&&std::abs(s.y-previous.y)<=1);
    assert(s.night==(second<18000||second>=72000));
    assert(s.stars<=210&&s.glow<=190&&s.pulse==256);previous=s;
  }
  strcpy(text,"00:00:00");auto zero=clockCycle7::state(text,true,0);
  assert(std::abs(previous.x-zero.x)<=1&&std::abs(previous.y-zero.y)<=1&&zero.x==120&&zero.y==170);
  strcpy(text,"06:00:00");auto dawn=clockCycle7::state(text,true,0);assert(dawn.x==31&&dawn.y==111);
  strcpy(text,"12:00:00");auto noon=clockCycle7::state(text,true,0);assert(noon.x==120&&noon.y==52);
  strcpy(text,"18:00:00");auto dusk=clockCycle7::state(text,true,0);assert(dusk.x==209&&dusk.y==111);
  const char* bad[]={"--:--:--","24:00:00","12:60:00","12:00:60","00/00/00","aa:00:00","0 :00:00"};
  for(auto b:bad){strcpy(text,b);assert(!clockCycle7::state(text,true,UINT_MAX).valid);}
  memset(text,'8',sizeof(text));assert(!clockCycle7::state(text,true,0).valid);
  strcpy(text,"23:59:59");assert(!clockCycle7::state(text,false,0).valid);
  unsigned meteorFrames=0,maxStep=0,lastPulse=0;
  for(unsigned ms=0;ms<194000;ms+=200){
    auto s=clockCycle7::state(text,true,ms);meteorFrames+=s.meteor>=0;
    unsigned step=unsigned(std::abs(int(s.pulse)-int(lastPulse)));if(step>maxStep)maxStep=step;lastPulse=s.pulse;
    auto repeat=clockCycle7::state(text,true,ms);assert(s.x==repeat.x&&s.y==repeat.y&&s.meteor==repeat.meteor);
  }
  assert(meteorFrames==14&&maxStep<=26);
  strcpy(text,"12:00:00");assert(clockCycle7::state(text,true,13600).meteor<0);
  assert(!clockCycle7::animationDue(false,false,100000,0));
  assert(!clockCycle7::animationDue(true,false,1199,1000)&&clockCycle7::animationDue(true,false,1200,1000));
  assert(!clockCycle7::animationDue(false,true,479,0)&&clockCycle7::animationDue(false,true,480,0));
  assert(clockCycle7::animationDue(true,false,100,UINT_MAX-100));
  assert(!clockCycle7::animationDue(true,false,99,UINT_MAX-99));
  auto rolloverBefore=clockCycle7::state(text,true,UINT_MAX),rolloverAfter=clockCycle7::state(text,true,0);
  assert(rolloverBefore.valid&&rolloverAfter.valid&&std::abs(int(rolloverBefore.pulse)-int(rolloverAfter.pulse))<=1);
  assert(clockCycle7::mix(0x1234,0xabcd,0)==0x1234&&clockCycle7::mix(0x1234,0xabcd,256)==0xabcd);
  // Sky is clipped inside the original oval; the ornament and text plates stay intact.
  for(int y=0;y<320;++y)for(int x=0;x<240;++x)if(clockCycle7::sky(x,y))assert(x>=41&&x<=199&&y>=67&&y<=155);
  puts("PASS: 86,400 local-second positions, 24-hour continuity/cardinals, periods, malformed/unset time, bounded pulse/meteor, deterministic phases and millis wrap.");
}
