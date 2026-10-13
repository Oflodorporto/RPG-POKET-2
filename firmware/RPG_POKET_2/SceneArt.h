#pragma once
#include "SceneArtCatalog.h"
// All scene pixels are opaque; palette words are read explicitly as little-endian.
// Rendering uses one480-byte stack row, never heap allocations or SD access.
namespace sceneArt {
inline bool valid(Id id){return unsigned(id)<unsigned(Id::Count);}
inline uint16_t pixel(Id id,unsigned x,unsigned y){
 if(!artMemory||!valid(id)||x>=240||y>=320)return 0;
 const auto d=catalog[unsigned(id)];unsigned index=assetBytes(d.plane)[y*240+x];const auto* p=assetBytes(d.palette)+index*2;return uint16_t(p[0])|(uint16_t(p[1])<<8);
}
template<class C>void draw(C& c,Id id){if(!artMemory||!valid(id))return;uint16_t row[240];for(unsigned y=0;y<320;++y){for(unsigned x=0;x<240;++x)row[x]=pixel(id,x,y);c.draw16bitRGBBitmap(0,y,row,240,1);}}
}
