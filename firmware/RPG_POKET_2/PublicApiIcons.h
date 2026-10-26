#pragma once
#include "Indexed4.h"
#include "PublicApiIconData.h"
#include <limits.h>

// Optional provider only: no existing UI or firmware translation unit includes this.
// Storage and decode use the same indexed4/RGB565 contract as existing enemy art.
namespace publicApiIcons {
constexpr unsigned MAX_WORKSPACE_PIXELS=32*32;
constexpr unsigned MAX_WORKSPACE_BYTES=MAX_WORKSPACE_PIXELS*sizeof(uint16_t);
struct Frame {const uint8_t* data=nullptr;unsigned size=0,bytes=0;};
inline Frame frame(Icon icon,unsigned size){
  const unsigned id=unsigned(icon);
  if(id>=COUNT)return {};
  if(size==24)return {plane24[id],24,sizeof(plane24[id])};
  if(size==32)return {plane32[id],32,sizeof(plane32[id])};
  return {};
}
inline bool decode(Icon icon,unsigned size,uint16_t* workspace,size_t capacity){
  const auto f=frame(icon,size);
  return f.data&&indexed4::decode(f.data,f.bytes,palette,sizeof(palette),
                                f.size,f.size,f.size/2,workspace,capacity);
}
// Drawing decodes one row at a time: 24/32 RGB565 pixels of caller-owned stack.
// Full-frame decode remains available to callers that need it. No heap or shared state.
// Keep labels beside these icons; the 24px silhouettes are intentionally simplified.
template<class Canvas>inline bool draw(Canvas& canvas,Icon icon,unsigned size,int x,int y,
                                      uint16_t* workspace,size_t capacity){
  const auto f=frame(icon,size);
  if(!f.data||x>INT_MAX-int(f.size)||y>INT_MAX-int(f.size))return false;
  if(!workspace||capacity<size)return false;
  for(unsigned row=0;row<size;++row){
    if(!indexed4::decode(f.data+row*(size/2),size/2,palette,sizeof(palette),
                         size,1,size/2,workspace,capacity))return false;
    unsigned column=0;
    while(column<size){
      const unsigned start=column;
      const uint16_t color=workspace[column++];
      while(column<size&&workspace[column]==color)++column;
      if(color!=TRANSPARENT)canvas.fillRect(x+int(start),y+int(row),int(column-start),1,color);
    }
  }
  return true;
}
static_assert(MAX_WORKSPACE_BYTES==2048,"Public icon decode workspace contract");
}
