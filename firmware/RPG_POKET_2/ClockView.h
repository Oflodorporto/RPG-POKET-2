#pragma once
#include "ClockArtData.h"

namespace clockArt {
inline uint16_t pixel(unsigned x,unsigned y){
  if(x>=W||y>=H)return 0;
  return palette[plane[y*W+x]];
}
// One stack row, no heap, decoder state, PSRAM dependency or SD I/O.
template<class C>void draw(C& c){
  uint16_t row[W];
  for(unsigned y=0;y<H;++y){
    for(unsigned x=0;x<W;++x)row[x]=pixel(x,y);
    c.draw16bitRGBBitmap(0,y,row,W,1);
  }
}
}

namespace clockUi {
// Kept separate from the time-adjust strip at y=274.
template<class C>void envelope(C& c,int x,int y,uint16_t color){
  c.fillRect(x,y,25,19,0xe6b7);c.drawRect(x,y,25,19,color);
  for(int i=0;i<12;++i){c.fillRect(x+i,y+i/2,1,1,0x8a65);c.fillRect(x+24-i,y+i/2,1,1,0x8a65);}
  c.fillRect(x+10,y+8,6,6,0x9800);c.fillRect(x+12,y+13,2,4,0x7800);
}
}

// Presentation only: idle entry, tap routing and physical backlight dimming
// remain with the existing Page::Clock controller and applyBrightness().
template<class C>void drawClock(C& c,const rpg::Game& g,unsigned frame=0){
  clockArt::draw(c);
  auto label=[&](int x,int y,int w,const char* text,uint16_t color=UI_GOLD,int size=1){
    touchLabel(c,x,y,w,text,color,size);
  };
  label(60,24,120,"EM DESCANSO");
  char time[6],seconds[5],date[11];
  if(menu.clockValid){
    snprintf(time,sizeof(time),"%.5s",menu.clockTime);
    snprintf(seconds,sizeof(seconds),"%.2s s",menu.clockTime+6);
    snprintf(date,sizeof(date),"%.10s",menu.clockDate);
  }else{
    memcpy(time,"--:--",sizeof(time));
    memcpy(seconds,"--",3);
    memcpy(date,"--/--/----",sizeof(date));
  }
  label(35,102,170,time,0xffbd,4);
  label(84,141,72,seconds);
  label(65,155,110,menu.clockValid?worldClock::name(menu.worldPeriod):"Sem hora",UI_MUTED);
  label(46,198,148,date,0x3942,2);
  if(menu.sessionStarted&&g.eventStage==1){
    clockUi::envelope(c,32,233,frame%8<4?UI_GOLD:UI_WHITE);
    label(65,232,142,"Carta da Guilda");
    label(65,247,142,"Toque para ler",UI_WHITE);
  }else{
    label(25,235,190,"Toque para voltar");
    label(25,250,190,menu.sessionStarted?"ao jogo":"ao titulo");
  }
  if(!menu.clockValid){
    c.fillRect(46,273,148,9,0x0843);
    label(46,274,148,"Ajuste hora / Wi-Fi");
  }
}
