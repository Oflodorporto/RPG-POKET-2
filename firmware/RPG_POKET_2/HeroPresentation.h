#pragma once
#include "Rules.h"
#include "CombatPresentation.h"
// One identity per actual gesture. This is not a second combat rules engine.
enum class HeroChoreography:uint8_t {None,StaffBolt,LegacyLightning,MagicDarts,FlameCone,ScorchingRays,Fireball,ArcaneGuard,SwordCut,SwordPower,DivineSmite,ShieldGuard,LayHands,SacredWeapon,TurnUndead,SpearThrust,PrecisionThrust,SpearGuard,SecondWind,ActionSurge,AxeCut,RageStrike,AxeGuard,RageBuff};
inline HeroChoreography heroChoreography(unsigned cls,const CombatResult& r){
 if(!r.hasHeroAction||r.enemyAction)return HeroChoreography::None;
 using A=rpg::Action;using H=HeroChoreography;
 switch(r.heroAction){
 case A::Attack:return cls==0?H::StaffBolt:cls==1?H::SwordCut:cls==2?H::SpearThrust:H::AxeCut;
 case A::Offensive:return cls==0?H::LegacyLightning:cls==1?H::SwordPower:cls==2?H::PrecisionThrust:H::RageStrike;
 case A::Defensive:return cls==0?H::ArcaneGuard:cls==1?H::ShieldGuard:cls==2?H::SpearGuard:H::AxeGuard;
 case A::MagicMissile:return H::MagicDarts;case A::BurningHands:return H::FlameCone;case A::ScorchingRay:return H::ScorchingRays;case A::Fireball:return H::Fireball;case A::ShieldSpell:return H::ArcaneGuard;
 case A::DivineSmite:return H::DivineSmite;case A::LayHands:return H::LayHands;case A::SacredWeapon:return H::SacredWeapon;case A::TurnUndead:return H::TurnUndead;
 case A::SecondWind:return H::SecondWind;case A::ActionSurge:return H::ActionSurge;case A::RagePower:return H::RageBuff;
 default:return H::None; // Potions and escape have no animation in the dispatcher.
 }
}
struct HeroPose {uint8_t heroFrame=0;int8_t x=0,y=0;};
inline HeroPose heroPose(unsigned cls,const CombatResult& r,unsigned p,unsigned ms){
 using H=HeroChoreography;auto h=heroChoreography(cls,r);if(h==H::None||p>=1000)return {};
 if(h==H::MagicDarts){auto m=magePose(ms);return {m.heroFrame,m.heroX,0};}
 // All paths recover to the exact original pivot; never move the HUD.
 unsigned phase=p<200?0:p<491?1:p<650?2:p<850?3:4;
 if(h==H::LayHands)return {0,0,int8_t(phase<2?2:phase==2?1:0)};
 if(h==H::SecondWind)return {uint8_t(phase<2?5:0),int8_t(phase<2?-2:0),int8_t(phase<2?3:0)};
 if(h==H::SacredWeapon)return {uint8_t(phase<3?2:phase==3?1:0),0,0};
 if(h==H::TurnUndead)return {uint8_t(phase<2?1:phase<4?3:0),int8_t(phase==2?2:0),0};
 if(h==H::ActionSurge)return {uint8_t(phase<2?1:phase<4?5:0),int8_t(phase==2?3:phase==3?1:0),0};
 if(h==H::RageBuff)return {uint8_t(phase<2?1:phase<4?2:0),0,int8_t(phase<2?2:0)};
 if(h==H::ArcaneGuard||h==H::ShieldGuard||h==H::SpearGuard||h==H::AxeGuard)return {uint8_t(h==H::ArcaneGuard&&phase<4?1:0),int8_t(phase<4?-2:0),int8_t(h==H::AxeGuard&&phase<4?2:0)};
 if(h==H::FlameCone)return {uint8_t(phase<2?1:phase<4?2:5),int8_t(phase==2?3:0),0};
 if(h==H::ScorchingRays)return {uint8_t(phase==0?1:phase<4?3:5),int8_t(phase==0?-2:phase<3?2:0),0};
 if(h==H::StaffBolt||h==H::Fireball||h==H::LegacyLightning)return {uint8_t(phase==0?1:phase==1?2:phase==2?3:phase==3?4:5),int8_t(phase==0?-2:phase==2?3:0),0};
 if(h==H::SpearThrust||h==H::PrecisionThrust){static const uint8_t frames[]={1,1,3,4,5};static const int8_t dx[]={-4,-2,9,3,0};return {frames[phase],int8_t(dx[phase]+(h==H::PrecisionThrust&&phase==2?2:0)),0};}
 if(h==H::AxeCut||h==H::RageStrike){static const uint8_t frames[]={1,2,3,4,5};static const int8_t dx[]={-4,-3,7,3,0};return {frames[phase],dx[phase],int8_t(phase<2?2:0)};}
 static const uint8_t frames[]={1,2,3,4,5};static const int8_t dx[]={-2,-1,6,2,0};return {frames[phase],int8_t(dx[phase]+(h==H::SwordPower&&phase==2?2:0)),0};
}
// Crops refer to the unchanged RGB565 64x52 user-authored weapon images.
// Transparent padding is excluded; foreground stays on the right and exits at y=172.
struct DungeonHeroPose {uint8_t frame=0;int16_t x=0,y=0,w=0,h=0;int16_t originX=0,originY=0,palmX=0,palmY=0;};
struct DungeonWeaponCrop {uint8_t x,y,w,h;};
constexpr DungeonWeaponCrop dungeonWeaponCrops[4][2]={{{22,3,29,46},{9,6,45,42}},{{12,4,44,45},{13,6,43,42}},{{7,2,45,47},{8,6,46,42}},{{18,3,37,46},{15,5,39,43}}};
inline DungeonHeroPose dungeonHeroPose(unsigned cls,const CombatResult& r,unsigned p,unsigned ms){
 (void)ms;cls%=4;using H=HeroChoreography;auto h=heroChoreography(cls,r);bool active=h!=H::None&&p<1000;
 bool cast=h==H::StaffBolt||h==H::MagicDarts||h==H::Fireball||h==H::LegacyLightning||h==H::ScorchingRays;
 bool thrust=h==H::SpearThrust||h==H::PrecisionThrust;
 bool cut=h==H::SwordCut||h==H::SwordPower||h==H::DivineSmite||h==H::AxeCut||h==H::RageStrike;
 // Reuse the enlarged idle drawing for benefits and hand gestures, without a fake strike.
 unsigned f=active&&(cast||thrust||cut)&&p>=200&&p<750?1:0;
 auto crop=dungeonWeaponCrops[cls][f];int w=std::min(88,int(crop.w)*2),height=crop.h*2;
 int x=240-w,y=172-height;
 if(active){
  int phase=p<200?0:p<491?1:p<650?2:p<850?3:4;
  if(thrust){static const int8_t dy[]={4,2,-8,-2,0};y+=dy[phase];height-=dy[phase];}
  else if(cut){static const int8_t dy[]={4,1,-7,-2,0},dx[]={3,1,-4,-1,0};y+=dy[phase];x+=dx[phase];height-=dy[phase];}
  else if(cast){static const int8_t dy[]={3,-3,-5,-1,0};y+=dy[phase];height-=dy[phase];}
  else if(h==H::LayHands||h==H::SecondWind){int d=phase<2?8:phase==2?3:0;y+=d;height-=d;}
  else if(h==H::ArcaneGuard||h==H::ShieldGuard||h==H::SpearGuard||h==H::AxeGuard||h==H::RageBuff||h==H::ActionSurge||h==H::TurnUndead||h==H::FlameCone){int d=phase<4?-3:0;y+=d;height-=d;}
 }
 // Uniform final fit preserves the authored weapon, while the 29-enemy rooted
 // silhouette gate reserves the center of the viewport. Still ~1.6-1.7x source.
 int motionX=x-(240-w);w=w*85/100;height=height*85/100;
 x=240-w+motionX*85/100+1;y=172-height;
 // Staff/orb coordinates are measured in each original crop, not the old (176,138) anchor.
 int ox=cls==0?(f?38:7):crop.w/2,oy=cls==0?(f?6:6):crop.h/2;
 return {uint8_t(f),int16_t(x),int16_t(y),int16_t(w),int16_t(height),int16_t(x+ox*w/crop.w),int16_t(y+oy*height/crop.h),int16_t(std::min(230,x+w*3/4)),int16_t(y+height*4/5)};
}
