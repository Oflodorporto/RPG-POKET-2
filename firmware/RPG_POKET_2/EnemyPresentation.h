#pragma once
#include "EnemyArt.h"
#include "EnemyScale.h"
#include "CombatPresentation.h"
// These phases resample existing effect clocks. They never delay resolution, save or input.
inline enemyArt::Pose enemyPose(bool active,const CombatResult& r,unsigned progress,bool targetHero,bool waiting=false){
 if(!active)return waiting?enemyArt::Pose::Anticipation:enemyArt::Pose::Idle;
 if(r.enemyAction){
  if(r.kind==CombatFeedback::Skip||r.kind==CombatFeedback::Heal)return enemyArt::Pose::Idle;
  if(r.resolvedEnemyIntent==rpg::Intent::Prepare)return progress<650?enemyArt::Pose::Anticipation:enemyArt::Pose::Idle;
  if(r.attack())return progress<200?enemyArt::Pose::Anticipation:progress<650?enemyArt::Pose::Attack:enemyArt::Pose::Idle;
 }else if(!targetHero&&r.injury()&&progress>=491&&progress<(r.enemyDefeated?650u:818u))return enemyArt::Pose::Hurt;
 return enemyArt::Pose::Idle;
}
inline unsigned enemyOpacity(bool active,const CombatResult& r,unsigned progress,bool defeated){
 if(!active)return defeated?0:1000;
 if(r.enemyDefeated&&!r.enemyAction&&r.injury()&&progress>=650)return progress>=1000?0:(1000-progress)*1000/350;
 return 1000;
}
// Enemy benefits use a quiet recovery/preparation cue, never an injury flash.
// Shared authored attack families express existing Strike/Heavy/Drain only.
template<class C>inline void drawEnemyAction(C& c,unsigned id,const CombatResult& r,unsigned progress,int sourceX,int sourceY,int targetX,int targetY){
 if(id>=enemyArt::ENEMY_COUNT||!r.enemyAction||r.kind==CombatFeedback::Skip)return;
 if(r.kind==CombatFeedback::Heal||r.resolvedEnemyIntent==rpg::Intent::Prepare){
  if(progress<200||progress>=818)return;
  unsigned step=(progress-200)*8/618;int radius=8+int(step%4)*2;
  if(r.kind==CombatFeedback::Heal){c.fillRect(sourceX-2,sourceY-radius-8,4,12,0x6e0c);c.fillRect(sourceX-6,sourceY-radius-4,12,4,0x6e0c);}
  else c.drawRect(sourceX-radius,sourceY-radius,2*radius,2*radius,0xd5aa);
  return;
 }
 if(!r.attack()||!r.injury()||progress<200||progress>=818)return;
 unsigned frame=std::min(7u,(progress-200)*8/618);int travel=std::min(1000u,(progress-200)*1000/291);
 // Drain's return is a visual trail only: the rules determine HP/MP, with no implied heal.
 if(r.resolvedEnemyIntent==rpg::Intent::Drain&&progress>=650)travel=int((818-progress)*1000/168);
 int x=sourceX+(targetX-sourceX)*travel/1000,y=sourceY+(targetY-sourceY)*travel/1000;
 int size=r.resolvedEnemyIntent==rpg::Intent::Heavy?44:32;
 enemyArt::drawEffect(c,enemyArt::enemies[id].family,frame,x,y,size);
}
