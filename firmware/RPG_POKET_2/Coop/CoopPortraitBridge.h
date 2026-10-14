#pragma once
#include "CoopTouchSampler.h"
#include "../src/GFX/font/glcdfont.h"
namespace coopplatform {
// Storage is borrowed; same 76,800 pixels, no second framebuffer or allocation.
// Text is rasterized in logical landscape coordinates before mapping each pixel.
// Forwarding setRotation/text to the portrait GFX would corrupt the physical stride.
template<class PortraitCanvas> class LandscapeCanvas {
 PortraitCanvas& portrait;
 int32_t cursorX=0,cursorY=0;uint16_t textColor=0xffff;uint8_t textSize=1;bool wrap=false;
public:
 explicit LandscapeCanvas(PortraitCanvas& p):portrait(p){}
 void drawPixel(int32_t x,int32_t y,uint16_t color){auto p=toPortrait(x,y);if(p.x>=0)portrait.drawPixel(p.x,p.y,color);}
 void fillRect(int32_t x,int32_t y,int32_t w,int32_t h,uint16_t color){
  if(w<=0||h<=0)return;
  // Wide arithmetic ensures malformed geometry cannot overflow before clipping.
  int64_t x2=std::min<int64_t>(320,int64_t(x)+w),y2=std::min<int64_t>(240,int64_t(y)+h);
  x=std::max<int32_t>(0,x);y=std::max<int32_t>(0,y);
  if(x>=x2||y>=y2)return;
  portrait.fillRect(int16_t(240-y2),int16_t(x),int16_t(y2-y),int16_t(x2-x),color);
 }
 int16_t width()const{return 320;}int16_t height()const{return 240;}
 void fillScreen(uint16_t color){fillRect(0,0,320,240,color);}
 void drawRect(int32_t x,int32_t y,int32_t w,int32_t h,uint16_t color){
  if(w<=0||h<=0)return;
  fillRect(x,y,w,1,color);fillRect(x,y,1,h,color);
  const int64_t right=int64_t(x)+w-1,bottom=int64_t(y)+h-1;
  if(right>=0&&right<320)fillRect(int32_t(right),y,1,h,color);
  if(bottom>=0&&bottom<240)fillRect(x,int32_t(bottom),w,1,color);
 }
 void setCursor(int32_t x,int32_t y){cursorX=x;cursorY=y;}
 int32_t getCursorX()const{return cursorX;}int32_t getCursorY()const{return cursorY;}
 uint8_t getTextSize()const{return textSize;}
 void setTextColor(uint16_t color){textColor=color;}
 void setTextSize(unsigned size){textSize=uint8_t(std::max(1u,std::min(4u,size)));}
 void setTextWrap(bool value){wrap=value;}
 void print(const char* value){
  if(!value)return;
  for(;*value;++value){const unsigned ch=uint8_t(*value);
   if(ch=='\r')continue;
   if(ch=='\n'||(wrap&&int64_t(cursorX)+6*textSize>320)){
    cursorX=0;cursorY=int32_t(std::min<int64_t>(INT32_MAX,int64_t(cursorY)+8*textSize));
    if(ch=='\n')continue;
   }
   for(unsigned col=0;col<5;++col){const uint8_t bits=font[ch*5+col];
    for(unsigned row=0;row<8;++row)if(bits&(1u<<row)){
     const int64_t x=int64_t(cursorX)+col*textSize,y=int64_t(cursorY)+row*textSize;
     if(x>=-int(textSize)&&x<320&&y>=-int(textSize)&&y<240)fillRect(int32_t(x),int32_t(y),textSize,textSize,textColor);
    }
   }
   cursorX=int32_t(std::min<int64_t>(INT32_MAX,int64_t(cursorX)+6*textSize));
  }
 }
 void draw16bitRGBBitmap(int32_t x,int32_t y,const uint16_t* pixels,int16_t w,int16_t h){
  if(!pixels||w<=0||h<=0)return;
  // Caller owns w*h source pixels. Destination is clipped before every write.
  for(int32_t row=0;row<h;++row)for(int32_t col=0;col<w;++col){
   int64_t dx=int64_t(x)+col,dy=int64_t(y)+row;
   if(dx>=0&&dx<320&&dy>=0&&dy<240)drawPixel(int32_t(dx),int32_t(dy),pixels[size_t(row)*w+col]);
  }
 }
};
}
