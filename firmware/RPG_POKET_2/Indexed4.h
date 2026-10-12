#pragma once
#include <stdint.h>
#include <stddef.h>
// Storage-only codec. No allocation, filesystem access, timing or game state.
namespace indexed4 {
constexpr uint16_t transparent=0xf81f;
inline uint16_t little16(const uint8_t* p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
inline bool decode(const uint8_t* data,size_t bytes,const uint8_t* palette,size_t paletteBytes,
                   unsigned width,unsigned height,unsigned stride,uint16_t* out,size_t capacity){
  // Bounded by the largest approved enemy frame, including all intermediate arithmetic.
  if(!data||!palette||!out||!width||width>80||!height||height>86||stride<(width+1)/2||stride>40||
     paletteBytes!=32||capacity<size_t(width)*height||bytes!=size_t(stride)*height||little16(palette)!=transparent)return false;
  for(unsigned i=1;i<16;++i)if(little16(palette+2*i)==transparent)return false;
  for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
    uint8_t byte=data[size_t(y)*stride+x/2];unsigned index=(x&1)?byte&15:byte>>4;
    out[size_t(y)*width+x]=index?little16(palette+index*2):transparent;
  }
  return true;
}
}
