#pragma once
#include "EnemyArtCatalog.h"
#include "Indexed4.h"
#include <algorithm>
namespace enemyArt {
enum class Pose:uint8_t {Idle,Anticipation,Attack,Hurt};
constexpr size_t WORKSPACE_PIXELS=80*86,WORKSPACE_BYTES=13760;
static_assert(WORKSPACE_BYTES==WORKSPACE_PIXELS*sizeof(uint16_t),"Enemy decode workspace size");
// Owned by setup, reused synchronously. Never retain a decoded view across another decode.
inline uint16_t* workspace=nullptr;
struct Frame {uint16_t asset,palette,w,h,stride;uint32_t bytes;int16_t anchorX,anchorY;Anchor anchor;};
struct Decoded {const uint16_t* pixels=nullptr;unsigned w=0,h=0;};
inline Frame body(unsigned id,Pose pose){
 if(id>=ENEMY_COUNT||unsigned(pose)>=POSE_COUNT)return {0,0,0,0,0,0,0,0,Anchor::Ground};
 const auto& e=enemies[id];return {e.poses[unsigned(pose)],e.palette,BODY_W,BODY_H,BODY_STRIDE,BODY_BYTES,e.anchorX,e.anchorY,e.anchor};
}
inline Frame effect(Family family,unsigned frame){
 if(unsigned(family)>=FAMILY_COUNT||frame>=FX_FRAMES)return {0,0,0,0,0,0,0,0,Anchor::Ground};
 return {effectFrames[unsigned(family)][frame],effectPalettes[unsigned(family)],FX_W,FX_H,FX_STRIDE,FX_BYTES,16,16,Anchor::Virtual};
}
inline bool valid(const Frame& f){
 if(!f.w||f.asset>=ART_COUNT||f.palette>=ART_COUNT||f.anchorX<0||f.anchorX>f.w||f.anchorY<0||f.anchorY>f.h)return false;
 const auto& data=assetMeta[f.asset];const auto& pal=assetMeta[f.palette];
 return ((f.w==BODY_W&&f.h==BODY_H&&f.stride==BODY_STRIDE&&f.bytes==BODY_BYTES)||(f.w==FX_W&&f.h==FX_H&&f.stride==FX_STRIDE&&f.bytes==FX_BYTES))&&
  data.w==f.stride&&data.h==f.h&&data.elem==1&&data.offset<=ART_BYTES&&f.bytes<=ART_BYTES-data.offset&&
  pal.w==16&&pal.h==1&&pal.elem==2&&pal.offset<=ART_BYTES&&32<=ART_BYTES-pal.offset;
}
inline Decoded decode(const Frame& f){
 if(!artMemory||!workspace||!valid(f))return {};
 if(!indexed4::decode(assetBytes(f.asset),f.bytes,assetBytes(f.palette),32,f.w,f.h,f.stride,workspace,WORKSPACE_PIXELS))return {};
 return {workspace,f.w,f.h};
}
struct Placement {int x,y,w,h,anchorX,anchorY;};
// Fit canonical frames uniformly. No opaque-bound cropping: ground and virtual anchors
// are unchanged across the four poses, including floating enemies and the crab.
inline Placement fit(const Frame& f,int x,int y,int boxW,int boxH){
 if(!f.w||!f.h||boxW<=0||boxH<=0)return {x,y,0,0,x,y};
 int w=boxW,h=boxW*f.h/f.w;if(h>boxH){h=boxH;w=boxH*f.w/f.h;}
 int left=x+(boxW-w)/2,top=y+boxH-h;
 return {left,top,w,h,left+f.anchorX*w/f.w,top+f.anchorY*h/f.h};
}
// Dungeon coordinates identify a world support point, not the canonical box center.
// Ground roots sit on the floor; Virtual roots retain their authored hover offset.
inline Placement fitWorld(const Frame& f,int center,int base,int boxW,int boxH){
 auto p=fit(f,0,0,boxW,boxH);if(!p.w||!p.h)return {center,base,0,0,center,base};
 p.x=center-f.anchorX*p.w/f.w;
 p.y=base-(f.anchor==Anchor::Ground?f.anchorY*p.h/f.h:p.h);
 p.anchorX=center;p.anchorY=p.y+f.anchorY*p.h/f.h;return p;
}
// Effects attach relative to the declared ground/virtual root, never opaque bounds.
inline int actionY(const Frame& f,const Placement& p){return p.anchorY-(f.anchor==Anchor::Ground?3*p.h/8:p.h/8);}
inline bool visible(unsigned x,unsigned y,unsigned opacity){
 static constexpr uint8_t bayer[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
 return opacity>=1000||(opacity&&bayer[(y&3)*4+(x&3)]*1000u<opacity*16u);
}
template<class C>inline void draw(C& c,Decoded image,Placement p,unsigned opacity=1000){
 if(!image.pixels||p.w<1||p.h<1||!opacity)return;
 for(int y=0;y<p.h;++y)for(int x=0;x<p.w;++x){
  auto color=image.pixels[size_t(y)*image.h/p.h*image.w+size_t(x)*image.w/p.w];
  if(color!=indexed4::transparent&&visible(x,y,opacity))c.fillRect(p.x+x,p.y+y,1,1,color);
 }
}
template<class C>inline void drawBody(C& c,unsigned id,Pose pose,int x,int y,int w,int h,unsigned opacity=1000){
 auto f=body(id,pose);auto p=fit(f,x,y,w,h);draw(c,decode(f),p,opacity);
}
template<class C>inline void drawEffect(C& c,Family family,unsigned frame,int cx,int cy,int size){
 auto f=effect(family,frame);auto p=fit(f,cx-size/2,cy-size/2,size,size);draw(c,decode(f),p);
}
}
