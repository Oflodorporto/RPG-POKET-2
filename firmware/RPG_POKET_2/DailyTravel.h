#pragma once
#include "Rules.h"

// Pure state transitions. The controller must durably save every accepted
// transition before exposing it or beginning its animation. Timers are UI-only.
namespace rpg {
constexpr uint8_t DAILY_TRAVEL_POLICY=1,DAILY_ROUTE_VERSION=1;
inline bool dailyTrip(const Game& g){return g.tripStage==4;}
inline unsigned dailyTripDays(unsigned origin,unsigned destination){return origin<4&&destination<4?2u*unsigned(abs(int(origin)-int(destination))):0;}
inline uint8_t dailyRegion(const Game& g,unsigned completedDays){
  const int step=g.tripTo>g.daily.origin?1:-1;
  return uint8_t(int(g.daily.origin)+step*int(completedDays/2));
}
inline float dailyTripPosition(const Game& g){
  if(!dailyTrip(g)||!g.daily.totalDays)return 0;
  const bool ended=g.daily.phase==DailyPhase::NightRoll||g.daily.phase==DailyPhase::NightBattle||g.daily.phase==DailyPhase::Rest||g.daily.phase==DailyPhase::Tent;
  return float(g.daily.day-(ended?0:1))/g.daily.totalDays;
}
inline bool dailyNightSafe(const Game& g){return g.daily.roll==20||(g.daily.roll!=1&&g.daily.total>=g.daily.difficulty);}
inline bool dailyBattle(const Game& g){return dailyTrip(g)&&(g.daily.phase==DailyPhase::NightBattle||g.daily.phase==DailyPhase::RoadBattle);}
inline void clearDailyNight(DailyTrip& d){d.roll=d.difficulty=d.survivalBonus=d.luckBonus=d.kit=d.total=d.enemy=d.result=0;}
inline const char* dailyTripError(const Game& g,unsigned destination){
  if(auto error=tripError(g,destination))return error;
  if(g.discovery||g.campaignStage||!dailyStateEmpty(g.daily)||!g.p.hp)return "Termine a acao atual";
  if(g.rations<dailyTripDays(g.city,destination))return "Uma racao para cada dia";
  return nullptr;
}
inline const char* prepareDailyTrip(Game& g,uint8_t destination,bool useKit=true){
  if(auto error=dailyTripError(g,destination))return error;
  const uint8_t supplies=(g.charts?1:0)|(g.charms?2:0)|((useKit&&g.sleepKit)?4:0);
  // Reuse the approved original road roll and its exact supply modifiers once.
  if(auto error=prepareTrip(g,destination))return error;
  g.tripStage=4;g.daily=DailyTrip{};auto& d=g.daily;
  d.phase=DailyPhase::RoadRoll;d.origin=g.city;d.totalDays=dailyTripDays(g.city,destination);
  d.day=d.rationsPaid=1;d.region=g.city;d.supplies=supplies;
  d.policy=DAILY_TRAVEL_POLICY;d.routeVersion=DAILY_ROUTE_VERSION;
  return nullptr;
}
inline bool acceptDailyRoad(Game& g){
  if(!dailyTrip(g)||g.daily.phase!=DailyPhase::RoadRoll||g.phase!=Phase::Home)return false;
  if(tripSafe(g)){g.daily.phase=DailyPhase::Leg;return true;}
  g.daily.phase=DailyPhase::RoadBattle;
  if(!begin(g,g.tripEnemy)){g.daily.phase=DailyPhase::RoadRoll;return false;}
  return true;
}
inline void dailyFullRecovery(Game& g,uint8_t result){
  g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;refreshPowers(g);g.fatigue=0;
  g.daily.result=result;g.daily.nightsDone=g.daily.day;g.daily.phase=DailyPhase::Rest;
}
inline bool finishDailyLeg(Game& g){
  if(!dailyTrip(g)||g.daily.phase!=DailyPhase::Leg||g.phase!=Phase::Home)return false;
  auto& d=g.daily;
  if(d.day==d.totalDays){
    if(escortTrip(g)){if(g.eventStage==2)g.eventProgress=1;else g.questProgress=1;}
    g.city=g.tripTo;clearTrip(g);return true;
  }
  d.region=dailyRegion(g,d.day);d.roll=1+random(g)%20;d.difficulty=11+2*d.region;
  d.survivalBonus=survival(g);d.luckBonus=luck(g);d.kit=(d.supplies&4)?1:0;
  d.total=d.roll+d.survivalBonus+d.luckBonus+2*d.kit;d.enemy=d.result=0;
  d.phase=DailyPhase::NightRoll;return true;
}
inline bool acceptDailyNight(Game& g){
  if(!dailyTrip(g)||g.daily.phase!=DailyPhase::NightRoll||g.phase!=Phase::Home)return false;
  if(dailyNightSafe(g)){dailyFullRecovery(g,1);return true;}
  auto& d=g.daily;const auto oldRandom=g.randomState;
  d.enemy=regionalEnemyAt(g,d.region,random(g),true);d.phase=DailyPhase::NightBattle;
  if(!begin(g,d.enemy)){d.enemy=0;d.phase=DailyPhase::NightRoll;g.randomState=oldRandom;return false;}
  return true;
}
inline bool resolveDailyBattle(Game& g){
  if(!dailyBattle(g)||(g.phase!=Phase::Won&&g.phase!=Phase::Lost&&g.phase!=Phase::Fled))return false;
  const bool night=g.daily.phase==DailyPhase::NightBattle,won=g.phase==Phase::Won;
  const bool lost=g.phase==Phase::Lost,escort=escortTrip(g);
  g.phase=Phase::Home;g.enemyBeat=0;g.sacredTurns=g.turnedTurns=0;clearMartialCombat(g);
  if(lost){g.city=g.daily.origin;g.p.hp=1;clearTrip(g);return true;}
  if(!won&&escort){g.city=g.daily.origin;clearTrip(g);return true;}
  if(!night){g.daily.phase=DailyPhase::Leg;return true;}
  if(won)dailyFullRecovery(g,2);
  else {g.daily.result=3;g.daily.nightsDone=g.daily.day;g.daily.phase=DailyPhase::Tent;}
  return true;
}
inline bool finishDailyRest(Game& g){
  if(!dailyTrip(g)||g.daily.phase!=DailyPhase::Rest||g.phase!=Phase::Home)return false;
  g.daily.phase=DailyPhase::Tent;return true;
}
inline bool continueDailyTrip(Game& g){
  if(!dailyTrip(g)||g.daily.phase!=DailyPhase::Tent||g.phase!=Phase::Home||!g.rations||g.daily.day>=g.daily.totalDays)return false;
  --g.rations;++g.daily.day;++g.daily.rationsPaid;clearDailyNight(g.daily);
  g.daily.region=dailyRegion(g,g.daily.day-1);g.daily.phase=DailyPhase::Leg;return true;
}
inline bool dailyEnemyInRegion(uint8_t region,uint8_t enemy){
  if(region==0)return enemy==0||enemy==1||enemy==4;
  if(region==1)return enemy==2||enemy==5;
  if(region==2)return enemy==6||enemy==25||enemy==26;
  return region==3&&(enemy==7||enemy==27||enemy==28);
}
inline bool dailyTravelValid(const Game& g){
  const auto& d=g.daily;
  if(!dailyTrip(g))return dailyStateEmpty(d);
  if(uint8_t(d.phase)<1||uint8_t(d.phase)>7||d.origin>3||g.city!=d.origin||g.tripTo>3||g.tripTo==d.origin||
     d.totalDays!=dailyTripDays(d.origin,g.tripTo)||!d.day||d.day>d.totalDays||d.rationsPaid!=d.day||
     d.region>3||d.policy!=DAILY_TRAVEL_POLICY||d.routeVersion!=DAILY_ROUTE_VERSION||d.supplies>7||
     ((d.supplies&4)&&!g.sleepKit)||g.campStage||g.campRoll||g.campRation||g.campKit||g.discovery||
     g.dungeonFlags||g.campaignStage||g.clubStage==1||g.clubStage==2||g.rations>RATION_LIMIT||
     g.rations<d.totalDays-d.rationsPaid)return false;
  if(g.tripRoll<1||g.tripRoll>20||g.tripSurvival>13||g.tripLuck>11||g.tripTotal!=g.tripRoll+g.tripSurvival+g.tripLuck||
     g.tripDifficulty!=routeDifficulty(d.origin,g.tripTo))return false;
  const unsigned danger=std::max(cityLevel(d.origin),cityLevel(g.tripTo));
  if(g.tripEnemy!=(danger>=18?7:danger>=10?6:danger>=5?5:4))return false;
  const bool road=d.phase==DailyPhase::RoadRoll||d.phase==DailyPhase::RoadBattle;
  const bool leg=d.phase==DailyPhase::Leg;
  const bool done=d.phase==DailyPhase::Rest||d.phase==DailyPhase::Tent;
  const bool battle=d.phase==DailyPhase::RoadBattle||d.phase==DailyPhase::NightBattle;
  if(g.phase==Phase::Home?battle:!battle)return false;
  if(d.nightsDone!=d.day-(done?0:1))return false;
  if(road&&(d.day!=1||d.rationsPaid!=1||d.nightsDone))return false;
  if(road||leg){
    if(d.region!=dailyRegion(g,d.day-1)||d.roll||d.difficulty||d.survivalBonus||d.luckBonus||d.kit||d.total||d.enemy||d.result)return false;
    return d.phase!=DailyPhase::RoadBattle||(!tripSafe(g)&&g.enemyId==g.tripEnemy);
  }
  if(d.day>=d.totalDays||d.region!=dailyRegion(g,d.day)||d.roll<1||d.roll>20||d.difficulty!=11+2*d.region||
     d.survivalBonus>10||d.luckBonus>8||d.kit!=((d.supplies&4)?1:0)||
     d.total!=d.roll+d.survivalBonus+d.luckBonus+2*d.kit)return false;
  if(d.phase==DailyPhase::NightRoll)return !d.enemy&&!d.result;
  if(d.phase==DailyPhase::NightBattle)return !d.result&&!dailyNightSafe(g)&&dailyEnemyInRegion(d.region,d.enemy)&&g.enemyId==d.enemy;
  if(d.result<1||d.result>3||(d.phase==DailyPhase::Rest&&d.result==3))return false;
  if(d.result==1){if(!dailyNightSafe(g)||d.enemy)return false;}
  else if(dailyNightSafe(g)||!dailyEnemyInRegion(d.region,d.enemy)||g.enemyId!=d.enemy)return false;
  if(d.result==2&&g.enemyHp)return false;
  if(d.result==3)return g.enemyHp&&!escortTrip(g);
  return g.p.hp==g.p.maxhp&&g.p.mp==g.p.maxmp&&!g.fatigue&&!g.laySpent&&!g.channelSpent&&!g.windSpent&&!g.surgeSpent&&!g.rageSpent;
}
}
