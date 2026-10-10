#pragma once
#include "AssetCatalog.h"
inline const uint16_t* worldEnemyFrame(unsigned id,unsigned pose){return reinterpret_cast<const uint16_t*>(assetBytes(480+(id>=25&&id<=28?id-25:0)*2+std::min(pose,1u)));}
