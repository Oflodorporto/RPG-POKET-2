#pragma once
#include "src/GFX/Arduino_GFX.h"
#include <esp_heap_caps.h>
#include <algorithm>
#include <cstring>
// Compose in 150 KiB of PSRAM, then send a complete frame to the display.
// No per-frame allocations. Every primitive retains Arduino_GFX clipping.
class FrameBuffer:public Arduino_GFX {
public:
  uint16_t* pixels=nullptr;void (*onChunk)()=nullptr;
  void checkpoint(int y){if(onChunk&&(y&7)==0)onChunk();}
  FrameBuffer():Arduino_GFX(240,320){}
  bool begin(int32_t speed=GFX_NOT_DEFINED) override {
    (void)speed;pixels=static_cast<uint16_t*>(heap_caps_malloc(240*320*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return pixels!=nullptr;
  }
  void writePixelPreclipped(int16_t x,int16_t y,uint16_t color) override {pixels[y*240+x]=color;}
  void writeFillRectPreclipped(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t color) override {
    for(int j=0;j<h;++j){std::fill_n(pixels+(y+j)*240+x,w,color);checkpoint(j);}
  }
  void draw16bitRGBBitmap(int16_t x,int16_t y,uint16_t* bitmap,int16_t w,int16_t h) override {
    if(x<0||y<0||x+w>240||y+h>320){Arduino_GFX::draw16bitRGBBitmap(x,y,bitmap,w,h);return;}
    for(int j=0;j<h;++j){memcpy(pixels+(y+j)*240+x,bitmap+j*w,w*2);checkpoint(j);}
  }
};
