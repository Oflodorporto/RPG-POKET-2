#pragma once
#include "FallenArtCatalog.h"
#include "PersonalArt.h"
namespace fallenArt {
inline uint16_t tint(uint16_t pixel,uint8_t mask,const rpg::Game& g){
 if(pixel==0xf81f)return pixel;if(mask<=2)return outfitPixel(pixel,mask,g.shirt,g.trousers);if(mask!=3||g.race!=3)return pixel;
 unsigned light=std::max((pixel>>11)*255/31,std::max(((pixel>>5)&63)*255/63,(pixel&31)*255/31));return uint16_t((((118*light/255)>>3)<<11)|(((156*light/255)>>2)<<5)|((40*light/255)>>3));
}
// Mage hair/beard uses the actual class palette. Explicit face intervals keep
// brown skin/eye/nose shading intact; the approved head silhouette is unchanged.
inline bool mageFace(unsigned race,unsigned x,unsigned y){
 constexpr uint8_t human[][2]={{30,36},{30,36},{30,36},{30,36},{30,36},{30,36},{26,37},{19,37},{18,37},{17,37},{17,37},{17,37},{17,47},{17,47},{17,47},{17,47},{17,47},{16,47},{16,47},{17,47},{17,47},{23,47},{23,47},{23,47},{23,47},{23,47},{23,47}};
 constexpr uint8_t dwarf[][2]={{21,24},{20,26},{20,26},{20,26},{13,26},{11,20},{10,22},{9,23},{9,23},{9,23},{8,23},{8,22},{8,21},{7,20},{7,19},{8,18},{8,18},{8,17},{9,16},{13,16},{14,16}};
 if(race==0&&y>=27&&y<=53)return x>=human[y-27][0]&&x<=human[y-27][1];
 if(race==2&&y>=25&&y<=45)return x>=dwarf[y-25][0]&&x<=dwarf[y-25][1];return false;
}
inline uint16_t mageHair(uint16_t pixel,unsigned layer,unsigned index,const rpg::Game& g){
 if(g.p.cls!=0||!((g.race==0&&layer==1)||(g.race==2&&layer==3))||pixel==0xf81f||mageFace(g.race,index%W,index/W))return pixel;
 unsigned r=(pixel>>11)*255/31,green=((pixel>>5)&63)*255/63,b=(pixel&31)*255/31;
 if(r<32||10*r<=13*green||5*r<=8*b)return pixel;
 unsigned v=g.race==0?std::min(242u,65+3*r/2):std::min(219u,50+4*r/5);
 unsigned gg=g.race==0?v-7:v,bb=g.race==0?v-9:v;return uint16_t(((v>>3)<<11)|((gg>>2)<<5)|(bb>>3));
}
inline uint16_t pixel(unsigned layer,unsigned index,const rpg::Game& g){if(!artMemory||layer>=9||index>=PIXELS)return 0xf81f;auto d=layers[layer];const auto* p=assetBytes(d.color)+index*2;return mageHair(tint(uint16_t(p[0])|(uint16_t(p[1])<<8),assetBytes(d.mask)[index],g),layer,index,g);}
template<class C>void draw(C& c,const rpg::Game& g,int x,int y){
 if(!artMemory||x<0||y<0||x>240-int(W)||y>320-int(H))return;
 uint16_t row[W];for(unsigned layer:{0u,5u+g.p.cls%4,1u+g.race%4})for(unsigned py=0;py<H;++py){for(unsigned px=0;px<W;++px)row[px]=pixel(layer,py*W+px,g);unsigned x0=0;while(x0<W){while(x0<W&&row[x0]==0xf81f)++x0;unsigned start=x0;while(x0<W&&row[x0]!=0xf81f)++x0;if(x0>start)c.draw16bitRGBBitmap(x+start,y+py,row+start,x0-start,1);}}
}
}
