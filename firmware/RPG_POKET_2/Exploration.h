#pragma once
#include "Rules.h"
namespace rpg {
// Discovery: 0 none, 1 find, 2 unopened chest, 3 local event,
// 4 mimic combat (no second chest reward), 5 collected receipt.
inline void clearLock(Game& g){g.chestLock=g.chestTries=g.chestRoll=0;g.chestPick=g.chestTrap=false;}
inline void clearDiscovery(Game& g){clearLock(g);g.discovery=g.discoverLoot=g.discoverAmount=0;}
inline unsigned lockDifficulty(const Game& g){return 12+2*g.city;}
inline int lockBonus(const Game& g,bool pick){const int force[]={0,2,3,4},dex[]={2,1,2,1};return g.dndProgression?abilityMod(g.attributes[pick?1:0]):(pick?dex[g.p.cls<4?g.p.cls:0]:force[g.p.cls<4?g.p.cls:0]);}
inline bool lockSuccess(const Game& g){return g.chestRoll==20||(g.chestRoll!=1&&int(g.chestRoll)+lockBonus(g,g.chestPick)>=int(lockDifficulty(g)));}
inline unsigned lockChance(const Game& g,bool pick){unsigned n=0;for(int roll=1;roll<=20;++roll)n+=roll==20||(roll!=1&&roll+lockBonus(g,pick)>=int(lockDifficulty(g)));return n*5;}
inline unsigned trapCeiling(const Game& g){return g.chestTrap?std::min<unsigned>(g.p.hp?g.p.hp-1:0,2+g.city):0;}
inline bool lockValid(const Game& g){
 if(g.gazuas>9||g.chestLock>3||g.chestTries>2||g.chestRoll>20)return false;
 if(!g.chestLock)return !g.chestTries&&!g.chestRoll&&!g.chestPick&&!g.chestTrap;
 if(g.discovery!=2||g.discoverLoot==7||g.phase!=Phase::Home)return false;
 if(!g.chestTries)return g.chestLock==1&&!g.chestRoll&&!g.chestPick;
 if(!g.chestRoll)return false;
 if(g.chestLock==2)return lockSuccess(g);
 if(lockSuccess(g))return false;
 if(g.chestLock==1)return g.chestTries==1&&g.chestPick;
 return g.chestTries==2||!g.chestPick;
}
inline unsigned gazuaPrice(const Game& g){const unsigned prices[]={4,7,6,10};return prices[g.city<4?g.city:0];}
inline const char* buyGazua(Game& g){if(g.phase!=Phase::Home||g.discovery||g.tripStage||g.campStage||g.dungeonFlags||g.campaignStage||g.eventStage==2||g.clubStage==1||g.clubStage==2)return "Termine a acao atual";if(g.gazuas>=9)return "Limite de 9 gazuas";if(g.p.gold<gazuaPrice(g))return "Ouro insuficiente";g.p.gold-=gazuaPrice(g);++g.gazuas;return nullptr;}
inline const char* attemptLock(Game& g,bool pick){
 if(g.phase!=Phase::Home||g.discovery!=2||g.chestLock!=1)return "Fechadura indisponivel";
 if(g.chestTries>=2)return "Sem tentativas";if(pick&&!g.gazuas)return "Sem gazuas; pode forcar ou deixar";
 if(pick)--g.gazuas;++g.chestTries;g.chestPick=pick;g.chestRoll=1+random(g)%20;
 if(lockSuccess(g))g.chestLock=2;
 else {g.p.hp-=trapCeiling(g);if(!pick||g.chestTries==2)g.chestLock=3;}
 return nullptr;
}
inline bool discoveryValid(const Game& g){
 if(!lockValid(g))return false;
 if(g.scrap>9||g.discovery>7)return false;
 if(!g.discovery)return !g.discoverLoot&&!g.discoverAmount&&(g.enemyId<14||g.enemyId>17||g.phase==Phase::Home);
 if(g.tripStage||g.campStage||g.dungeonFlags||g.campaignStage||g.eventStage==2||g.clubStage==1||g.clubStage==2)return false;
 if((g.discovery==6||g.discovery==7)&&(g.questId<4||contractKind(g.questId)!=ContractKind::Recover||g.city!=contractCity(g.questId)||(g.discovery==6?questComplete(g):!g.questProgress)||g.discoverLoot!=3||g.discoverAmount!=1))return false;
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
 if(roll<50)return explore(g,false,night);
 g.discovery=roll<70?1:roll<90?2:3;
 // Store the rolled reward before displaying or accepting a decision.
 unsigned reward=random(g)%100;
 g.discoverLoot=reward<48?0:reward<62?1:reward<75?2:reward<86?3:reward<93?4:reward<98?5:6;
 g.discoverAmount=g.discoverLoot==0?uint8_t(3+g.city*4+random(g)%6):g.discoverLoot==6?uint8_t(g.p.cls*3+1+random(g)%std::min<unsigned>(g.city+1,g.p.level>=8?3:g.p.level>=4?2:1)):1;
 if(g.discovery==3){g.discoverLoot=night?2:3;g.discoverAmount=1;}
 // Active recovery:20% finds+10% local meetings become tracked cargo.
 if((g.discovery==1||g.discovery==3)&&g.questId>=4&&contractKind(g.questId)==ContractKind::Recover&&g.city==contractCity(g.questId)&&!questComplete(g)){g.discovery=6;g.discoverLoot=3;g.discoverAmount=1;}
 // Mimic outcome is fixed at discovery, concealed behind its warning.
 if(g.discovery==2&&random(g)%100<5)g.discoverLoot=7;
 if(g.discovery==2&&g.discoverLoot!=7&&random(g)%100<40){g.chestLock=1;g.chestTrap=random(g)%100<25;if(g.discoverLoot==0)g.discoverAmount+=8+g.city*4;}
 return true;
}
inline const char* discoveryName(const Game& g){return g.discovery>=6?"CARGA RECUPERADA":g.discovery==2?"BAU ESQUECIDO":g.discovery==3?"ENCONTRO LOCAL":g.discovery==5?"ACHADO RECOLHIDO":"ALGO PELO CAMINHO";}
inline const char* lootName(unsigned id){const char* n[]={"Moedas de ouro","Pocao de vida","Pocao de mana","Racao de viagem","Bota velha","Roupa rasgada","Equipamento","Bau inquieto"};return n[id<8?id:0];}
inline const char* collectDiscovery(Game& g){
 if(g.phase!=Phase::Home||!g.discovery||g.discovery==4||g.discovery==5||g.discovery==7)return "Nada para recolher";
 if(g.chestLock&&g.chestLock!=2)return "Abra a fechadura primeiro";
 if(g.discoverLoot==7){g.discovery=4;g.discoverLoot=0;g.discoverAmount=1;if(begin(g,14+g.city))return nullptr;return "Encontro indisponivel";}
 if(g.discovery==6){++g.questProgress;clearLock(g);g.discovery=7;g.discoverLoot=3;return nullptr;}
 if(g.discoverLoot==0){if(g.p.gold>999999u-g.discoverAmount)return "Gaste ouro ou deixe o achado";g.p.gold+=g.discoverAmount;}
 else if(g.discoverLoot==6){uint32_t bit=1u<<(g.discoverAmount-1);if(g.owned&bit){unsigned gold=6+g.city*3;if(g.p.gold>999999u-gold)return "Gaste ouro ou deixe o achado";g.p.gold+=gold;g.discoverLoot=0;g.discoverAmount=gold;}else g.owned|=bit;}
 else {uint8_t& n=g.discoverLoot==1?g.p.life:g.discoverLoot==2?g.p.mana:g.discoverLoot==3?g.rations:g.scrap;unsigned cap=g.discoverLoot<3?99:9;if(n>=cap)return "Bolsa cheia; pode deixar aqui";++n;}
 clearLock(g);g.discovery=5;return nullptr;
}
inline const char* sellScrap(Game& g,bool discard=false){if(g.phase!=Phase::Home||g.discovery)return "Termine o encontro";if(!g.scrap)return "Sem sucata";if(!discard&&g.p.gold>999999u-g.scrap*2)return "Gaste ouro primeiro";if(!discard)g.p.gold+=g.scrap*2;g.scrap=0;return nullptr;}
}
