#pragma once
#include "Rules.h"
namespace rpg {
// Two opportunities per local day: 09h and 18h. Pending letters never expire.
// Slot 0 is the first offer, slot 1 the second; the recorded day never decreases.
// Stage 0=none,1=pending,2=mission,3=completed,4=refused,5=lost,6=fled.
inline uint8_t eventTierFor(uint8_t level){return level<5?0:level<10?1:level<18?2:3;}
inline unsigned eventGold(uint8_t tier){static const unsigned n[]={25,70,150,300};return n[tier<4?tier:0];}
inline unsigned eventXp(uint8_t tier){static const unsigned n[]={20,60,140,280};return n[tier<4?tier:0];}
inline const char* eventPlace(uint8_t tier){static const char* n[]={"Pomar de Carvalho","Ruinas de Vespera","Costa de Mares","Telhados de Aurora"};return n[tier<4?tier:0];}
inline bool eventSafe(const Game& g){return !g.discovery&&g.tutorial&&!g.campaignStage&&g.phase==Phase::Home&&g.p.hp&&!g.tripStage&&!g.campStage&&!g.dungeonFlags&&g.clubStage!=1&&g.clubStage!=2;}
inline bool eventReturnPage(uint8_t p){return p==2||p==9||p==27||p==36||p==49;}
// Kind:0 legacy/hippogriff,1 recover two loads,2 escort to the adjacent lower-tier city.
inline const char* eventCityName(unsigned city){const char* names[]={"Carvalho","Ruinas","Mares","Aurora"};return names[city<4?city:0];}
inline bool eventReady(const Game& g){return g.eventStage==2&&g.phase==Phase::Home&&(g.eventKind==1?g.eventProgress==2:g.eventKind==2&&g.eventProgress==1);}
inline uint8_t eventDestination(const Game& g){return g.eventTier?g.eventTier-1:0;}
inline unsigned eventGold(const Game& g){return g.eventLevel?30+8*g.eventLevel+10*g.eventKind:eventGold(g.eventTier);}
inline unsigned eventXp(const Game& g){if(!g.eventLevel)return eventXp(g.eventTier);if(g.dndProgression&&g.eventLevel==20)return 0;unsigned next=g.dndProgression?dndXpNeeded(g.eventLevel):xpNeeded(g.eventLevel);return std::max(20u,next*(g.eventKind==2?10u:8u)/100);}
inline const char* eventTitle(const Game& g){return g.eventKind==1?"O QUE A ESTRADA LEVOU":g.eventKind==2?"NINGUEM VIAJA SOZINHO":"ASAS SOBRE OS TELHADOS";}
inline bool eventValid(const Game& g){
 if(g.eventStage>6||g.eventTier>3||g.eventOriginCity>3||g.eventOfferSlot>1||g.eventKind>2||g.eventLevel>20||g.eventProgress>(g.eventKind==1?2:1))return false;
 if(g.eventLevel&&(g.eventLevel>g.p.level||g.eventTier!=eventTierFor(g.eventLevel)))return false;
 if((!g.eventKind&&g.eventProgress)||(g.eventKind&&!g.eventLevel)||(g.eventKind==2&&!g.eventTier))return false;
 if(!g.eventStage)return !g.eventKind&&!g.eventProgress&&!g.eventLevel&&!g.eventOfferSlot&&!g.eventDay&&!g.eventTier&&!g.eventOriginCity&&!g.eventOriginPage&&(g.enemyId<10||g.enemyId>=14);
 if(g.eventDay<20454||g.eventDay>47481)return false;
 if(g.eventStage==1)return !g.eventProgress&&!g.eventOriginCity&&!g.eventOriginPage&&(g.enemyId<10||g.enemyId>=14);
 if(!eventReturnPage(g.eventOriginPage)&&g.eventStage!=4)return false;
 if(g.eventStage==4&&g.eventProgress)return false;
 if(g.eventStage!=2)return g.enemyId<10||g.enemyId>=14;
 if(g.campStage||g.dungeonFlags||g.discovery||g.campaignStage||g.clubStage==1||g.clubStage==2)return false;
 if(!g.eventKind)return !g.tripStage&&g.phase!=Phase::Home&&g.enemyId==10+g.eventTier&&g.city==g.eventTier;
 if(g.enemyId>=10&&g.enemyId<14)return false;
 if(g.eventKind==1)return !g.tripStage&&g.city==g.eventTier&&(g.eventProgress<2||g.phase==Phase::Home);
 if(g.eventProgress)return !g.tripStage&&g.phase==Phase::Home&&g.city==eventDestination(g);
 if(g.city!=g.eventTier)return false;
 if(g.tripStage)return g.tripTo==eventDestination(g)&&(g.tripStage==2||g.phase==Phase::Home);
 return g.phase==Phase::Home||g.phase==Phase::Lost||g.phase==Phase::Fled;
}
inline bool offerEvent(Game& g,uint32_t day,unsigned hour){
 if(!eventSafe(g)||g.eventStage==1||g.eventStage==2||hour<9||hour>23||day<20454||day>47481||day<g.eventDay)return false;
 const bool sameDay=day==g.eventDay;
 if(sameDay&&(g.eventOfferSlot==1||hour<18))return false;
 // A late login can answer the first letter, then receive the evening letter.
 unsigned eligible=g.p.level<5?2:3;unsigned next=random(g);
 g.eventKind=sameDay?uint8_t((g.eventKind+1+next%(eligible-1))%eligible):uint8_t(next%eligible);
 g.eventProgress=0;g.eventLevel=std::min(20u,unsigned(g.p.level));g.eventOfferSlot=sameDay?1:0;g.eventDay=day;g.eventStage=1;g.eventTier=eventTierFor(g.p.level);g.eventOriginCity=g.eventOriginPage=0;return true;
}
inline const char* acceptEvent(Game& g,uint8_t page){if(g.eventStage!=1)return "Carta indisponivel";if(!eventSafe(g))return "Termine a acao atual primeiro";g.eventOriginCity=g.city;g.eventOriginPage=eventReturnPage(page)?page:2;g.city=g.eventTier;g.eventStage=2;g.eventProgress=0;
 if(!g.eventKind&&!begin(g,10+g.eventTier))return "Nao foi possivel iniciar";return nullptr;
}
inline bool refuseEvent(Game& g){if(g.eventStage!=1)return false;g.eventStage=4;return true;}
inline const char* searchEvent(Game& g,bool night=false){
 if(g.eventStage!=2||g.eventKind!=1||g.phase!=Phase::Home||g.city!=g.eventTier||g.eventProgress>=2)return "Busca indisponivel";
 unsigned roll=random(g);if(roll%100<50){if(!begin(g,regionalEnemy(g,random(g),night)))return "Encontro indisponivel";}
 else ++g.eventProgress;return nullptr;
}
inline bool resumeEventBattle(Game& g){if(g.eventStage!=2||!g.eventKind||g.phase!=Phase::Won)return false;return home(g);}
inline bool abandonEvent(Game& g){if(g.eventStage!=2||!g.eventKind||g.phase!=Phase::Home||g.tripStage||eventReady(g))return false;g.phase=Phase::Fled;return true;}
inline bool finishEvent(Game& g){if(g.eventStage!=2)return false;
 bool won=eventReady(g)||(!g.eventKind&&g.phase==Phase::Won);
 if(!won&&g.phase!=Phase::Lost&&g.phase!=Phase::Fled)return false;
 uint8_t outcome=won?3:g.phase==Phase::Lost?5:6;
 if(won){g.p.gold=std::min<uint32_t>(999999u,g.p.gold+eventGold(g));unsigned xp=eventXp(g);g.p.xp=g.p.xp>UINT32_MAX-xp?UINT32_MAX:g.p.xp+xp;levelUp(g);}
 if(g.phase!=Phase::Home)home(g);clearTrip(g);g.city=g.eventOriginCity;g.eventStage=outcome;g.enemyId=2;g.enemyHp=enemySpec(2).hp;g.guard=0;g.gainGold=g.gainXp=0;g.dropLife=g.dropMana=false;clearFeedback(g);return true;
}
}
