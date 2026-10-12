#pragma once
#include <stdint.h>
#include "AssetCatalog.h"
// Generated from the immutable original roster. Gameplay IDs stay 0..28.
namespace enemyArt {
enum class Family:uint8_t {Slash,Bite,Charge,Claw,Magic,Pulse,Crush};
enum class Anchor:uint8_t {Ground,Virtual};
struct EnemyDescriptor {uint16_t poses[4],palette;int16_t anchorX,anchorY;Anchor anchor;Family family;};
constexpr unsigned ENEMY_COUNT=29,POSE_COUNT=4,FAMILY_COUNT=7,FX_FRAMES=8;
constexpr unsigned BODY_W=80,BODY_H=86,BODY_STRIDE=40,BODY_BYTES=3440;
constexpr unsigned FX_W=32,FX_H=32,FX_STRIDE=16,FX_BYTES=512;
constexpr unsigned PALETTE_COLORS=16,PALETTE_BYTES=32;
constexpr uint16_t TRANSPARENT=0xf81f;
// Every supplied body faces left. Dimensions are logical; AssetMeta stores byte planes.
constexpr bool BODY_FACING_LEFT=true;
constexpr EnemyDescriptor enemies[ENEMY_COUNT]={
  {{92,93,94,95},600,48,82,Anchor::Ground,Family::Slash}, // 0: goblin
  {{96,97,98,99},601,40,82,Anchor::Ground,Family::Bite}, // 1: lobo
  {{100,101,102,103},602,48,82,Anchor::Ground,Family::Slash}, // 2: esqueleto
  {{104,105,106,107},603,48,82,Anchor::Ground,Family::Crush}, // 3: guardiao
  {{108,109,110,111},604,40,82,Anchor::Ground,Family::Charge}, // 4: javali
  {{112,113,114,115},605,48,60,Anchor::Virtual,Family::Magic}, // 5: espectro
  {{116,117,118,119},606,48,82,Anchor::Ground,Family::Slash}, // 6: saqueador
  {{120,121,122,401},607,48,82,Anchor::Ground,Family::Crush}, // 7: sentinela
  {{402,403,404,405},608,48,82,Anchor::Ground,Family::Magic}, // 8: arconte
  {{406,407,408,409},609,48,82,Anchor::Ground,Family::Charge}, // 9: vigia_ossudo
  {{410,411,412,413},610,40,82,Anchor::Ground,Family::Claw}, // 10: hipogrifo_jovem
  {{414,415,416,459},611,40,82,Anchor::Ground,Family::Claw}, // 11: hipogrifo
  {{460,461,462,463},612,40,82,Anchor::Ground,Family::Claw}, // 12: hipogrifo_marcado
  {{480,481,482,483},613,40,82,Anchor::Ground,Family::Claw}, // 13: hipogrifo_alfa
  {{484,485,486,487},614,40,82,Anchor::Ground,Family::Bite}, // 14: mimico_do_bosque
  {{488,489,490,491},615,40,82,Anchor::Ground,Family::Bite}, // 15: mimico_das_ruinas
  {{492,493,494,495},616,40,82,Anchor::Ground,Family::Bite}, // 16: mimico_do_porto
  {{496,497,498,499},617,40,82,Anchor::Ground,Family::Bite}, // 17: mimico_do_castelo
  {{500,501,502,503},618,48,82,Anchor::Ground,Family::Slash}, // 18: odran
  {{504,505,506,507},619,48,60,Anchor::Virtual,Family::Pulse}, // 19: coracao_do_veu
  {{508,509,510,511},620,40,82,Anchor::Ground,Family::Crush}, // 20: sentinela_coral
  {{512,513,514,515},621,40,60,Anchor::Virtual,Family::Magic}, // 21: arraia_runar
  {{516,517,518,519},622,48,82,Anchor::Ground,Family::Slash}, // 22: corsario_afogado
  {{520,521,522,523},623,48,82,Anchor::Ground,Family::Magic}, // 23: oraculo_abissal
  {{524,525,526,527},624,40,82,Anchor::Ground,Family::Crush}, // 24: thalvor
  {{528,529,530,531},625,55,82,Anchor::Ground,Family::Claw}, // 25: caranguejo_ferreo
  {{532,533,534,535},626,48,82,Anchor::Ground,Family::Magic}, // 26: mao_de_cinza
  {{536,537,538,539},627,40,82,Anchor::Ground,Family::Crush}, // 27: automato_dourado
  {{540,541,542,543},628,48,60,Anchor::Virtual,Family::Magic}, // 28: eco_do_veu
};
constexpr uint16_t effectFrames[FAMILY_COUNT][FX_FRAMES]={
  {544,545,546,547,548,549,550,551}, // slash
  {552,553,554,555,556,557,558,559}, // bite
  {560,561,562,563,564,565,566,567}, // charge
  {568,569,570,571,572,573,574,575}, // claw
  {576,577,578,579,580,581,582,583}, // magic
  {584,585,586,587,588,589,590,591}, // pulse
  {592,593,594,595,596,597,598,599}, // crush
};
constexpr uint16_t effectPalettes[FAMILY_COUNT]={629,630,631,632,633,634,635};
constexpr bool validPlane(unsigned id,unsigned w,unsigned h) {
  return id<ART_COUNT && assetMeta[id].w==w && assetMeta[id].h==h &&
    assetMeta[id].elem==1 && assetMeta[id].tw==1 && assetMeta[id].th==1 &&
    assetMeta[id].offset+w*h<=ART_BYTES &&
    assetMeta[id].fallback<sizeof(artFallback) && artFallback[assetMeta[id].fallback]==0;
}
constexpr bool validPalette(unsigned id) {
  return id<ART_COUNT && assetMeta[id].w==PALETTE_COLORS && assetMeta[id].h==1 &&
    assetMeta[id].elem==2 && assetMeta[id].tw==PALETTE_COLORS && assetMeta[id].th==1 &&
    assetMeta[id].offset+PALETTE_BYTES<=ART_BYTES &&
    assetMeta[id].fallback+PALETTE_BYTES<=sizeof(artFallback) &&
    artFallback[assetMeta[id].fallback]==0x1f && artFallback[assetMeta[id].fallback+1]==0xf8;
}
constexpr bool validCatalog() {
  if(ART_COUNT!=636 || sizeof(assetMeta)/sizeof(assetMeta[0])!=ART_COUNT) return false;
  unsigned end=0;
  for(unsigned i=0;i<ART_COUNT;++i) {
    const auto& m=assetMeta[i];
    if(m.offset!=end || !m.w || !m.h || !m.tw || !m.th || (m.elem!=1 && m.elem!=2)) return false;
    if(m.fallback+unsigned(m.tw)*m.th*m.elem>sizeof(artFallback)) return false;
    end+=unsigned(m.w)*m.h*m.elem;
    if(end>ART_BYTES) return false;
  }
  return end==ART_BYTES;
}
constexpr bool validDescriptors() {
  for(unsigned i=0;i<ENEMY_COUNT;++i) {
    const auto& e=enemies[i];
    if(e.palette!=600+i || !validPalette(e.palette) || unsigned(e.family)>=FAMILY_COUNT ||
       e.anchorX<0 || e.anchorX>=int(BODY_W) || e.anchorY<0 || e.anchorY>=int(BODY_H)) return false;
    for(unsigned p=0;p<POSE_COUNT;++p) {
      if(!validPlane(e.poses[p],BODY_STRIDE,BODY_H)) return false;
      for(unsigned j=0;j<i*POSE_COUNT+p;++j) if(enemies[j/POSE_COUNT].poses[j%POSE_COUNT]==e.poses[p]) return false;
    }
  }
  for(unsigned k=0;k<FAMILY_COUNT;++k) {
    if(effectPalettes[k]!=629+k || !validPalette(effectPalettes[k])) return false;
    for(unsigned f=0;f<FX_FRAMES;++f) if(effectFrames[k][f]!=544+k*FX_FRAMES+f ||
      !validPlane(effectFrames[k][f],FX_STRIDE,FX_H)) return false;
  }
  return true;
}
static_assert(BODY_W/2==BODY_STRIDE && BODY_STRIDE*BODY_H==BODY_BYTES,"Enemy index plane dimensions");
static_assert(FX_W/2==FX_STRIDE && FX_STRIDE*FX_H==FX_BYTES,"Effect index plane dimensions");
static_assert(validCatalog(),"Asset catalog bounds and non-overlap");
static_assert(validDescriptors(),"Typed compact enemy descriptor contract");
} // namespace enemyArt
