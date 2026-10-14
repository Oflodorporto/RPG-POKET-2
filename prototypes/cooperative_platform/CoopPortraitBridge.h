#pragma once
#include "../cooperative_targeting/CoopTouchUi.h"
namespace coopplatform {
// Fixed rotation into the existing 240x320 portrait storage. Never call
// FrameBuffer::setRotation: its physical stride is hardcoded to 240.
struct Point {int16_t x=-1,y=-1;};
inline Point toPortrait(int32_t x,int32_t y){
 if(x<0||x>=320||y<0||y>=240)return {};
 return {int16_t(239-y),int16_t(x)};
}
inline Point toLandscape(int32_t x,int32_t y){
 if(x<0||x>=240||y<0||y>=320)return {};
 return {int16_t(y),int16_t(239-x)};
}
// Storage is borrowed; same 76,800 pixels, no second framebuffer or allocation.
// This primitive bridge is not yet a complete artwork/text renderer.
template<class PortraitCanvas> class LandscapeCanvas {
 PortraitCanvas& portrait;
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
 void draw16bitRGBBitmap(int32_t x,int32_t y,const uint16_t* pixels,int16_t w,int16_t h){
  if(!pixels||w<=0||h<=0)return;
  // Caller owns w*h source pixels. Destination is clipped before every write.
  for(int32_t row=0;row<h;++row)for(int32_t col=0;col<w;++col){
   int64_t dx=int64_t(x)+col,dy=int64_t(y)+row;
   if(dx>=0&&dx<320&&dy>=0&&dy<240)drawPixel(int32_t(dx),int32_t(dy),pixels[size_t(row)*w+col]);
  }
 }
};
// Bridges readTouch(valid, down, x, y), whose up sample has NO coordinates.
// Valid untouched samples must remain stable for 30ms, including initial entry.
class ContactSampler {
 bool armed=false,contact=false,releasing=false;uint32_t releaseAt=0;Point last{};
public:
 static constexpr uint32_t kReleaseMs=30;
 void reset(){*this=ContactSampler{};}
 void cancel(coopui::TouchUi& ui,coop::Endpoint& endpoint,uint32_t now){
  ui.onTouch(endpoint,false,-1,-1,now);reset();
 }
 void sample(coopui::TouchUi& ui,coop::Endpoint& endpoint,bool valid,bool down,int32_t px,int32_t py,uint32_t now){
  if(!valid){cancel(ui,endpoint,now);return;}
  if(down){
   Point p=toLandscape(px,py);if(p.x<0){cancel(ui,endpoint,now);return;}
   releasing=false;
   if(!contact){if(!armed)return;contact=true;armed=false;}
   last=p;ui.onTouch(endpoint,true,p.x,p.y,now);return;
  }
  if(!releasing){releasing=true;releaseAt=now;}
  if(uint32_t(now-releaseAt)<kReleaseMs)return;
  if(contact){ui.onTouch(endpoint,false,last.x,last.y,now);contact=false;}
  armed=true;
 }
};
}
