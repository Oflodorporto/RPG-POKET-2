#pragma once
#include <stdint.h>
#include "WorldClock.h"

// Presentation-only civil-time dial. No astronomical/location assumptions,
// timers to catch up, heap, persisted state, or calls to the gameplay RNG.
namespace clockCycle7 {
constexpr int FACE_TOP=40,FACE_BOTTOM=184;
constexpr uint32_t FRAME_MS=200,DAY_SECONDS=86400;
inline bool animationDue(bool valid,bool letter,uint32_t now,uint32_t last){
  return (valid||letter)&&uint32_t(now-last)>=(valid?FRAME_MS:480u);
}
inline constexpr uint16_t rgb(unsigned r,unsigned g,unsigned b){return uint16_t((r>>3)<<11|(g>>2)<<5|(b>>3));}
inline uint16_t mix(uint16_t a,uint16_t b,unsigned weight){
  unsigned inverse=256-weight;
  return uint16_t(((((a>>11)&31)*inverse+((b>>11)&31)*weight)>>8)<<11|
                  ((((a>>5)&63)*inverse+((b>>5)&63)*weight)>>8)<<5|
                  (((a&31)*inverse+(b&31)*weight)>>8));
}
// A quarter-wave in flash and linear interpolation avoid per-frame trig.
inline int sine(uint32_t second){
  static constexpr uint16_t quarter[]={0,67,134,200,265,329,392,453,512,569,623,675,724,770,812,851,887,918,946,970,989,1004,1015,1022,1024};
  second%=DAY_SECONDS;unsigned q=second/21600,p=second%21600;
  if(q&1)p=21600-p;
  unsigned index=p/900,remainder=p%900;
  int value=quarter[index];if(index<24)value+=(int(quarter[index+1])-value)*int(remainder)/900;
  return q>=2?-value:value;
}
inline int scaled(int value,int radius){return value<0?-((-value*radius+512)/1024):(value*radius+512)/1024;}
struct State {
  bool valid=false,night=false;uint32_t second=0;
  worldClock::Period period=worldClock::Period::Day;
  int x=120,y=52;unsigned pulse=0,stars=0,glow=0;
  uint16_t top=0,bottom=0;int meteor=-1;
};
inline State state(const char (&time)[9],bool valid,uint32_t now){
  State s;
  if(!valid||time[2]!=':'||time[5]!=':'||time[8]!=0)return s;
  for(unsigned i=0;i<8;++i)if(i!=2&&i!=5)if(time[i]<'0'||time[i]>'9')return s;
  unsigned h=(time[0]-'0')*10+time[1]-'0',m=(time[3]-'0')*10+time[4]-'0',sec=(time[6]-'0')*10+time[7]-'0';
  if(h>=24||m>=60||sec>=60)return s;
  s.valid=true;s.second=h*3600+m*60+sec;s.period=worldClock::period(h);
  s.night=s.period==worldClock::Period::Night;
  s.x=120-scaled(sine(s.second),89);s.y=111+scaled(sine(s.second+21600),59);
  // 4.096-second breathing light; a power-of-two period stays continuous
  // across uint32 millis rollover. Fixed radius, never an on/off blink.
  unsigned phase=now&4095u;s.pulse=phase<2048?phase/8:(4096-phase)/8;
  struct Key {uint32_t second;uint16_t top,bottom;unsigned stars,glow;};
  static constexpr Key keys[]={
    {0,rgb(8,20,42),rgb(13,28,47),210,0},
    {5*3600,rgb(8,20,42),rgb(13,28,47),210,0},
    {6*3600,rgb(31,36,63),rgb(91,53,33),60,190},
    {7*3600,rgb(18,51,86),rgb(51,61,76),0,135},
    {18*3600,rgb(18,51,86),rgb(51,61,76),0,135},
    {19*3600,rgb(43,29,51),rgb(87,44,31),35,180},
    {20*3600,rgb(8,20,42),rgb(13,28,47),210,0},
    {DAY_SECONDS,rgb(8,20,42),rgb(13,28,47),210,0}};
  unsigned i=0;while(i<6&&s.second>=keys[i+1].second)++i;
  const auto& a=keys[i];const auto& b=keys[i+1];
  unsigned weight=(s.second-a.second)*256/(b.second-a.second);
  s.top=mix(a.top,b.top,weight);s.bottom=mix(a.bottom,b.bottom,weight);
  s.stars=(a.stars*(256-weight)+b.stars*weight)/256;
  s.glow=(a.glow*(256-weight)+b.glow*weight)/256;
  // One brief, deterministic shooting star per 97 s of uptime, only at night.
  // It never advances game RNG or appears in an unset-clock fallback.
  unsigned meteorPhase=now%97000;
  if(s.night&&meteorPhase>=13000&&meteorPhase<14400)s.meteor=int(meteorPhase-13000);
  return s;
}
inline bool sky(int x,int y){int dx=x-120,dy=y-111;return dx*dx*1936+dy*dy*6241<=12082576;}
inline uint16_t skyPixel(uint16_t original,int x,int y,const State& s){
  if(!s.valid||!sky(x,y))return original;
  unsigned row=unsigned(y-67)*256/88;
  uint16_t tone=mix(s.top,s.bottom,row);
  // Muted painted dawn/day rays in the upper sky, below the ornate rim.
  int dx=x-120;if(dx<0)dx=-dx;
  int dy=y-88;if(dy<0)dy=-dy;
  int light=72-dx-dy*2;
  if(light>0&&s.glow)tone=mix(tone,rgb(126,92,39),unsigned(light)*s.glow/144);
  // Soft painted edge, not a new opaque panel inside the existing ornament.
  int radial=(x-120)*(x-120)*1936+(y-111)*(y-111)*6241;
  unsigned edge=unsigned(12082576-radial)/12586;
  unsigned alpha=edge<192?edge:192;
  return mix(original,tone,alpha);
}
}
