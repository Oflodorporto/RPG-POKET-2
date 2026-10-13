#pragma once
#include "AssetCatalog.h"
#include "Save.h"
#include "StorageDiagnostics.h"
#include <errno.h>
enum class ArtStatus:uint8_t {Fallback,Ready,Missing,Invalid,Memory};
// Reader size/read; exact header, size and CRC checked before publishing art.
template<class Reader> ArtStatus readArt(Reader& r,storageDiag::Failure* detail=nullptr){
  using storageDiag::Issue;if(detail)detail->issue=Issue::None;
  uint8_t h[16];errno=0;unsigned size=r.size();int code=errno;
  if(size!=ART_BYTES+16||code){storageDiag::set(detail,Issue::Size,0,ART_BYTES+16,size,code);return ArtStatus::Invalid;}
  errno=0;int got=r.read(h,16);code=errno;
  if(got!=16||code){storageDiag::set(detail,Issue::Read,0,16,got<0?0:unsigned(got),code);return ArtStatus::Invalid;}
  if(memcmp(h,"PKA1",4)){storageDiag::set(detail,Issue::Magic,0,0x31414b50,rpg::get32(h,0));return ArtStatus::Invalid;}
  if(rpg::get32(h,4)!=ART_BYTES){storageDiag::set(detail,Issue::Length,4,ART_BYTES,rpg::get32(h,4));return ArtStatus::Invalid;}
  if(rpg::get32(h,8)!=ART_CRC){storageDiag::set(detail,Issue::HeaderCrc,8,ART_CRC,rpg::get32(h,8));return ArtStatus::Invalid;}
  if(rpg::get32(h,12)!=ART_COUNT){storageDiag::set(detail,Issue::Catalog,12,ART_COUNT,rpg::get32(h,12));return ArtStatus::Invalid;}
  uint32_t c=~0u;for(unsigned off=0;off<ART_BYTES;){unsigned n=std::min<uint32_t>(4096u,ART_BYTES-off);errno=0;got=r.read(artMemory+off,n);code=errno;
    if(got!=int(n)||code){storageDiag::set(detail,Issue::Read,16+off,n,got<0?0:unsigned(got),code);makeFallback();return ArtStatus::Invalid;}
    for(unsigned i=0;i<n;++i){c^=artMemory[off+i];for(unsigned k=0;k<8;++k)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}off+=n;}
  if(~c!=ART_CRC){storageDiag::set(detail,Issue::Crc,16,ART_CRC,~c);makeFallback();return ArtStatus::Invalid;}return ArtStatus::Ready;
}
