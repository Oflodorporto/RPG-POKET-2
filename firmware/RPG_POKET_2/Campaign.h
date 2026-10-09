#pragma once
#include "Rules.h"
namespace rpg {
// Port and royal archive missions are independent of repeatable guild contracts and loot.
inline bool campaignMemory(const Game& g){return g.dungeonClears||((g.dungeonFlags&1)&&(g.dungeonEnemies&64))||(g.enemyId==8&&g.phase==Phase::Won);}
inline bool anwenReady(const Game& g){return g.campaignFlags==127;}
inline bool contribution(const Game& g,unsigned city){return city<4&&(g.campaignFlags&(8u<<city));}
inline unsigned campaignMission(const Game& g){return !(g.campaignFlags&1)?1:!(g.campaignFlags&2)?2:!(g.campaignFlags&4)?3:!contribution(g,g.city)?4+g.city:0;}
inline unsigned campaignPerson(unsigned mission){return mission==4?1:mission>=6?2:0;}
inline unsigned contributionNextCity(const Game& g){for(unsigned city=0;city<4;++city)if(!contribution(g,city))return city;return 3;}
inline const char* contributionName(unsigned city){const char* names[]={"Raizes vivas","Memorias libertas","Rotas abertas","Estrutura renovada"};return names[city%4];}
inline bool campaignDonation(unsigned mission){return mission==4||mission==7;}
inline unsigned campaignGold(unsigned mission){return mission==1?120:mission==2?180:mission==3?350:mission==5||mission==6?100:0;}
inline unsigned campaignXp(const Game& g,unsigned mission){return (mission==1?100:mission==2?150:mission==3?350:mission>=4&&mission<=7?100:0)*(g.dndProgression?10:1);}
inline const char* campaignTitle(unsigned mission){return mission==1?"A CARGA DE CINZAS":mission==2?"O FAROL APAGADO":mission==3?"ARQUIVOS DA COROA":mission==4?"A RAIZ QUE RESISTE":mission==5?"NOMES ENTRE AS PEDRAS":mission==6?"O CAMINHO DOS VOLUNTARIOS":mission==7?"A FORJA DA PRIMEIRA LUZ":"A VIGILIA PERPETUA";}
inline unsigned campaignCity(unsigned mission){return mission>=4?mission-4:mission==3?3:2;}
inline unsigned campaignLevel(unsigned mission){return mission>=3?18:10;}
inline unsigned campaignEnemy(unsigned mission){return mission==5?5:mission==3||mission==7?7:6;}
inline unsigned campaignScenes(unsigned mission){return mission==3?6:4;}
inline bool campaignContact(const Game& g){unsigned m=campaignMission(g);return m&&g.city==campaignCity(m)&&campaignMemory(g);}
inline bool campaignValid(const Game& g){
 unsigned flags=g.campaignFlags,base=flags&7;if(flags&128||(base!=0&&base!=1&&base!=3&&base!=7)||(flags>7&&base!=7)||g.campaignStage>7||campaignDonation(g.campaignStage))return false;
 if(g.campaignFlags&&!campaignMemory(g))return false;
 if(!g.campaignStage)return true;
 return g.city==campaignCity(g.campaignStage)&&g.tutorial&&g.p.level>=campaignLevel(g.campaignStage)&&campaignMemory(g)&&g.campaignStage==campaignMission(g)&&g.enemyId==campaignEnemy(g.campaignStage)&&g.phase!=Phase::Home&&!g.discovery&&!g.tripStage&&!g.campStage&&!g.dungeonFlags&&g.eventStage!=2&&g.clubStage!=1&&g.clubStage!=2;
}
inline const char* campaignError(const Game& g,unsigned mission){
 if(!mission||mission>7||mission!=campaignMission(g))return "Missao ja resolvida";
 if(g.phase!=Phase::Home||g.campaignStage||g.discovery||g.tripStage||g.campStage||g.dungeonFlags||g.eventStage==2||g.clubStage==1||g.clubStage==2)return "Termine a acao atual";
 if(!g.tutorial||!campaignMemory(g))return "Encontre o Livro das Vigilias";
 if(g.city!=campaignCity(mission))return mission>=4?"Encontre o aliado desta cidade":mission==3?"Encontre Liora em Aurora":"Encontre Sabela em Mares";
 if(g.p.level<campaignLevel(mission))return mission>=3?"Prepare-se: requer nivel 18":"Prepare-se: requer nivel 10";
 if(mission==4&&g.rations<3)return "Leve 3 racoes de viagem";
 if(mission==7&&g.p.gold<150)return "Componentes custam 150 ouro";
 return nullptr;
}
inline void campaignReward(Game& g,unsigned mission){g.campaignFlags|=1u<<(mission-1);g.p.gold=std::min<uint32_t>(999999,g.p.gold+campaignGold(mission));unsigned xp=campaignXp(g,mission);g.p.xp=g.p.xp>UINT32_MAX-xp?UINT32_MAX:g.p.xp+xp;levelUp(g);}
inline const char* startCampaign(Game& g,unsigned mission){
 auto error=campaignError(g,mission);if(error)return error;
 if(campaignDonation(mission)){if(mission==4)g.rations-=3;else g.p.gold-=150;campaignReward(g,mission);return nullptr;}
 if(!begin(g,campaignEnemy(mission)))return "Encontro indisponivel";
 g.campaignStage=mission;return nullptr;
}
inline bool resolveCampaign(Game& g){
 if(!g.campaignStage||!campaignValid(g)||(g.phase!=Phase::Won&&g.phase!=Phase::Lost&&g.phase!=Phase::Fled))return false;
 unsigned mission=g.campaignStage;bool won=g.phase==Phase::Won;
 if(won)campaignReward(g,mission);
 g.campaignStage=0;return home(g);
}
}
