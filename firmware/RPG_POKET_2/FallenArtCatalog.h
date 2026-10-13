#pragma once
#include "AssetCatalog.h"
namespace fallenArt {
constexpr unsigned W=160,H=80,PIXELS=W*H;
struct Descriptor{unsigned color,mask;};
constexpr Descriptor layers[]={{648,649},{650,651},{652,653},{654,655},{656,657},{658,659},{660,661},{662,663},{664,665}};
constexpr bool validCatalog(){if(sizeof(layers)/sizeof(layers[0])!=9)return false;for(auto d:layers){if(d.color>=ART_COUNT||d.mask>=ART_COUNT)return false;auto c=assetMeta[d.color],m=assetMeta[d.mask];if(c.w!=W||c.h!=H||c.elem!=2||m.w!=W||m.h!=H||m.elem!=1)return false;}return true;}
static_assert(validCatalog(),"Typed fallen hero layers");
}
