#pragma once
#include "Rules.h"
namespace rpg {
// Pilot: one offer per valid local day, available at 09h; maximum date never decreases.
// Stage 0=none,1=pending,2=mission,3=completed,4=refused,5=lost,6=fled.
inline uint8_t eventTierFor(uint8_t level){return level<5?0:level<10?1:level<18?2:3;}
inline unsigned eventGold(uint8_t tier){static const unsigned n[]={25,70,150,300};return n[tier<4?tier:0];}
inline unsigned eventXp(uint8_t tier){static const unsigned n[]={20,60,140,280};return n[tier<4?tier:0];}
inline const char* eventPlace(uint8_t tier){static const char* n[]={"Pomar de Carvalho","Ruinas de Vespera","Costa de Mares","Telhados de Aurora"};return n[tier<4?tier:0];}
inline bool eventSafe(const Game& g){return g.tutorial&&!g.campaignStage&&g.phase==Phase::Home&&g.p.hp&&!g.tripStage&&!g.campStage&&!g.dungeonFlags&&g.clubStage!=1&&g.clubStage!=2;}
inline bool eventReturnPage(uint8_t p){return p==2||p==9||p==27||p==36||p==49;}
inline bool eventValid(const Game& g){
 if(g.eventStage>6||g.eventTier>3||g.eventOriginCity>3)return false;
 if(!g.eventStage)return !g.eventDay&&!g.eventTier&&!g.eventOriginCity&&!g.eventOriginPage&&g.enemyId<10;
 if(g.eventDay<20454||g.eventDay>47481)return false;
 if(g.eventStage==1)return !g.eventOriginCity&&!g.eventOriginPage&&g.enemyId<10;
 if(!eventReturnPage(g.eventOriginPage)&&g.eventStage!=4)return false;
 if(g.eventStage==2)return !g.tripStage&&!g.campStage&&!g.dungeonFlags&&g.clubStage!=1&&g.clubStage!=2&&g.phase!=Phase::Home&&g.enemyId==10+g.eventTier&&g.city==g.eventTier;
 return g.enemyId<10;
}
inline bool offerEvent(Game& g,uint32_t day,unsigned hour){if(!eventSafe(g)||g.eventStage==2||hour<9||day<20454||day>47481||day<=g.eventDay)return false;g.eventDay=day;g.eventStage=1;g.eventTier=eventTierFor(g.p.level);g.eventOriginCity=g.eventOriginPage=0;return true;}
inline const char* acceptEvent(Game& g,uint8_t page){if(g.eventStage!=1)return "Carta indisponivel";if(!eventSafe(g))return "Termine a acao atual primeiro";g.eventOriginCity=g.city;g.eventOriginPage=eventReturnPage(page)?page:2;g.city=g.eventTier;g.eventStage=2;if(!begin(g,10+g.eventTier))return "Nao foi possivel iniciar";return nullptr;}
inline bool refuseEvent(Game& g){if(g.eventStage!=1)return false;g.eventStage=4;return true;}
inline bool finishEvent(Game& g){if(g.eventStage!=2||(g.phase!=Phase::Won&&g.phase!=Phase::Lost&&g.phase!=Phase::Fled))return false;
 bool won=g.phase==Phase::Won;uint8_t outcome=won?3:g.phase==Phase::Lost?5:6;
 if(won){g.p.gold=std::min<uint32_t>(999999u,g.p.gold+eventGold(g.eventTier));unsigned xp=eventXp(g.eventTier);g.p.xp=g.p.xp>UINT32_MAX-xp?UINT32_MAX:g.p.xp+xp;levelUp(g);}
 home(g);g.city=g.eventOriginCity;g.eventStage=outcome;g.enemyId=2;g.enemyHp=enemySpec(2).hp;g.guard=0;g.gainGold=g.gainXp=0;g.dropLife=g.dropMana=false;clearFeedback(g);return true;
}
}
