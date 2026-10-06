#pragma once
#include "AssetCatalog.h"
constexpr unsigned regionFrames[4][4]={{401, 402, 403, 404}, {405, 406, 407, 408}, {409, 410, 411, 412}, {413, 414, 415, 416}};
inline const uint16_t* regionEnemyFrame(unsigned id,unsigned frame){return reinterpret_cast<const uint16_t*>(assetBytes(regionFrames[id>=4&&id<=7?id-4:0][frame%4]));}
