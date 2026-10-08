#pragma once
#include "CampArt.h"
// Approved animated GIF frames replace code-drawn chevrons/lines.
template<class Canvas> void magicSprite(Canvas& c,const uint16_t* image,int iw,int ih,int x,int y,int w,int h){
  for(int py=0;py<h;++py)for(int px=0;px<w;++px){auto color=image[(py*ih/h)*iw+px*iw/w];if(color!=0xf81f&&x+px>=0&&x+px<240&&y+py>=0&&y+py<320)c.fillRect(x+px,y+py,1,1,color);}
}
template<class Canvas> void magicEffect(Canvas& c,const ViewState& v,bool dungeon){
  unsigned frame=std::min(11u,v.effectFrame*12/8),step=std::min(7u,v.effectFrame);
  if(v.effect==Effect::Lightning){int x=dungeon?78:v.effectOnHero?4:132;magicSprite(c,campArt::lightning[frame],80,112,x,18,88,140);return;}
  if(v.effect==Effect::MagicDarts||v.effect==Effect::FlameVolley||v.effect==Effect::FireBurst){
    int travel=std::min(5u,step),cx=dungeon?176-travel*11:46+travel*27,cy=dungeon?138-travel*14:105;
    unsigned count=v.effect==Effect::FireBurst?1:3;int size=v.effect==Effect::FireBurst?(step<5?28:46+int(step-5)*18):18+int(step>=5)*12;
    for(unsigned i=0;i<count;++i){int dy=(int(i)-int(count/2))*19;const auto* image=campArt::fireball[(frame+i*3)%12];
      for(int py=0;py<size;++py)for(int px=0;px<size;++px){auto color=image[(py*40/size)*40+px*40/size];int x=cx-size/2+px,y=cy-size/2+py+dy;if(color==0xf81f||x<0||x>=240||y<0||y>=(dungeon?172:192))continue;if(v.effect==Effect::MagicDarts)color=(color&0x0400)?UI_WHITE:UI_BLUE;c.fillRect(x,y,1,1,color);}}
    return;
  }
  if(v.effect==Effect::Radiant){int cx=dungeon?120:v.effectOnHero?46:180,cy=dungeon?86:108;int radius=8+int(step)*5;for(int k=0;k<4;++k){int r=radius-k*2;c.drawRect(std::max(0,cx-r),std::max(0,cy-r),r*2,r*2,k%2?UI_WHITE:UI_GOLD);}return;}
  if(v.effect!=Effect::Projectile)return;
  // Small round fireball grows at the impact, with a clearly separate travel stage.
  unsigned travel=std::min(5u,step);int cx=dungeon?(v.effectOnHero?120:176-int(travel)*11):(v.effectOnHero?180-int(travel)*26:46+int(travel)*27);
  int cy=dungeon?(v.effectOnHero?45+int(travel)*14:138-int(travel)*14):105;
  int size=step<5?24:32+int(step-5)*10;
  magicSprite(c,campArt::fireball[frame],40,40,cx-size/2,cy-size/2,size,size);
}
