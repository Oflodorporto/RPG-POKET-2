#pragma once
#include "PersonalArt.h"
#include "CampArt.h"

// Art-directed, code-native cutout rig. Every head, garment, boot and parked
// weapon is sampled from this hero's existing race/class art and live masks.
// No new bitmap, PSRAM buffer, game state, random call, or per-frame allocation.
namespace campUi {
struct Part {int x,y,w,h;};
struct Point {int x,y;};
struct Rig {
 int left,right,top,hip,neck,cx,legSplit;
 Part weapon,shield;
 int armX,armY,skinX,skinY,sleeveX,sleeveY;
};
constexpr Rig rigs[4][4]={
 {{14,61,40,94,71,41,41,{60,45,18,67},{0,0,0,0},52,77,46,65,35,77},
  {13,44,56,99,80,30,29,{0,0,0,0},{39,84,16,24},40,84,35,76,38,87},
  {13,57,55,99,79,38,39,{55,77,23,23},{0,0,0,0},50,85,48,75,46,85},
  {18,60,45,99,72,39,40,{63,68,25,32},{0,0,0,0},52,79,52,69,37,78}},
 {{38,88,20,90,60,63,62,{17,15,24,98},{0,0,0,0},79,66,70,49,75,70},
  {33,72,29,89,62,52,52,{16,41,17,38},{71,64,22,35},66,68,61,55,63,67},
  {18,73,19,92,61,53,53,{79,2,26,47},{0,0,0,0},67,67,63,49,61,67},
  {17,66,17,91,60,42,43,{63,20,42,53},{0,0,0,0},57,66,49,49,43,68}},
 {{38,88,40,98,65,62,62,{18,32,20,80},{0,0,0,0},77,73,69,58,76,76},
  {37,76,42,98,68,58,58,{17,43,20,42},{76,73,25,37},71,74,65,61,70,73},
  {36,87,43,98,65,60,61,{17,1,22,111},{0,0,0,0},77,73,70,59,75,74},
  {17,74,40,98,67,47,48,{76,38,26,58},{0,0,0,0},67,73,61,59,61,78}},
 {{35,94,40,99,71,68,68,{25,35,23,82},{0,0,0,0},84,77,74,62,80,79},
  {39,81,26,99,68,62,63,{28,26,14,47},{79,68,26,42},75,72,69,62,72,74},
  {19,79,37,98,70,52,52,{84,17,12,94},{0,0,0,0},72,76,66,61,70,76},
  {24,82,27,98,66,56,57,{86,39,25,40},{0,0,0,0},75,72,69,60,69,75}}
};
constexpr int legBounds[4][4][3]={
 {{24,41,59},{20,31,49},{22,41,60},{21,43,66}},
 {{40,62,85},{36,55,78},{25,50,75},{28,50,73}},
 {{31,59,84},{32,57,82},{32,60,83},{27,54,83}},
 {{45,68,90},{38,60,84},{36,55,79},{36,63,87}}
};
constexpr uint16_t key=0xf81f;
inline uint16_t shade(uint16_t c,unsigned n,unsigned d=4){return uint16_t((((c>>11)*n/d)<<11)|((((c>>5)&63)*n/d)<<5)|((c&31)*n/d));}
// Integer affine mapping gives knees and ankles a genuine seated silhouette,
// rather than squeezing a standing sprite or alternating attack frames.
inline uint16_t partPixel(const rpg::Game& g,Part s,Point o,Point u,Point v,int x,int y){
 if(s.w<=0||s.h<=0)return key;
 int det=u.x*v.y-u.y*v.x,px=x-o.x,py=y-o.y;
 int a=px*v.y-py*v.x,b=u.x*py-u.y*px;
 if(det<0){det=-det;a=-a;b=-b;}
 if(!det||a<0||b<0||a>=det||b>=det)return key;
 return personalPixel(g,0,(s.y+b*s.h/det)*112+s.x+a*s.w/det);
}
inline void over(uint16_t& pixel,uint16_t next){if(next!=key)pixel=next;}
inline bool capsule(int x,int y,Point a,Point b,int radius){
 int dx=b.x-a.x,dy=b.y-a.y,l=dx*dx+dy*dy;
 int t=l?std::max(0,std::min(l,(x-a.x)*dx+(y-a.y)*dy)):0;
 int ex=(x-a.x)*std::max(1,l)-dx*t,ey=(y-a.y)*std::max(1,l)-dy*t;
 return int64_t(ex)*ex+int64_t(ey)*ey<=int64_t(radius)*radius*std::max(1,l)*std::max(1,l);
}
inline uint16_t material(const rpg::Game& g,int x,int y,uint16_t fallback){auto c=personalPixel(g,0,y*112+x);return c==key?fallback:c;}
inline unsigned mealPose(unsigned frame){return (frame/2)%12;}
inline Point foodHand(const Rig& r,unsigned frame,bool meal){
 // Reach, lift, hold for two visible bites, then lower to the lap.
 constexpr int lift[]={0,0,3,8,14,19,20,19,20,17,10,3};
 int n=meal?lift[mealPose(frame)]:0;
 const int mouthX=r.skinX+49-r.cx+2,mouthY=r.skinY+84-r.hip+9;
 return {74+(mouthX-74)*n/20,88+(mouthY-88)*n/20};
}
inline uint16_t seatedPixel(const rpg::Game& g,bool meal,unsigned frame,int x,int y){
 const auto& r=rigs[g.race%4][g.p.cls%4];
 const int dx=49-r.cx,dy=84-r.hip;
 const unsigned pose=mealPose(frame);const int chew=meal&&(pose==6||pose==8)?1:0;
 uint16_t pixel=key;
 // A freestanding oak stool, with two visible legs and a ground shadow.
 // Keep support below the folded boots, so no background lantern is a seat.
 if(y>=115&&y<119&&x>=16+abs(y-117)*7&&x<88-abs(y-117)*7)pixel=0x1082;
 if(y>=93&&y<117&&((x>=35&&x<42)||(x>=62&&x<69))){pixel=0x30c2;if(x==37||x==64)pixel=0xab25;else if(x==38||x==65)pixel=0x7243;}
 if(y>=109&&y<112&&x>=40&&x<65)pixel=y==109?0xab25:0x6243;
 if(y>=90&&y<97&&x>=29&&x<75){pixel=(y==90||y==96||x==29||x==74)?0x30c2:0x7243;if(y==92)pixel=0xbb85;}
 // Equipment rests alongside the seated hero, never held in a battle stance.
 if(r.weapon.w){int h=std::min(74,r.weapon.h);int w=std::max(8,r.weapon.w*h/r.weapon.h);over(pixel,partPixel(g,r.weapon,{9,112-h},{w,-4},{4,h},x,y));}
 if(r.shield.w)over(pixel,partPixel(g,r.shield,{5,81},{25,0},{3,31},x,y));
 // Folded far leg: thigh extends left, boot folds back under the near knee.
 const auto& legs=legBounds[g.race%4][g.p.cls%4];
 const int farW=legs[1]-legs[0],nearW=legs[2]-legs[1];
 const int thigh=std::max(4,(114-r.hip)/2),shin=114-r.hip-thigh;
 over(pixel,partPixel(g,{legs[0],r.hip,farW,thigh},{20,83},{22,3},{-3,13},x,y));
 over(pixel,partPixel(g,{legs[0],r.hip+thigh,farW,shin},{20,95},{22,2},{16,14},x,y));
 // Near bent knee and tucked boot; the original trouser/armor masks survive.
 over(pixel,partPixel(g,{legs[1],r.hip,nearW,thigh},{47,83},{28,-1},{12,14},x,y));
 over(pixel,partPixel(g,{legs[1],r.hip+thigh,nearW,shin},{57,96},{29,-2},{-7,14},x,y));
 // Upper body and face stay exact, including the elf ears, dwarf beard, orc
 // tusks, armor, cloak and racial coloration. Just the eating arm is re-rigged.
 int sx=x-dx,sy=y-dy+(y<r.neck+dy?chew:0);
 if(sx>=r.left&&sx<r.right&&sy>=r.top&&sy<r.hip){
  bool frontArm=sy>=r.neck&&sx>=r.armX+3;
  if(!frontArm)over(pixel,personalPixel(g,0,sy*112+sx));
 }
 const Point shoulder={r.armX+dx,r.armY+dy},elbow={shoulder.x+6,shoulder.y+13},hand=foodHand(r,frame,meal);
 uint16_t cloth=material(g,r.sleeveX,r.sleeveY,0x6b4d),skin=material(g,r.skinX,r.skinY,g.race==3?0x9d67:0xf5b5);
 // Pixel-art outlined sleeve, forearm and fingers, colored from this identity.
 if(capsule(x,y,shoulder,elbow,5)||capsule(x,y,elbow,hand,4))pixel=0x1082;
 if(capsule(x,y,shoulder,elbow,3))pixel=cloth;
 if(capsule(x,y,{shoulder.x-1,shoulder.y}, {elbow.x-1,elbow.y},1))pixel=shade(cloth,3);
 if(capsule(x,y,elbow,hand,2))pixel=(g.p.cls==1||g.p.cls==2)?cloth:skin;
 if(capsule(x,y,hand,hand,3))pixel=shade(skin,3);
 if(capsule(x,y,{hand.x,hand.y-1},{hand.x+1,hand.y-1},2))pixel=skin;
 if(meal){
  // A roasted ration from the existing camp art follows the hand to the mouth.
  const int fx=hand.x-8,fy=hand.y-14;
  if(x>=fx&&x<fx+19&&y>=fy&&y<fy+19)over(pixel,campArt::meat[((y-fy)*48/19)*48+(x-fx)*48/19]);
 }
 return pixel;
}
template<class Canvas>void hero(Canvas& c,const rpg::Game& g,int x,int y,int w,int h,bool meal=false,unsigned frame=0){
 for(int py=0;py<h;++py)for(int px=0;px<w;++px){auto color=seatedPixel(g,meal,frame,px*112/w,py*120/h);if(color!=key)c.fillRect(x+px,y+py,1,1,color);}
}
}
