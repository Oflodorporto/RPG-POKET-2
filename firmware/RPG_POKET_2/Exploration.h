#pragma once
#include "Rules.h"
namespace rpg {
// Discovery: 0 none, 1 find, 2 unopened chest, 3 local event,
// 4 mimic combat (no second chest reward), 5 collected receipt.
inline void clearDiscovery(Game& g){g.discovery=g.discoverLoot=g.discoverAmount=0;}
inline bool discoveryValid(const Game& g){
 if(g.scrap>9||g.discovery>5)return false;
 if(!g.discovery)return !g.discoverLoot&&!g.discoverAmount&&(g.enemyId<14||g.phase==Phase::Home);
 if(g.tripStage||g.campStage||g.dungeonFlags||g.campaignStage||g.eventStage==2||g.clubStage==1||g.clubStage==2)return false;
 if(g.discoverLoot>7||!g.discoverAmount)return false;
 if(g.discoverLoot==7&&g.discovery!=2)return false;
 if(g.discoverLoot==6&&!gearId(g.discoverAmount))return false;
 if(g.discoverLoot!=6&&g.discoverAmount>99)return false;
 if(g.discovery==4)return g.enemyId==14+g.city&&g.phase!=Phase::Home;
 return g.phase==Phase::Home;
}
inline bool startDiscovery(Game& g,bool night=false){
 if(g.discovery||g.phase!=Phase::Home||g.tripStage||g.campStage||g.dungeonFlags||g.campaignStage||g.eventStage==2||g.clubStage==1||g.clubStage==2||!g.p.hp)return false;
 unsigned roll=random(g)%100;
 if(roll<50)return explore(g);
 g.discovery=roll<70?1:roll<90?2:3;
 // Store the rolled reward before displaying or accepting a decision.
 unsigned reward=random(g)%100;
 g.discoverLoot=reward<48?0:reward<62?1:reward<75?2:reward<86?3:reward<93?4:reward<98?5:6;
 g.discoverAmount=g.discoverLoot==0?uint8_t(3+g.city*4+random(g)%6):g.discoverLoot==6?uint8_t(g.p.cls*3+1+random(g)%std::min<unsigned>(g.city+1,g.p.level>=8?3:g.p.level>=4?2:1)):1;
 if(g.discovery==3){g.discoverLoot=night?2:3;g.discoverAmount=1;}
 // Mimic outcome is fixed at discovery, concealed behind its warning.
 if(g.discovery==2&&random(g)%100<5)g.discoverLoot=7;
 return true;
}
inline const char* discoveryName(const Game& g){return g.discovery==2?"BAU ESQUECIDO":g.discovery==3?"ENCONTRO NA ESTRADA":g.discovery==5?"ACHADO RECOLHIDO":"ALGO PELO CAMINHO";}
inline const char* lootName(unsigned id){const char* n[]={"Moedas de ouro","Pocao de vida","Pocao de mana","Racao de viagem","Bota velha","Roupa rasgada","Equipamento","Bau inquieto"};return n[id<8?id:0];}
inline const char* collectDiscovery(Game& g){
 if(g.phase!=Phase::Home||!g.discovery||g.discovery==4||g.discovery==5)return "Nada para recolher";
 if(g.discoverLoot==7){g.discovery=4;g.discoverLoot=0;g.discoverAmount=1;if(begin(g,14+g.city))return nullptr;return "Encontro indisponivel";}
 if(g.discoverLoot==0){if(g.p.gold>999999u-g.discoverAmount)return "Gaste ouro ou deixe o achado";g.p.gold+=g.discoverAmount;}
 else if(g.discoverLoot==6){uint32_t bit=1u<<(g.discoverAmount-1);if(g.owned&bit){unsigned gold=6+g.city*3;if(g.p.gold>999999u-gold)return "Gaste ouro ou deixe o achado";g.p.gold+=gold;g.discoverLoot=0;g.discoverAmount=gold;}else g.owned|=bit;}
 else {uint8_t& n=g.discoverLoot==1?g.p.life:g.discoverLoot==2?g.p.mana:g.discoverLoot==3?g.rations:g.scrap;unsigned cap=g.discoverLoot<3?99:9;if(n>=cap)return "Bolsa cheia; pode deixar aqui";++n;}
 g.discovery=5;return nullptr;
}
inline const char* sellScrap(Game& g,bool discard=false){if(g.phase!=Phase::Home||g.discovery)return "Termine o encontro";if(!g.scrap)return "Sem sucata";if(!discard&&g.p.gold>999999u-g.scrap*2)return "Gaste ouro primeiro";if(!discard)g.p.gold+=g.scrap*2;g.scrap=0;return nullptr;}
}
