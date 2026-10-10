#pragma once
#include "Rules.h"
namespace rpg {
inline uint8_t campEnemy(const Game& g){return g.city==0?4:g.city==1?5:g.city==2?6:7;}
inline unsigned campDifficulty(const Game& g){return 11+g.city*2;}
inline unsigned campTotal(const Game& g){return g.campRoll+survival(g)+luck(g)+(g.campKit?2:0);}
inline bool campSafe(const Game& g){return g.campRoll==20||(g.campRoll!=1&&campTotal(g)>=campDifficulty(g));}
inline void clearCamp(Game& g){g.campStage=g.campRoll=0;g.campRation=g.campKit=false;}
inline bool campValid(const Game& g){
  if(g.campStage>3)return false;
  if(!g.campStage)return !g.campRoll&&!g.campRation&&!g.campKit;
  if(g.tripStage||g.dungeonFlags||g.clubStage==1||g.clubStage==2||g.city>3||g.campRoll<1||g.campRoll>20||(g.campKit&&!g.sleepKit))return false;
  if(g.campStage==2)return g.phase!=Phase::Home&&g.enemyId==campEnemy(g);
  return g.phase==Phase::Home;
}
inline const char* buySleepKit(Game& g){if(g.phase!=Phase::Home||g.campStage||g.tripStage||g.dungeonFlags)return "Termine a acao atual";if(g.sleepKit)return "Voce ja tem um kit";if(g.p.gold<80)return "Precisa de 80 ouro";g.p.gold-=80;g.sleepKit=true;return nullptr;}
inline const char* startCamp(Game& g,bool ration,bool kit){
  if(g.eventStage==2||g.phase!=Phase::Home||g.campStage||g.tripStage||g.dungeonFlags||g.clubStage==1||g.clubStage==2)return "Termine a acao atual";
  if(g.p.hp==g.p.maxhp&&g.p.mp==g.p.maxmp&&!g.laySpent&&!g.channelSpent&&!g.windSpent&&!g.surgeSpent&&!g.rageSpent)return "HP e MP ja estao cheios";
  if(ration&&!g.rations)return "Sem racoes";if(kit&&!g.sleepKit)return "Sem kit de dormir";
  g.campStage=1;g.campRation=ration;g.campKit=kit;if(ration)--g.rations;g.campRoll=1+random(g)%20;return nullptr;
}
inline bool acceptCamp(Game& g){if(g.campStage!=1||g.phase!=Phase::Home)return false;if(campSafe(g)){g.campStage=3;return true;}if(!begin(g,campEnemy(g)))return false;g.campStage=2;return true;}
inline bool resolveCamp(Game& g){if(g.campStage!=2||(g.phase!=Phase::Won&&g.phase!=Phase::Lost&&g.phase!=Phase::Fled))return false;bool won=g.phase==Phase::Won;home(g);if(won)g.campStage=3;else clearCamp(g);return true;}
inline bool finishCamp(Game& g){if(g.campStage!=3||g.phase!=Phase::Home)return false;
  g.p.hp=g.campRation?g.p.maxhp:std::min<uint32_t>(g.p.maxhp,uint32_t(g.p.hp)+std::max(1,g.p.maxhp/2));
  g.p.mp=g.campRation?g.p.maxmp:std::min<uint32_t>(g.p.maxmp,uint32_t(g.p.mp)+std::max(1,g.p.maxmp/2));refreshPowers(g);clearCamp(g);return true;
}
inline unsigned sellPrice(const Game& g,uint8_t id){return cityGearPrice(g,id)/2;}
inline const char* sellGear(Game& g,uint8_t id){if(g.phase!=Phase::Home||g.campStage||g.tripStage||g.dungeonFlags)return "Venda na cidade";
  if(!gearOwns(g.owned,id))return "Item nao esta na bolsa";if(g.equipped[gearSlot(id)]==id)return "Retire o item antes de vender";
  auto value=sellPrice(g,id);if(g.p.gold>999999u-value)return "Limite de ouro";g.p.gold+=value;g.owned&=~(1u<<(id-1));return nullptr;
}
}
