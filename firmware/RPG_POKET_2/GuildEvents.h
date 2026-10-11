#pragma once
#include "Dungeon.h"
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
// Kinds0..7: combat, recovery, escort, expedition, repair, rescue, investigation, defence.
inline const char* eventCityName(unsigned city){const char* names[]={"Carvalho","Ruinas","Mares","Aurora"};return names[city<4?city:0];}
inline unsigned eventGoal(unsigned kind){const unsigned n[]={0,2,1,1,3,4,3,3};return n[kind<8?kind:0];}
inline bool eventReady(const Game& g){return g.eventStage==2&&g.eventKind&&g.phase==Phase::Home&&g.eventProgress==eventGoal(g.eventKind);}
inline uint8_t eventDestination(const Game& g){return g.eventTier?g.eventTier-1:0;}
inline unsigned eventGold(const Game& g){return g.eventLevel?30+8*g.eventLevel+10*g.eventKind:eventGold(g.eventTier);}
inline unsigned eventXp(const Game& g){if(!g.eventLevel)return eventXp(g.eventTier);if(g.dndProgression&&g.eventLevel==20)return 0;unsigned next=g.dndProgression?dndXpNeeded(g.eventLevel):xpNeeded(g.eventLevel);return std::max(20u,next*((g.eventKind==2||g.eventKind==3||g.eventKind==5||g.eventKind==7)?10u:8u)/100);}
inline const char* eventTitle(const Game& g){const char* names[]={"ASAS SOBRE OS TELHADOS","O QUE A ESTRADA LEVOU","NINGUEM VIAJA SOZINHO","O SINO SOB A PEDRA","UMA LUZ PARA QUEM VOLTA","AS RODAS NA NEBLINA","NOMES GRAVADOS EM CINZA","UMA CHAMA CONTRA A NOITE"};return names[g.eventKind<8?g.eventKind:0];}
inline bool eventNight(const Game& g){return g.eventFlags&16;}
inline bool eventValid(const Game& g){
 if(g.eventStage>6||g.eventTier>3||g.eventOriginCity>3||g.eventOfferSlot>1||g.eventKind>7||g.eventLevel>20||g.eventRoll>20||g.eventFlags>31||g.eventProgress>eventGoal(g.eventKind))return false;
 if(g.eventLevel&&(g.eventLevel>g.p.level||g.eventTier!=eventTierFor(g.eventLevel)))return false;
 if((!g.eventKind&&g.eventProgress)||(g.eventKind&&!g.eventLevel)||(g.eventKind==2&&!g.eventTier))return false;
 unsigned flags=g.eventFlags&15;
 if(((g.eventKind==0||g.eventKind==2||g.eventKind==3)&&(flags||g.eventRoll))||(g.eventKind==1&&(flags>3||g.eventRoll))||(g.eventKind==4&&flags>2)||(g.eventKind==5&&(flags&~7u))||(g.eventKind==7&&flags>2))return false;
 if(!g.eventStage)return !g.eventKind&&!g.eventProgress&&!g.eventLevel&&!g.eventOfferSlot&&!g.eventDay&&!g.eventTier&&!g.eventOriginCity&&!g.eventOriginPage&&!g.eventFlags&&!g.eventRoll&&(g.enemyId<10||g.enemyId>=14);
 if(g.eventDay<20454||g.eventDay>47481)return false;
 if(g.eventStage==1)return !g.eventProgress&&!flags&&!g.eventRoll&&!g.eventOriginCity&&!g.eventOriginPage&&(g.enemyId<10||g.enemyId>=14);
 if(!eventReturnPage(g.eventOriginPage)&&g.eventStage!=4)return false;
 if(g.eventStage==4&&(g.eventProgress||flags||g.eventRoll))return false;
 if(g.eventStage!=2)return g.enemyId<10||g.enemyId>=14;
 if(g.campStage||g.discovery||g.campaignStage||g.clubStage==1||g.clubStage==2||(g.dungeonFlags&&g.eventKind!=3))return false;
 if(!g.eventKind)return !g.tripStage&&g.phase!=Phase::Home&&g.enemyId==10+g.eventTier&&g.city==g.eventTier;
 if(g.enemyId>=10&&g.enemyId<14)return false;
 if(g.eventKind==2){if(g.eventProgress)return !g.tripStage&&g.phase==Phase::Home&&g.city==eventDestination(g);if(g.city!=g.eventTier)return false;if(g.tripStage)return g.tripTo==eventDestination(g)&&(g.tripStage==2||g.phase==Phase::Home);return g.phase==Phase::Home||g.phase==Phase::Lost||g.phase==Phase::Fled;}
 if(g.tripStage||g.city!=g.eventTier)return false;
 if(g.eventKind==6&&((g.eventProgress&&((flags&7)!=7))||(!g.eventProgress&&(flags&8))))return false;
 if(g.eventKind==7&&g.eventProgress&&!flags)return false;
 if(eventReady(g))return true;
 if(g.eventKind==3)return g.dungeonFlags||g.phase==Phase::Home||g.phase==Phase::Fled;
 if(g.eventProgress==eventGoal(g.eventKind))return g.phase==Phase::Home;
 return true;
}
inline bool offerEvent(Game& g,uint32_t day,unsigned hour){
 if(!eventSafe(g)||g.eventStage==1||g.eventStage==2||hour<9||hour>23||day<20454||day>47481||day<g.eventDay)return false;
 const bool sameDay=day==g.eventDay;
 if(sameDay&&(g.eventOfferSlot==1||hour<18))return false;
 // A late login can answer the first letter, then receive the evening letter.
 uint8_t eligible[8];unsigned count=0;for(unsigned kind=0;kind<8;++kind)if((kind!=2||g.p.level>=5)&&(!sameDay||kind!=g.eventKind))eligible[count++]=kind;
 g.eventKind=eligible[random(g)%count];g.eventFlags=hour>=20?16:0;g.eventRoll=0;
 g.eventProgress=0;g.eventLevel=std::min(20u,unsigned(g.p.level));g.eventOfferSlot=sameDay?1:0;g.eventDay=day;g.eventStage=1;g.eventTier=eventTierFor(g.p.level);g.eventOriginCity=g.eventOriginPage=0;return true;
}
inline const char* acceptEvent(Game& g,uint8_t page){if(g.eventStage!=1)return "Carta indisponivel";if(!eventSafe(g))return "Termine a acao atual primeiro";g.eventOriginCity=g.city;g.eventOriginPage=eventReturnPage(page)?page:2;g.city=g.eventTier;g.eventStage=2;g.eventProgress=0;
 if(!g.eventKind&&!begin(g,10+g.eventTier))return "Nao foi possivel iniciar";return nullptr;
}
inline bool refuseEvent(Game& g){if(g.eventStage!=1)return false;g.eventStage=4;return true;}
inline const char* searchEvent(Game& g,bool night=false){
 if(g.eventStage!=2||g.eventKind!=1||g.phase!=Phase::Home||g.city!=g.eventTier||g.eventProgress>=2)return "Busca indisponivel";
 unsigned roll=random(g);if(roll%100<50&&(g.eventFlags&15)<3){if(!begin(g,regionalEnemy(g,random(g),night)))return "Encontro indisponivel";++g.eventFlags;}
 else {++g.eventProgress;g.eventFlags&=16;}return nullptr;
}
inline unsigned eventTest(Game& g,unsigned bonus,unsigned dc){g.eventRoll=1+random(g)%20;return g.eventRoll==20||(g.eventRoll!=1&&g.eventRoll+bonus>=dc);}
inline unsigned eventIntelligence(const Game& g){return g.dndProgression?unsigned(std::max(0,abilityMod(g.attributes[3])))+proficiency(g.p.level):luck(g);}
inline unsigned eventFoe(const Game& g,unsigned variant=0){return regionalEnemy(g,variant,eventNight(g));}
inline bool resumeEventBattle(Game& g){
 if(g.eventStage!=2||!g.eventKind||g.phase!=Phase::Won)return false;
 if(g.eventKind==3)return dungeonResolve(g);
 if(g.eventKind==4&&g.eventProgress==2)g.eventProgress=3;
 else if(g.eventKind==5){if(g.eventProgress==0){g.eventProgress=1;g.eventFlags&=~1u;}else if(g.eventProgress==1)g.eventProgress=2;else if(g.eventProgress==3){g.eventProgress=4;g.eventFlags&=~4u;}}
 else if(g.eventKind==6&&g.eventProgress==2)g.eventProgress=3;
 else if(g.eventKind==7&&g.eventProgress>=1&&g.eventProgress<3)++g.eventProgress;
 return home(g);
}
inline const char* eventAct(Game& g,unsigned choice=0){
 if(g.eventStage!=2||!g.eventKind||g.phase!=Phase::Home||eventReady(g)||choice>2)return "Acao indisponivel";
 if(g.eventKind==1)return choice?"Acao indisponivel":searchEvent(g,eventNight(g));
 if(g.eventKind==2)return choice?"Acao indisponivel":prepareTrip(g,eventDestination(g));
 if(g.eventKind==3){if(choice||inDungeon(g))return "Acao indisponivel";clearDungeon(g);g.dungeonFlags=1;g.dungeonXY=0x31;return nullptr;}
 if(g.eventKind==4){
  if(!g.eventProgress){if(choice)return "Acao indisponivel";g.eventProgress=1;if(!eventTest(g,survival(g),11+g.eventTier))begin(g,eventFoe(g));}
  else if(g.eventProgress==1){if(choice>1)return "Acao indisponivel";unsigned bonus=choice?survival(g):eventIntelligence(g);bool ok=eventTest(g,bonus,12+g.eventTier);if(ok||(g.eventFlags&15)>=2)g.eventProgress=2;else ++g.eventFlags;}
  else if(g.eventProgress==2){if(choice)return "Acao indisponivel";begin(g,eventFoe(g,1));}return nullptr;
 }
 if(g.eventKind==5){
  if(!g.eventProgress){if(choice)return "Acao indisponivel";if(eventTest(g,survival(g)+luck(g),13+g.eventTier))g.eventProgress=1;else {g.eventFlags|=1;begin(g,eventFoe(g));}}
  else if(g.eventProgress==1){if(choice)return "Acao indisponivel";begin(g,eventFoe(g,1));}
  else if(g.eventProgress==2){if(choice>1)return "Acao indisponivel";if(choice){if(!g.p.life)return "Voce nao tem pocao de vida";--g.p.life;g.eventFlags|=2;}g.eventProgress=3;}
  else if(g.eventProgress==3){if(choice)return "Acao indisponivel";if(eventTest(g,survival(g)+luck(g)+((g.eventFlags&2)?4:0),13+g.eventTier))g.eventProgress=4;else {g.eventFlags|=4;begin(g,eventFoe(g,2));}}return nullptr;
 }
 if(g.eventKind==6){
  if(!g.eventProgress){g.eventFlags|=1u<<choice;if((g.eventFlags&7)==7)g.eventProgress=1;}
  else if(g.eventProgress==1){if(choice==g.eventTier%3)g.eventProgress=2;else if(!(g.eventFlags&8)){g.eventFlags|=8;begin(g,eventFoe(g));}else return "Releia as pistas; tente outra ordem";}
  else if(g.eventProgress==2){if(choice)return "Acao indisponivel";begin(g,eventFoe(g,2));}return nullptr;
 }
 if(g.eventKind==7){
  if(!g.eventProgress){if(choice>1)return "Acao indisponivel";if(choice){if(!g.rations)return "Leve uma racao para as tochas";--g.rations;g.eventFlags=(g.eventFlags&16)|2;}else g.eventFlags=(g.eventFlags&16)|1;eventTest(g,choice?luck(g):survival(g),12+g.eventTier);g.eventProgress=1;}
  else {if(choice)return "Acao indisponivel";begin(g,eventFoe(g,g.eventProgress));g.guard=g.eventRoll==20||(g.eventRoll!=1&&g.eventRoll+((g.eventFlags&2)?luck(g):survival(g))>=12+g.eventTier)?75:50;}return nullptr;
 }
 return "Acao indisponivel";
}
inline bool abandonEvent(Game& g){if(g.eventStage!=2||!g.eventKind||g.phase!=Phase::Home||g.tripStage||eventReady(g))return false;g.phase=Phase::Fled;return true;}
inline bool finishEvent(Game& g){if(g.eventStage!=2)return false;
 bool won=eventReady(g)||(!g.eventKind&&g.phase==Phase::Won);
 if(!won&&g.phase!=Phase::Lost&&g.phase!=Phase::Fled)return false;
 uint8_t outcome=won?3:g.phase==Phase::Lost?5:6;
 if(won){if(g.eventKind==7&&g.rations<9)++g.rations;g.p.gold=std::min<uint32_t>(999999u,g.p.gold+eventGold(g));unsigned xp=eventXp(g);g.p.xp=g.p.xp>UINT32_MAX-xp?UINT32_MAX:g.p.xp+xp;levelUp(g);}
 if(g.phase!=Phase::Home)home(g);if(g.eventKind==3)clearDungeon(g);clearTrip(g);g.city=g.eventOriginCity;g.eventStage=outcome;g.enemyId=2;g.enemyHp=enemySpec(2).hp;g.guard=0;g.gainGold=g.gainXp=0;g.dropLife=g.dropMana=false;clearFeedback(g);return true;
}
}
