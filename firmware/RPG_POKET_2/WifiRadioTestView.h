#pragma once
#include "WifiRadioTestConfig.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>

// Immutable presentation snapshot. The renderer never touches radio or saves.
namespace wifi_radio_ui {
enum class Action:uint8_t { None,Peer,Invite,Accept,Reject,Continue,Retry,Level,Distance,
  Reports,Previous,Next,Details,Back,Export,Exit,Cancel,Restore };
struct Line {int16_t y=0;uint16_t color=0;uint8_t scale=1;char text[40]{};};
struct Button {int16_t x=0,y=0,w=0,h=0;Action action=Action::None;uint8_t value=0;
  bool enabled=true,selected=false;char text[38]{};};
struct Screen {
  Line lines[19]{};Button buttons[12]{};uint8_t lineCount=0,buttonCount=0;
  bool clipped=false;
  void clear(){*this=Screen{};}
  void line(int y,const char* text,uint8_t scale=1,uint16_t color=0xef1b){
    if(lineCount>=19||scale<1||scale>3){clipped=true;return;}auto& l=lines[lineCount++];l.y=int16_t(y);l.scale=scale;l.color=color;
    const size_t maxChars=std::min(size_t(37/scale),sizeof(l.text)-1);
    if(std::strlen(text)>maxChars)clipped=true;
    const size_t n=std::min(maxChars,std::strlen(text));std::memcpy(l.text,text,n);l.text[n]=0;
  }
  void button(int x,int y,int w,int h,const char* text,Action action,uint8_t value=0,bool enabled=true,bool selected=false){
    if(buttonCount>=12){clipped=true;return;}auto& b=buttons[buttonCount++];
    b.x=x;b.y=y;b.w=w;b.h=h;b.action=action;b.value=value;b.enabled=enabled;b.selected=selected;
    const size_t maxChars=std::min(size_t(std::max(0,w-8)/6),sizeof(b.text)-1);
    if(std::strlen(text)>maxChars)clipped=true;
    const size_t n=std::min(maxChars,std::strlen(text));std::memcpy(b.text,text,n);b.text[n]=0;
  }
  const Button* hit(int x,int y)const{for(unsigned i=0;i<buttonCount;++i){const auto& b=buttons[i];
    if(b.enabled&&x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return &b;}return nullptr;}
};
inline Screen screen;
inline bool networkFromWifi=false;
inline char entryNotice[40]{};
template<class Canvas>void draw(Canvas& c,const Screen& s){
  constexpr uint16_t ink=0x1083,panel=0x1926,gold=0xd5aa,white=0xef1b,muted=0x9cf4,green=0x6e0c;
  c.fillScreen(ink);c.setTextWrap(false);
  for(unsigned i=0;i<s.lineCount;++i){const auto& l=s.lines[i];
    c.setTextSize(l.scale);c.setTextColor(l.color);c.setCursor((240-int(std::strlen(l.text))*6*l.scale)/2,l.y);c.print(l.text);}
  for(unsigned i=0;i<s.buttonCount;++i){const auto& b=s.buttons[i];
    c.fillRect(b.x,b.y,b.w,b.h,panel);c.drawRect(b.x,b.y,b.w,b.h,b.enabled?(b.selected?green:gold):muted);
    c.setTextSize(1);c.setTextColor(b.enabled?white:muted);
    c.setCursor(b.x+(b.w-int(std::strlen(b.text))*6)/2,b.y+(b.h-8)/2);c.print(b.text);}
}
}
