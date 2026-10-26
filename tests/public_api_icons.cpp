#include "PublicApiIcons.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

struct Canvas {
  std::array<uint16_t,64*64> pixels{};
  unsigned calls=0;
  void fillRect(int x,int y,int w,int h,uint16_t color){
    assert(x>=0&&y>=0&&w>0&&h==1&&x+w<=64&&y+h<=64);
    assert(color!=publicApiIcons::TRANSPARENT);
    ++calls;
    for(int column=x;column<x+w;++column)pixels[y*64+column]=color;
  }
};

int main(){
  using namespace publicApiIcons;
  std::array<uint16_t,MAX_WORKSPACE_PIXELS+2> guarded{};
  guarded.front()=0x1234;guarded.back()=0xabcd;
  auto* workspace=guarded.data()+1;
  for(unsigned id=0;id<COUNT;++id)for(unsigned size:{24,32}){
    const auto icon=Icon(id);const auto f=frame(icon,size);
    assert(f.data&&f.size==size&&f.bytes==size*size/2);
    assert(decode(icon,size,workspace,MAX_WORKSPACE_PIXELS));
    unsigned opaque=0,transparent=0;
    for(unsigned i=0;i<size*size;++i){
      if(workspace[i]==TRANSPARENT)++transparent;else ++opaque;
      const unsigned x=i%size,y=i/size;
      if(x==0||y==0||x==size-1||y==size-1)assert(workspace[i]==TRANSPARENT);
    }
    assert(opaque>size&&transparent>size);
    assert(guarded.front()==0x1234&&guarded.back()==0xabcd);
    std::array<uint16_t,MAX_WORKSPACE_PIXELS> expectedPixels{};
    for(unsigned i=0;i<size*size;++i)expectedPixels[i]=workspace[i];
    Canvas canvas;
    assert(draw(canvas,icon,size,8,9,workspace,size));
    assert(canvas.calls>0&&canvas.calls<=opaque);
    for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x){
      uint16_t expected=0;
      if(x>=8&&x<8+size&&y>=9&&y<9+size){
        const auto pixel=expectedPixels[(y-9)*size+(x-8)];
        if(pixel!=TRANSPARENT)expected=pixel;
      }
      assert(canvas.pixels[y*64+x]==expected);
    }
    assert(!decode(icon,size,workspace,size*size-1));
  }
  for(unsigned size:{0u,1u,16u,23u,25u,31u,33u,80u,0xffffffffu})assert(!decode(Icon::Broadsword,size,workspace,MAX_WORKSPACE_PIXELS));
  for(unsigned id:{16,17,255})assert(!decode(Icon(id),32,workspace,MAX_WORKSPACE_PIXELS));
  assert(!decode(Icon::Campfire,32,nullptr,MAX_WORKSPACE_PIXELS));
  assert(!decode(Icon::Campfire,32,workspace,0));
  Canvas noDraw;
  assert(!draw(noDraw,Icon::Campfire,24,0,0,workspace,23)&&noDraw.calls==0);
  assert(!draw(noDraw,Icon::Campfire,24,0,0,nullptr,24)&&noDraw.calls==0);
  assert(!draw(noDraw,Icon::Count,32,0,0,workspace,MAX_WORKSPACE_PIXELS)&&noDraw.calls==0);
  assert(!draw(noDraw,Icon::Campfire,32,INT_MAX,0,workspace,MAX_WORKSPACE_PIXELS)&&noDraw.calls==0);
  assert(!draw(noDraw,Icon::Campfire,32,0,INT_MAX,workspace,MAX_WORKSPACE_PIXELS)&&noDraw.calls==0);
  std::cout<<"PASS: 32 indexed4 frames; real shared decoder; guarded workspace; bounds; transparent span renderer\n";
}
