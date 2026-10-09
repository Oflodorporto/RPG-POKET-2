#pragma once
#include "Rules.h"
namespace rpg {
// Port and royal archive missions are independent of repeatable guild contracts and loot.
inline bool campaignMemory(const Game& g){return g.dungeonClears||((g.dungeonFlags&1)&&(g.dungeonEnemies&64))||(g.enemyId==8&&g.phase==Phase::Won);}
inline unsigned campaignMission(const Game& g){return !(g.campaignFlags&1)?1:!(g.campaignFlags&2)?2:!(g.campaignFlags&4)?3:0;}
inline unsigned campaignGold(unsigned mission){return mission==1?120:mission==2?180:mission==3?350:0;}
inline unsigned campaignXp(const Game& g,unsigned mission){return (mission==1?100:mission==2?150:mission==3?350:0)*(g.dndProgression?10:1);}
inline const char* campaignTitle(unsigned mission){return mission==1?"A CARGA DE CINZAS":mission==2?"O FAROL APAGADO":mission==3?"ARQUIVOS DA COROA":"A VIGILIA PERPETUA";}
inline unsigned campaignCity(unsigned mission){return mission==3?3:2;}
inline unsigned campaignLevel(unsigned mission){return mission==3?18:10;}
inline unsigned campaignEnemy(unsigned mission){return mission==3?7:6;}
inline unsigned campaignScenes(unsigned mission){return mission==3?6:4;}
inline bool campaignContact(const Game& g){unsigned m=campaignMission(g);return m&&g.city==campaignCity(m)&&campaignMemory(g);}
inline bool campaignValid(const Game& g){
 if((g.campaignFlags!=0&&g.campaignFlags!=1&&g.campaignFlags!=3&&g.campaignFlags!=7)||g.campaignStage>3)return false;
 if(g.campaignFlags&&!campaignMemory(g))return false;
 if(!g.campaignStage)return true;
 return g.city==campaignCity(g.campaignStage)&&g.tutorial&&g.p.level>=campaignLevel(g.campaignStage)&&campaignMemory(g)&&g.campaignStage==campaignMission(g)&&g.enemyId==campaignEnemy(g.campaignStage)&&g.phase!=Phase::Home&&!g.tripStage&&!g.campStage&&!g.dungeonFlags&&g.eventStage!=2&&g.clubStage!=1&&g.clubStage!=2;
}
inline const char* campaignError(const Game& g,unsigned mission){
 if(!mission||mission!=campaignMission(g))return "Missao ja resolvida";
 if(g.phase!=Phase::Home||g.campaignStage||g.tripStage||g.campStage||g.dungeonFlags||g.eventStage==2||g.clubStage==1||g.clubStage==2)return "Termine a acao atual";
 if(!g.tutorial||!campaignMemory(g))return "Encontre o Livro das Vigilias";
 if(g.city!=campaignCity(mission))return mission==3?"Encontre Liora em Aurora":"Encontre Sabela em Mares";
 if(g.p.level<campaignLevel(mission))return mission==3?"Prepare-se: requer nivel 18":"Prepare-se: requer nivel 10";
 return nullptr;
}
inline const char* startCampaign(Game& g,unsigned mission){
 auto error=campaignError(g,mission);if(error)return error;
 if(!begin(g,campaignEnemy(mission)))return "Encontro indisponivel";
 g.campaignStage=mission;return nullptr;
}
inline bool resolveCampaign(Game& g){
 if(!g.campaignStage||!campaignValid(g)||(g.phase!=Phase::Won&&g.phase!=Phase::Lost&&g.phase!=Phase::Fled))return false;
 unsigned mission=g.campaignStage;bool won=g.phase==Phase::Won;
 if(won){g.campaignFlags|=1u<<(mission-1);g.p.gold=std::min<uint32_t>(999999,g.p.gold+campaignGold(mission));unsigned xp=campaignXp(g,mission);g.p.xp=g.p.xp>UINT32_MAX-xp?UINT32_MAX:g.p.xp+xp;levelUp(g);}
 g.campaignStage=0;return home(g);
}
}
