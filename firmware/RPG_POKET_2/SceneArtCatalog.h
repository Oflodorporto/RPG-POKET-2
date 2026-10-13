#pragma once
#include "AssetCatalog.h"
namespace sceneArt {
struct Descriptor {unsigned plane,palette;};
enum class Id:unsigned {mares,aurora,preparar_viagem,levelup,defeat_solo,defeat_escort,Count};
constexpr Descriptor catalog[]={{636,637},{638,639},{640,641},{642,643},{644,645},{646,647}};
constexpr bool validCatalog(){for(auto d:catalog){if(d.plane>=ART_COUNT||d.palette>=ART_COUNT)return false;auto a=assetMeta[d.plane],p=assetMeta[d.palette];if(a.w!=240||a.h!=320||a.elem!=1||p.w!=128||p.h!=2||p.elem!=2)return false;}return true;}
static_assert(validCatalog(),"Typed scene geometry");
}
