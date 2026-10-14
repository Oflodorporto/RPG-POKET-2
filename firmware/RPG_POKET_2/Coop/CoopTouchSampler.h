#pragma once
#include "CoopTouchUi.h"
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
