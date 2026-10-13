#pragma once
#include "EnemyArt.h"
#include <limits.h>
// Presentation-only, fixed across poses. Bounds cover the union of all four
// authored silhouettes (including weapons), never a per-frame auto-crop.
// IDs, source artwork, Ground/Virtual roots and gameplay distance are unchanged.
namespace enemyScale {
struct Profile {uint8_t left,top,right,bottom,arenaWidth,arenaHeight,worldHeight;int8_t worldOffset;};
constexpr Profile profiles[enemyArt::ENEMY_COUNT]={
 {2,26,72,82,92,68,147,0}, // goblin: small humanoid
 {2,47,73,82,105,53,165,-8}, // wolf: long, low
 {2,22,69,83,103,90,172,0}, // skeleton
 {2,40,76,83,112,90,173,-8}, // guardian: broad stone mass
 {2,44,73,83,109,64,158,-8}, // boar
 {13,38,77,83,107,89,191,0}, // specter
 {2,36,67,82,103,88,195,0}, // raider
 {2,26,76,82,111,92,188,-8}, // sentinel
 {2,32,72,83,112,92,201,0}, // archon
 {3,39,64,83,106,90,212,0}, // bone watcher
 {2,39,66,82,95,70,177,-8}, // young hippogriff
 {2,41,70,82,107,83,173,-8}, // hippogriff
 {2,48,62,82,109,84,215,-8}, // marked hippogriff
 {2,39,71,83,112,92,201,-8}, // alpha
 {2,48,60,82,94,55,186,0}, // woodland mimic
 {2,37,65,82,99,65,168,0}, // ruins mimic
 {2,38,67,82,101,69,182,0}, // port mimic
 {2,33,67,82,106,77,189,-8}, // castle mimic
 {2,33,77,82,112,92,185,0}, // Odran
 {18,40,78,81,112,91,178,-8}, // heart of the veil
 {2,41,58,82,107,89,222,-8}, // coral sentinel
 {9,36,77,80,111,73,158,-8}, // runic ray
 {2,29,68,83,104,89,192,0}, // drowned corsair
 {19,30,77,82,108,92,175,0}, // abyssal oracle
 {2,24,67,83,112,92,186,-8}, // Thalvor
 {2,41,77,83,104,59,158,0}, // iron crab: low and wide
 {8,35,78,82,106,91,181,0}, // hand of ash
 {2,27,67,82,111,92,200,-8}, // golden automaton
 {21,37,74,82,107,90,210,0}, // echo of the veil
};
static_assert(sizeof(Profile)==8,"Scale profile is flash-only compact metadata");
constexpr const Profile& profile(unsigned id){return profiles[id<enemyArt::ENEMY_COUNT?id:0];}
inline int canonicalHeight(unsigned id,int width,int height){
 const auto& p=profile(id);
 return std::min(width*int(enemyArt::BODY_H)/(p.right-p.left),height*int(enemyArt::BODY_H)/(p.bottom-p.top));
}
inline enemyArt::Placement arena(unsigned id,const enemyArt::Frame& f,int motionX=0,int motionY=0){
 if(!f.w||!f.h)return {0,0,0,0,0,0};
 const auto& p=profile(id);int h=canonicalHeight(id,std::min(102,int(p.arenaWidth)),p.arenaHeight),w=h*f.w/f.h;
 // The full authored pose union fits to the right of the unchanged hero. A
 // seven-pixel recoil allowance keeps every pose inside the right scene edge.
 int x=129-p.left*w/f.w,y=179-p.bottom*h/f.h;
 return {x+motionX,y+motionY,w,h,x+motionX+f.anchorX*w/f.w,y+motionY+f.anchorY*h/f.h};
}
inline enemyArt::Placement world(unsigned id,const enemyArt::Frame& f,int center,int base,float depth){
 if(!f.w||!f.h||depth<.1f)return {center,base,0,0,center,base};
 const auto& p=profile(id);int h=std::min(350,int(p.worldHeight/depth)),w=h*f.w/f.h;
 // A tiny species-dependent lateral position within the same map cell clears
 // the unchanged foreground weapon. It projects with depth like the body.
 center+=int(p.worldOffset/depth);
 return enemyArt::fitWorld(f,center,base,w,h);
}
// Thin, restrained light edge separates the source silhouette from bright stone and dark
// corridors alike. Interior authored RGB565 colors remain pixel-exact.
inline bool opaque(enemyArt::Decoded image,int x,int y){return x>=0&&y>=0&&x<int(image.w)&&y<int(image.h)&&image.pixels[y*image.w+x]!=enemyArt::TRANSPARENT;}
inline uint16_t edgeColor(const enemyArt::Frame& f){return f.anchor==enemyArt::Anchor::Virtual?0x6c34:0x8c30;}
struct Contact {int x,y;};
template<class C>inline Contact drawArena(C& c,unsigned id,const enemyArt::Frame& f,enemyArt::Decoded image,enemyArt::Placement p,unsigned opacity){
 const auto& bounds=profile(id);Contact target{p.anchorX,p.y+(bounds.top+bounds.bottom)*p.h/(2*int(enemyArt::BODY_H))};
 int best=INT_MAX;if(!image.pixels||p.w<=0||p.h<=0||!opacity)return target;
 // Fixed union bounds trim only known transparent padding. One output pixel
 // around the union retains the rim; no per-frame crop or extra buffer.
 const int firstX=std::max(0,p.x+bounds.left*p.w/80-1),lastX=std::min(240,p.x+(bounds.right*p.w+79)/80+1);
 const int firstY=std::max(86,p.y+bounds.top*p.h/86-1),lastY=std::min(181,p.y+(bounds.bottom*p.h+85)/86+1);
 for(int y=firstY;y<lastY;++y)for(int x=firstX;x<lastX;++x){
  const int ix=(x-p.x)*image.w/p.w,iy=(y-p.y)*image.h/p.h;auto color=image.pixels[iy*image.w+ix];
  if(color!=enemyArt::TRANSPARENT){
   const int dx=x-p.anchorX,dy=y-(p.y+(bounds.top+bounds.bottom)*p.h/(2*int(enemyArt::BODY_H))),distance=dx*dx+dy*dy;
   if(distance<best){best=distance;target={x,y};}
   if(enemyArt::visible(ix,iy,opacity))c.fillRect(x,y,1,1,color);}
  else if(opaque(image,(x-1-p.x)*int(image.w)/p.w,iy)||opaque(image,(x+1-p.x)*int(image.w)/p.w,iy)||opaque(image,ix,(y-1-p.y)*int(image.h)/p.h)||opaque(image,ix,(y+1-p.y)*int(image.h)/p.h)){
   if(enemyArt::visible(ix,iy,opacity))c.fillRect(x,y,1,1,edgeColor(f));
  }
 }
 return target;
}
}
