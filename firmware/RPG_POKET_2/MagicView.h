#pragma once
#include "CampArt.h"
// Approved animated GIF frames replace code-drawn chevrons/lines.
template<class Canvas> void magicSprite(Canvas& c,const uint16_t* image,int iw,int ih,int x,int y,int w,int h){
  for(int py=0;py<h;++py)for(int px=0;px<w;++px){auto color=image[(py*ih/h)*iw+px*iw/w];if(color!=0xf81f&&x+px>=0&&x+px<240&&y+py>=0&&y+py<320)c.fillRect(x+px,y+py,1,1,color);}
}
template<class Canvas> void magicEffect(Canvas& c,const ViewState& v,bool dungeon){
  unsigned frame=std::min(11u,v.effectFrame*12/8),step=std::min(7u,v.effectFrame);
  if(v.effect==Effect::Lightning){int x=dungeon?78:v.effectOnHero?4:132;magicSprite(c,campArt::lightning[frame],80,112,x,18,88,140);return;}
  if(v.effect!=Effect::Projectile)return;
  // Small round fireball grows at the impact, with a clearly separate travel stage.
  unsigned travel=std::min(5u,step);int cx=dungeon?(v.effectOnHero?120:176-int(travel)*11):(v.effectOnHero?180-int(travel)*26:46+int(travel)*27);
  int cy=dungeon?(v.effectOnHero?45+int(travel)*14:138-int(travel)*14):105;
  int size=step<5?24:32+int(step-5)*10;
  magicSprite(c,campArt::fireball[frame],40,40,cx-size/2,cy-size/2,size,size);
}
