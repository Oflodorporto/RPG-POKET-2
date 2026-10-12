#pragma once
#include "CampArt.h"
// Approved animated GIF frames replace code-drawn chevrons/lines.
template<class Canvas> void magicSprite(Canvas& c,const uint16_t* image,int iw,int ih,int x,int y,int w,int h){
  for(int py=0;py<h;++py)for(int px=0;px<w;++px){auto color=image[(py*ih/h)*iw+px*iw/w];if(color!=0xf81f&&x+px>=0&&x+px<240&&y+py>=0&&y+py<320)c.fillRect(x+px,y+py,1,1,color);}
}
// Mage pilot reuses existing indexed fireball frames and their blue/white mapping.
// Exact 180/540/660/900/1100 ms phases; only the local subject moves.
template<class Canvas>void magicDartsPilot(Canvas& c,const ViewState& v,bool dungeon,int originX=-1,int originY=-1,int contactX=-1,int contactY=-1){
 unsigned t=v.effectElapsedMs==UINT32_MAX?v.effectFrame*1100/8:v.effectElapsedMs;
 auto ring=[&](int cx,int cy,int r,uint16_t color){for(int y=-r;y<=r;++y)for(int x=-r;x<=r;++x){int d=x*x+y*y;if(d<=r*r&&d>=(r-1)*(r-1))c.fillRect(cx+x,cy+y,1,1,color);}};
 auto dart=[&](unsigned frame,int cx,int cy,int size){const auto* image=campArt::fireball[frame%12];for(int y=0;y<size;++y)for(int x=0;x<size;++x){auto color=image[(y*40/size)*40+x*40/size];if(color!=0xf81f)c.fillRect(cx-size/2+x,cy-size/2+y,1,1,(color&0x0400)?UI_WHITE:UI_BLUE);}};
 int targetX=contactX>=0?contactX:dungeon?120:181,targetY=contactY>=0?contactY:dungeon?86:100;
 if(t<180){int x=originX>=0?originX:dungeon?174:30,y=originY>=0?originY:dungeon?137:88;ring(x,y,3+t/60,UI_BLUE);c.fillRect(x-1,y-1,2,2,UI_WHITE);}
 else if(t<540){int q=t-180;for(int n=0;n<3;++n){int startX=originX>=0?originX:dungeon?176:103,startY=originY>=0?originY:dungeon?138:110;int x=startX+(targetX-startX)*q/360,y=startY+(targetY-startY)*q/360+(n-1)*(17-q*9/360);
   for(int k=3;k>0;--k)c.fillRect(x+(dungeon?1:-1)*k*4,y+(n-1)*k,2,1,k==1?UI_BLUE:0x2210);
   dart(t*12/1100+n*3,x,y,9);
  }}
 else if(t<780&&v.combatResult.injury()){
  int q=t-540;dart(t*12/1100,targetX,targetY,q<120?26:16);ring(targetX,targetY,5+q/15,q<120?UI_WHITE:UI_BLUE);
  static const int8_t dx[]={1,-1,0,0,1,-1,1,-1},dy[]={0,0,1,-1,1,1,-1,-1};
  for(int n=0;n<8;++n){int d=5+q/13;c.fillRect(targetX+dx[n]*d,targetY+dy[n]*d,2,q<120?2:1,n%3?UI_BLUE:UI_GOLD);}
 }
}
template<class Canvas> void magicEffect(Canvas& c,const ViewState& v,bool dungeon){
  if(v.effect==Effect::MagicDarts){magicDartsPilot(c,v,dungeon);return;}
  unsigned frame=std::min(11u,v.effectFrame*12/8),step=std::min(7u,v.effectFrame);
  if(v.effect==Effect::Lightning){int x=dungeon?78:v.effectOnHero?4:132;magicSprite(c,campArt::lightning[frame],80,112,x,18,88,140);return;}
  if(v.effect==Effect::MagicDarts||v.effect==Effect::FlameVolley||v.effect==Effect::FireBurst){
    int travel=std::min(5u,step),cx=dungeon?176-travel*11:46+travel*27,cy=dungeon?138-travel*14:105;
    unsigned count=v.effect==Effect::FireBurst?1:3;int size=v.effect==Effect::FireBurst?(step<5?28:46+int(step-5)*18):18+int(step>=5)*12;
    for(unsigned i=0;i<count;++i){int dy=(int(i)-int(count/2))*19;const auto* image=campArt::fireball[(frame+i*3)%12];
      for(int py=0;py<size;++py)for(int px=0;px<size;++px){auto color=image[(py*40/size)*40+px*40/size];int x=cx-size/2+px,y=cy-size/2+py+dy;if(color==0xf81f||x<0||x>=240||y<0||y>=(dungeon?172:192))continue;if(v.effect==Effect::MagicDarts)color=(color&0x0400)?UI_WHITE:UI_BLUE;c.fillRect(x,y,1,1,color);}}
    return;
  }
  if(v.effect==Effect::DivineSlash){
    int cx=dungeon?120:180,cy=dungeon?86:108,r=20+int(step)*2;
    for(int y=-r;y<=r;++y)for(int x=-r;x<=r;++x){
      int d=x*x+y*y,inside=(x+9)*(x+9)+(y-5)*(y-5);
      bool crescent=step<5&&d<=r*r&&inside>(r-5)*(r-5);
      bool burst=step>=4&&(x==0||y==0||x==y||x==-y)&&d<=(r-5)*(r-5);
      int px=cx+x,py=cy+y;
      if((crescent||burst)&&px>=0&&px<240&&py>=0&&py<(dungeon?172:192))c.fillRect(px,py,1,1,(d>(r-2)*(r-2)||burst)?UI_WHITE:UI_GOLD);
    }return;
  }
  if(v.effect==Effect::Radiant){int cx=dungeon?(v.effectOnHero?176:120):v.effectOnHero?46:180,cy=dungeon?(v.effectOnHero?135:86):108;int radius=8+int(step)*5;for(int k=0;k<4;++k){int r=radius-k*2;c.drawRect(std::max(0,cx-r),std::max(0,cy-r),r*2,r*2,k%2?UI_WHITE:UI_GOLD);}return;}
  if(v.effect!=Effect::Projectile)return;
  // Small round fireball grows at the impact, with a clearly separate travel stage.
  unsigned travel=std::min(5u,step);int cx=dungeon?(v.effectOnHero?120:176-int(travel)*11):(v.effectOnHero?180-int(travel)*26:46+int(travel)*27);
  int cy=dungeon?(v.effectOnHero?45+int(travel)*14:138-int(travel)*14):105;
  int size=step<5?24:32+int(step-5)*10;
  magicSprite(c,campArt::fireball[frame],40,40,cx-size/2,cy-size/2,size,size);
}
