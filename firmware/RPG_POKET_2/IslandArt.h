#pragma once
#include "AssetCatalog.h"
namespace islandArt {
inline const uint16_t* sentinela_coral(){return reinterpret_cast<const uint16_t*>(assetBytes(459));}
inline const uint16_t* arraia_runar(){return reinterpret_cast<const uint16_t*>(assetBytes(460));}
inline const uint16_t* corsario_afogado(){return reinterpret_cast<const uint16_t*>(assetBytes(461));}
inline const uint16_t* oraculo_abisso(){return reinterpret_cast<const uint16_t*>(assetBytes(462));}
inline const uint16_t* thalvor(){return reinterpret_cast<const uint16_t*>(assetBytes(463));}
inline const uint16_t* enemy(unsigned id){const uint16_t* a[]={sentinela_coral(),arraia_runar(),corsario_afogado(),oraculo_abisso(),thalvor()};return a[id>=20&&id<=24?id-20:0];}
inline const uint16_t* f0_wall(){return reinterpret_cast<const uint16_t*>(assetBytes(464));}
inline const uint16_t* f0_floor(){return reinterpret_cast<const uint16_t*>(assetBytes(465));}
inline const uint16_t* f0_ceiling(){return reinterpret_cast<const uint16_t*>(assetBytes(466));}
inline const uint16_t* f0_door(){return reinterpret_cast<const uint16_t*>(assetBytes(467));}
inline const uint16_t* f1_wall(){return reinterpret_cast<const uint16_t*>(assetBytes(468));}
inline const uint16_t* f1_floor(){return reinterpret_cast<const uint16_t*>(assetBytes(469));}
inline const uint16_t* f1_ceiling(){return reinterpret_cast<const uint16_t*>(assetBytes(470));}
inline const uint16_t* f1_door(){return reinterpret_cast<const uint16_t*>(assetBytes(471));}
inline const uint16_t* f2_wall(){return reinterpret_cast<const uint16_t*>(assetBytes(472));}
inline const uint16_t* f2_floor(){return reinterpret_cast<const uint16_t*>(assetBytes(473));}
inline const uint16_t* f2_ceiling(){return reinterpret_cast<const uint16_t*>(assetBytes(474));}
inline const uint16_t* f2_door(){return reinterpret_cast<const uint16_t*>(assetBytes(475));}
inline const uint16_t* texture(unsigned f,unsigned t){const uint16_t* a[3][4]={{f0_wall(),f0_floor(),f0_ceiling(),f0_door()},{f1_wall(),f1_floor(),f1_ceiling(),f1_door()},{f2_wall(),f2_floor(),f2_ceiling(),f2_door()}};return a[std::min(2u,f)][std::min(3u,t)];}
inline const uint16_t* lever_off(){return reinterpret_cast<const uint16_t*>(assetBytes(476));}
inline const uint16_t* lever_on(){return reinterpret_cast<const uint16_t*>(assetBytes(477));}
inline const uint16_t* trap(){return reinterpret_cast<const uint16_t*>(assetBytes(478));}
inline const uint16_t* map_island(){return reinterpret_cast<const uint16_t*>(assetBytes(479));}
}
