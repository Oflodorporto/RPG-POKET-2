#pragma once
#include "Rules.h"
#include "RaceArt.h"
inline const char* raceName(unsigned n){const char* names[]={"Humano","Elfo","Anao","Orc"};return names[n%4];}
constexpr uint16_t outfitColors[]={0,0x001f,0xf800,0x07e0,0xffe0,0x781f,0xef7d,0x4208};
inline uint16_t outfitPixel(uint16_t pixel,uint8_t mask,uint8_t shirt,uint8_t pants){
 unsigned selected=mask==1?shirt:mask==2?pants:0;if(!selected||pixel==0xf81f)return pixel;
 uint16_t color=outfitColors[selected%8];unsigned light=std::max((pixel>>11)*255/31,std::max(((pixel>>5)&63)*255/63,(pixel&31)*255/31));
 return uint16_t((((color>>11)*light/255)<<11)|((((color>>5)&63)*light/255)<<5)|((color&31)*light/255));
}
inline uint16_t personalPixel(const rpg::Game& g,unsigned frame,unsigned index){unsigned r=g.race%4,c=g.p.cls%4,f=frame%7;return outfitPixel(reinterpret_cast<const uint16_t*>(assetBytes(raceAsset[r][c][f]))[index],assetBytes(raceMask[r][c][f])[index],g.shirt,g.trousers);}
template<class Canvas> void personalSprite(Canvas& c,const rpg::Game& g,int x,int y,unsigned frame=0,bool mirror=false){
 int w=frame==6?80:112,h=frame==6?66:120;uint16_t line[112];
 for(int row=0;row<h;++row){for(int col=0;col<w;++col)line[col]=personalPixel(g,frame,row*w+(mirror?w-1-col:col));int col=0;while(col<w){while(col<w&&line[col]==0xf81f)++col;int a=col;while(col<w&&line[col]!=0xf81f)++col;if(col>a)c.draw16bitRGBBitmap(x+a,y+row,line+a,col-a,1);}}
}
