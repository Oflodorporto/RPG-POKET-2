#pragma once
#include "BagSkinData.h"
// Local typography only. One extra horizontal stroke, fitted before drawing.
template<class C>void touchLabel(C& c,int x,int y,int w,const char* s,uint16_t color=0xffff,int size=1){
 if(w<7||y<0||y+8*size>320)return;if(int(strlen(s))*6*size+1>w)size=1;
 char text[64];snprintf(text,sizeof(text),"%.*s",std::min(63,(w-1)/(6*size)),s);int left=x+(w-int(strlen(text))*6*size-1)/2;
 c.setTextWrap(false);c.setTextSize(size);c.setTextColor(color);c.setCursor(left+1,y);c.print(text);c.setCursor(left,y);c.print(text);
}
template<class C>void touchLines(C& c,int x,int y,int w,const char* s,uint16_t color){
 if(w<7)return;unsigned limit=std::min(63,(w-1)/6);char line[64];unsigned row=0;
 while(*s&&row<2){unsigned n=0;while(s[n]&&n<limit)++n;unsigned take=n;
  if(s[n])for(unsigned i=n;i>0;--i)if(s[i-1]==' '){take=i-1;break;}
  if(!take)take=n;memcpy(line,s,take);line[take]=0;touchLabel(c,x,y+row*11,w,line,color);s+=take;while(*s==' ')++s;++row;
 }
}
namespace touchUi {
constexpr scenicUi::Rect gearPrevious={10,198,68,44},gearNext={162,198,68,44};
constexpr scenicUi::Rect gearBack={10,276,70,44},gearEquip={84,276,70,44},gearSell={158,276,72,44};
}
namespace bagSkin {
inline uint16_t pixel(unsigned x,unsigned y){if(x>=240||y>=320)return 0;unsigned i=y*240+x;return palette[(plane[i/2]>>(i&1?0:4))&15];}
// Fixed stack row. No heap/SD read and no shared decoder state.
template<class C>void crop(C& c,int sx,int sy,int sw,int sh,int x,int y,int w,int h){
 if(sx<0||sx>=240||sy<0||sy>=320||sw<=0||sw>240-sx||sh<=0||sh>320-sy||x<0||y<0||w<=0||w>240||h<=0||h>320||x>240-w||y>320-h)return;
 uint16_t row[240];for(int py=0;py<h;++py){for(int px=0;px<w;++px)row[px]=pixel(sx+px*sw/w,sy+py*sh/h);c.draw16bitRGBBitmap(x,y+py,row,w,1);}
}
template<class C>void leather(C& c,int x,int y,int w,int h){c.fillRect(x,y,w,h,0x3942);for(int py=0;py<h;++py)for(int px=0;px<w;++px)if(((px*17+py*29) & 31)==0)c.fillRect(x+px,y+py,1,1,0x49c3);}
template<class C>void frame(C& c,const scenicUi::Rect& r){crop(c,51,50,41,41,r.x,r.y,r.w,r.h);}
}
