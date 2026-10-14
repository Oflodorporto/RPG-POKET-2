#pragma once
#include "ClockArtData.h"
#include "ClockCycle7.h"

namespace clockArt {
inline uint16_t pixel(unsigned x,unsigned y){
  if(x>=W||y>=H)return 0;
  return palette[plane[y*W+x]];
}
// One stack row, no heap, decoder state, PSRAM dependency or SD I/O.
template<class C>void draw(C& c,const clockCycle7::State& sky={},unsigned first=0,unsigned last=H){
  uint16_t row[W];
  for(unsigned y=first;y<last;++y){
    for(unsigned x=0;x<W;++x)row[x]=clockCycle7::skyPixel(pixel(x,y),x,y,sky);
    c.draw16bitRGBBitmap(0,y,row,W,1);
  }
}
}

namespace clockUi {
// Gilded medallion breathes in both area and luminance. The 6..9 px body
// and 8..11 px halo keep all orbit extremes inside the existing redraw band.
// Fixed-point radii avoid per-pixel floating point, sqrt, heap or art changes.
template<class C>void medallion(C& c,const clockCycle7::State& s){
  if(!s.valid)return;
  const uint16_t ink=0x0844;
  const uint16_t gold=clockCycle7::mix(clockCycle7::rgb(165,108,29),clockCycle7::rgb(255,231,142),s.pulse);
  const uint16_t light=clockCycle7::mix(clockCycle7::rgb(199,147,62),clockCycle7::rgb(255,239,174),s.pulse);
  const uint16_t moon=clockCycle7::mix(clockCycle7::rgb(151,173,192),clockCycle7::rgb(240,251,255),s.pulse);
  const unsigned body=6*256+3*s.pulse,halo=8*256+3*s.pulse;
  const unsigned body2=body*body,halo2=halo*halo;
  for(int dy=-clockCycle7::MEDALLION_RADIUS;dy<=clockCycle7::MEDALLION_RADIUS;++dy)
  for(int dx=-clockCycle7::MEDALLION_RADIUS;dx<=clockCycle7::MEDALLION_RADIUS;++dx){
    unsigned d=unsigned(dx*dx+dy*dy)*65536;if(d>=halo2)continue;
    int x=s.x+dx,y=s.y+dy;
    uint16_t color=clockCycle7::skyPixel(clockArt::pixel(x,y),x,y,s);
    if(d>=body2){
      unsigned alpha=(halo2-d)*(48+s.pulse/2)/(halo2-body2);
      color=clockCycle7::mix(color,s.night?moon:gold,alpha);
    }else{
      unsigned scaled=d*64/body2;
      if(scaled>48)color=dx+dy<0?light:gold;
      else if(scaled>36)color=0x51e4;
      else color=ink;
      int sx=(dx<0?-1:1)*int((unsigned(dx<0?-dx:dx)*8*256+body/2)/body);
      int sy=(dy<0?-1:1)*int((unsigned(dy<0?-dy:dy)*8*256+body/2)/body);
      if(scaled<=25){
        if(s.night){if((sx+2)*(sx+2)+(sy+2)*(sy+2)>20)color=moon;}
        else if(scaled<=8||sx==0||sy==0||sx==sy||sx==-sy)color=scaled<=8?light:gold;
      }
    }
    c.fillRect(x,y,1,1,color);
  }
}
template<class C>void starPixel(C& c,int x,int y,uint16_t color,unsigned alpha,const clockCycle7::State& s){
  if(!clockCycle7::sky(x,y))return;
  c.fillRect(x,y,1,1,clockCycle7::mix(clockCycle7::skyPixel(clockArt::pixel(x,y),x,y,s),color,alpha));
}
template<class C>void skyDetails(C& c,const clockCycle7::State& s,uint32_t now){
  if(!s.valid)return;
  if(s.glow){
    // Painted sunrise above the digits: a restrained sun and broken rays.
    for(int dy=-9;dy<=0;++dy)for(int dx=-13;dx<=13;++dx){
      int d=dx*dx+dy*dy;
      if(d<=64)starPixel(c,120+dx,99+dy,0xb4c7,s.glow*unsigned(80-d)/200,s);
      else if(d<160&&(dx==0||dx==dy*2||dx==-dy*2))starPixel(c,120+dx,99+dy,0x8b85,s.glow/4,s);
    }
  }
  static constexpr int stars[][2]={{78,84},{94,75},{114,83},{135,74},{153,83},{168,93},{60,98},{184,105},{52,116},{190,121},{66,136},{177,137}};
  for(unsigned i=0;i<12&&s.stars;++i){
    unsigned phase=(now+i*547u)&8191u,breath=phase<4096?phase:8192-phase;
    unsigned alpha=s.stars*(160+breath/64)/256;
    starPixel(c,stars[i][0],stars[i][1],0xce9a,alpha,s);
    if(i==2||i==4){starPixel(c,stars[i][0]-1,stars[i][1],0x9494,alpha/2,s);starPixel(c,stars[i][0]+1,stars[i][1],0x9494,alpha/2,s);}
  }
  if(s.meteor>=0){
    int x=85+s.meteor*72/1400,y=75+s.meteor*19/1400;
    unsigned fade=s.meteor<250?unsigned(s.meteor)*200/250:s.meteor>1100?unsigned(1400-s.meteor)*200/300:200;
    for(int i=0;i<13;++i)starPixel(c,x-i,y-i/4,0xce9a,fade*unsigned(13-i)/13,s);
  }
}
// Kept separate from the time-adjust strip at y=274.
template<class C>void envelope(C& c,int x,int y,uint16_t color){
  c.fillRect(x,y,25,19,0xe6b7);c.drawRect(x,y,25,19,color);
  for(int i=0;i<12;++i){c.fillRect(x+i,y+i/2,1,1,0x8a65);c.fillRect(x+24-i,y+i/2,1,1,0x8a65);}
  c.fillRect(x+10,y+8,6,6,0x9800);c.fillRect(x+12,y+13,2,4,0x7800);
}
}

// Presentation only: idle entry, tap routing and physical backlight dimming
// remain with the existing Page::Clock controller and applyBrightness().
template<class C>void drawClock(C& c,const rpg::Game& g,unsigned frame=0,bool faceOnly=false){
  const auto cycle=clockCycle7::state(menu.clockTime,menu.clockValid,menu.renderNow);
  clockArt::draw(c,cycle,faceOnly?clockCycle7::FACE_TOP:0,faceOnly?clockCycle7::FACE_BOTTOM:clockArt::H);
  clockUi::skyDetails(c,cycle,menu.renderNow);
  clockUi::medallion(c,cycle);
  auto label=[&](int x,int y,int w,const char* text,uint16_t color=UI_GOLD,int size=1){
    touchLabel(c,x,y,w,text,color,size);
  };
  if(!faceOnly)label(60,24,120,"EM DESCANSO");
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
  label(65,155,110,menu.clockValid?worldClock::name(cycle.valid?cycle.period:menu.worldPeriod):"Sem hora",UI_MUTED);
  if(faceOnly)return;
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
