#pragma once
#include "AssetCatalog.h"
#include "Save.h"
enum class ArtStatus:uint8_t {Fallback,Ready,Missing,Invalid,Memory};
// Reader size/read; exact header, size and CRC checked before publishing art.
template<class Reader> ArtStatus readArt(Reader& r){
  uint8_t h[16];if(r.size()!=ART_BYTES+16||r.read(h,16)!=16)return ArtStatus::Invalid;
  if(memcmp(h,"PKA1",4)||rpg::get32(h,4)!=ART_BYTES||rpg::get32(h,8)!=ART_CRC||rpg::get32(h,12)!=ART_COUNT)return ArtStatus::Invalid;
  uint32_t c=~0u;for(unsigned off=0;off<ART_BYTES;){unsigned n=std::min<uint32_t>(4096u,ART_BYTES-off);if(r.read(artMemory+off,n)!=int(n)){makeFallback();return ArtStatus::Invalid;}
    for(unsigned i=0;i<n;++i){c^=artMemory[off+i];for(unsigned k=0;k<8;++k)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}off+=n;}
  if(~c!=ART_CRC){makeFallback();return ArtStatus::Invalid;}return ArtStatus::Ready;
}
