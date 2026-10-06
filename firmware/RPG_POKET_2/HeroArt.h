#pragma once
#include <stdint.h>
#include "AssetCatalog.h"
#define hero_mago_0 (reinterpret_cast<const uint16_t*>(assetBytes(64)))
#define hero_mago_1 (reinterpret_cast<const uint16_t*>(assetBytes(65)))
#define hero_mago_2 (reinterpret_cast<const uint16_t*>(assetBytes(66)))
#define hero_mago_3 (reinterpret_cast<const uint16_t*>(assetBytes(67)))
#define hero_mago_4 (reinterpret_cast<const uint16_t*>(assetBytes(68)))
#define hero_mago_5 (reinterpret_cast<const uint16_t*>(assetBytes(69)))
inline const uint16_t* const* hero_mago_get(){static const uint16_t* const a[]={hero_mago_0,hero_mago_1,hero_mago_2,hero_mago_3,hero_mago_4,hero_mago_5};return a;}
#define hero_mago (hero_mago_get())
#define portrait_mago (reinterpret_cast<const uint16_t*>(assetBytes(70)))
#define hero_cavaleiro_0 (reinterpret_cast<const uint16_t*>(assetBytes(71)))
#define hero_cavaleiro_1 (reinterpret_cast<const uint16_t*>(assetBytes(72)))
#define hero_cavaleiro_2 (reinterpret_cast<const uint16_t*>(assetBytes(73)))
#define hero_cavaleiro_3 (reinterpret_cast<const uint16_t*>(assetBytes(74)))
#define hero_cavaleiro_4 (reinterpret_cast<const uint16_t*>(assetBytes(75)))
#define hero_cavaleiro_5 (reinterpret_cast<const uint16_t*>(assetBytes(76)))
inline const uint16_t* const* hero_cavaleiro_get(){static const uint16_t* const a[]={hero_cavaleiro_0,hero_cavaleiro_1,hero_cavaleiro_2,hero_cavaleiro_3,hero_cavaleiro_4,hero_cavaleiro_5};return a;}
#define hero_cavaleiro (hero_cavaleiro_get())
#define portrait_cavaleiro (reinterpret_cast<const uint16_t*>(assetBytes(77)))
#define hero_guerreiro_0 (reinterpret_cast<const uint16_t*>(assetBytes(78)))
#define hero_guerreiro_1 (reinterpret_cast<const uint16_t*>(assetBytes(79)))
#define hero_guerreiro_2 (reinterpret_cast<const uint16_t*>(assetBytes(80)))
#define hero_guerreiro_3 (reinterpret_cast<const uint16_t*>(assetBytes(81)))
#define hero_guerreiro_4 (reinterpret_cast<const uint16_t*>(assetBytes(82)))
#define hero_guerreiro_5 (reinterpret_cast<const uint16_t*>(assetBytes(83)))
inline const uint16_t* const* hero_guerreiro_get(){static const uint16_t* const a[]={hero_guerreiro_0,hero_guerreiro_1,hero_guerreiro_2,hero_guerreiro_3,hero_guerreiro_4,hero_guerreiro_5};return a;}
#define hero_guerreiro (hero_guerreiro_get())
#define portrait_guerreiro (reinterpret_cast<const uint16_t*>(assetBytes(84)))
#define hero_barbaro_0 (reinterpret_cast<const uint16_t*>(assetBytes(85)))
#define hero_barbaro_1 (reinterpret_cast<const uint16_t*>(assetBytes(86)))
#define hero_barbaro_2 (reinterpret_cast<const uint16_t*>(assetBytes(87)))
#define hero_barbaro_3 (reinterpret_cast<const uint16_t*>(assetBytes(88)))
#define hero_barbaro_4 (reinterpret_cast<const uint16_t*>(assetBytes(89)))
#define hero_barbaro_5 (reinterpret_cast<const uint16_t*>(assetBytes(90)))
inline const uint16_t* const* hero_barbaro_get(){static const uint16_t* const a[]={hero_barbaro_0,hero_barbaro_1,hero_barbaro_2,hero_barbaro_3,hero_barbaro_4,hero_barbaro_5};return a;}
#define hero_barbaro (hero_barbaro_get())
#define portrait_barbaro (reinterpret_cast<const uint16_t*>(assetBytes(91)))
inline const uint16_t* const* hero_portraits_get(){static const uint16_t* const a[]={portrait_mago,portrait_cavaleiro,portrait_guerreiro,portrait_barbaro};return a;}
#define hero_portraits (hero_portraits_get())
inline const uint16_t* const* const* hero_frames_get(){static const uint16_t* const* const a[]={hero_mago,hero_cavaleiro,hero_guerreiro,hero_barbaro};return a;}
#define hero_frames (hero_frames_get())