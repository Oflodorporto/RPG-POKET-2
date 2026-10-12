#pragma once
#include "CombatFx.h"
// Capture the already-resolved action. onHero is a target, never proof of injury.
inline CombatResult heroCombatResult(const rpg::Game& g,rpg::Action a){
 CombatResult r;
 if(a==rpg::Action::LayHands||a==rpg::Action::SecondWind){r.kind=CombatFeedback::Heal;r.amount=g.damage;}
 else if(a==rpg::Action::Defensive||a==rpg::Action::ShieldSpell)r.kind=CombatFeedback::Guard;
 else if(a==rpg::Action::RagePower||a==rpg::Action::ActionSurge||a==rpg::Action::SacredWeapon||a==rpg::Action::TurnUndead)r.kind=CombatFeedback::Buff;
 else {r.kind=g.dodge?CombatFeedback::Miss:g.damage?CombatFeedback::Damage:CombatFeedback::NoDamage;r.amount=g.dodge?0:g.damage;r.critical=!g.dodge&&g.crit;}
 return r;
}
inline CombatResult enemyCombatResult(const rpg::Game& g,rpg::Intent intent,unsigned previousHp,bool skipped=false){
 CombatResult r;r.enemyAction=true;if(skipped){r.kind=CombatFeedback::Skip;return r;}
 if(intent==rpg::Intent::Mend){r.kind=CombatFeedback::Heal;r.amount=g.enemyHp>previousHp?g.enemyHp-previousHp:0;}
 else if(intent==rpg::Intent::Prepare)r.kind=CombatFeedback::Buff;
 else {r.kind=g.dodge?CombatFeedback::Miss:g.damage?CombatFeedback::Damage:CombatFeedback::NoDamage;r.amount=g.dodge?0:g.damage;r.critical=!g.dodge&&g.crit;}
 return r;
}
// A shared local clip for every primitive; no framebuffer, heap allocation or SD IO.
template<class C>struct CombatCanvas {
 C& c;int top=0,bottom=172,offsetX=0,offsetY=0;
 void fillRect(int x,int y,int w,int h,uint16_t color){
  x+=offsetX;y+=offsetY;int left=std::max(0,x),right=std::min(240,x+w),first=std::max(top,y),last=std::min(bottom,y+h);
  if(right>left&&last>first)c.fillRect(left,first,right-left,last-first,color);
 }
 void drawRect(int x,int y,int w,int h,uint16_t color){if(w<=0||h<=0)return;fillRect(x,y,w,1,color);fillRect(x,y+h-1,w,1,color);fillRect(x,y,1,h,color);fillRect(x+w-1,y,1,h,color);}
 void draw16bitRGBBitmap(int x,int y,uint16_t* pixels,int w,int h){
  int left=std::max(0,x+offsetX),right=std::min(240,x+offsetX+w),first=std::max(top,y+offsetY),last=std::min(bottom,y+offsetY+h);
  if(right<=left||last<=first)return;
  for(int row=first;row<last;++row)c.draw16bitRGBBitmap(left,row,pixels+(row-y-offsetY)*w+left-x-offsetX,right-left,1);
 }
};
inline unsigned combatProgress(unsigned elapsed,unsigned frame){return elapsed<=1000?elapsed:std::min(1000u,frame*125u);}
struct MagePose {uint8_t heroFrame;int8_t heroX,enemyX,enemyY;};
// The approved mage pilot, sampled at the existing 60 ms effect cadence.
inline MagePose magePose(unsigned ms){
 if(ms<180)return {1,int8_t(ms<120?-2:-3),0,0};
 if(ms<540)return {uint8_t(ms<300?2:3),int8_t(ms<300?2:3),0,0};
 if(ms<660)return {3,2,int8_t(ms<600?5:7),int8_t(ms<600?-1:0)};
 if(ms<900)return {4,1,int8_t(ms<720?6:ms<780?3:ms<840?1:0),0};
 if(ms<1100)return {5,0,0,0};
 return {0,0,0,0};
}
inline int combatRecoil(const CombatResult& r,unsigned p){
 if(!r.injury()||p<491||p>=818)return 0;
 if(p<600)return 6;
 return int((818-p)*6/218);
}
