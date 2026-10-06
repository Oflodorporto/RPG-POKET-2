#pragma once
#include <stdint.h>
#include <algorithm>
#include "Gear.h"
// Adapted from Heltec 2026.09.24-polimento1: Combat, Skills, State,
// Economy, UI, World and Gear. Network/calendar are not part of this slice.
namespace rpg {
enum class Phase:uint8_t { Home, Hero, Enemy, Won, Lost, Fled };
enum class Action:uint8_t { Attack, Offensive, Defensive, Life, Mana, Flee };
struct EnemySpec {const char* name;uint8_t hp,atk,def,xp,gold;};
inline const EnemySpec& enemySpec(uint8_t id){static const EnemySpec e[]={{"GOBLIN",12,4,2,8,4},{"LOBO",14,5,2,10,5},{"ESQUELETO",17,5,4,12,6},{"GUARDIAO",28,6,6,25,15}};return e[id<4?id:2];}
struct Player {
  uint8_t cls=0, level=3, atk=6, def=4, life=0, mana=2;
  uint16_t hp=18,maxhp=18,mp=14,maxmp=14;
  uint32_t xp=0,gold=0;
};
struct Game {
  Player p;
  uint8_t race=0,shirt=0,trousers=0,clubStage=0;bool guildMember=false;uint32_t clubSession=0;
  uint8_t questId=0,questProgress=0,questLevel=0,city=0;
  uint32_t owned=0;
  uint8_t equipped[3]={},forge[3]={};
  Phase phase=Phase::Home;
  uint16_t enemyHp=17;
  uint8_t guard=0,enemyId=2,ruinsWins=0;
  bool guardianDefeated=false;
  uint32_t randomState=1;
  uint16_t gainXp=0,damage=0;
  uint8_t gainGold=0;
  bool crit=false,dodge=false,dropLife=false,dropMana=false,tutorial=false;
};
inline const char* className(uint8_t c) {static const char* n[]={"Mago","Cavaleiro","Guerreiro","Barbaro"};return n[c<4?c:0];}
inline const char* skillName(uint8_t c,bool defensive) {static const char* n[][2]={{"Raio","Barreira"},{"Investida","Escudo"},{"Precisao","Aparar"},{"Furia","Vigor"}};return n[c<4?c:0][defensive];}
inline uint16_t maxMana(uint8_t c,uint8_t l){return (c==0?12:8)+l-1;}
inline int effectiveAttack(const Game& g){return int(g.p.atk)+g.forge[0]*2+gearBonus(g.equipped[0]);}
inline int effectiveDefense(const Game& g){return int(g.p.def)+g.forge[1]*3+gearBonus(g.equipped[1]);}
inline uint16_t totalMana(const Game& g){return maxMana(g.p.cls,g.p.level)+g.forge[2]*3+gearBonus(g.equipped[2]);}
inline const char* forgeName(uint8_t slot){static const char* n[]={"ARMA","ARMADURA","AMULETO"};return slot<3?n[slot]:"INVALIDO";}
inline uint8_t forgeRequirement(uint8_t tier){return tier==0?1:tier==1?4:tier==2?8:255;}
inline uint16_t forgePrice(const Game& g,uint8_t slot){static const uint16_t base[]={40,45,35};return slot<3&&g.forge[slot]<3?base[slot]*(g.forge[slot]+1)*(g.forge[slot]+1):0;}
inline const char* forgeError(const Game& g,uint8_t slot){
  if(g.phase!=Phase::Home)return "Volte para a vila";
  if(slot>2)return "Melhoria indisponivel";
  if(g.forge[slot]>=3)return "Nivel maximo da forja";
  if(g.p.level<forgeRequirement(g.forge[slot]))return "Nivel insuficiente";
  if(g.p.gold<forgePrice(g,slot))return "Ouro insuficiente";
  return nullptr;
}
// Heltec permanent slot improvement, independent of the item worn in that slot.
inline const char* upgradeForge(Game& g,uint8_t slot){
  const char* err=forgeError(g,slot);if(err)return err;
  g.p.gold-=forgePrice(g,slot);++g.forge[slot];g.p.maxmp=totalMana(g);return nullptr;
}
inline const char* gearBuyError(const Game& g,uint8_t id){
  if(g.phase!=Phase::Home)return "Volte para a vila";
  if(!gearId(id))return "Item indisponivel";
  if(gearOwns(g.owned,id))return "Voce ja possui este item";
  if(!gearAllowed(id,g.p.cls,g.p.level))return "Classe ou nivel insuficiente";
  if(g.p.gold<gearPrice(id))return "Ouro insuficiente";
  return nullptr;
}
inline const char* buyGear(Game& g,uint8_t id){
  const char* err=gearBuyError(g,id);if(err)return err;
  g.p.gold-=gearPrice(id);g.owned|=1u<<(id-1);return nullptr;
}
// Toggle the selected owned item. Replacing an item never removes it from the bag.
inline const char* equipGear(Game& g,uint8_t id){
  if(g.phase!=Phase::Home)return "Volte para a vila";
  if(!gearOwns(g.owned,id))return "Item nao esta na bolsa";
  if(!gearAllowed(id,g.p.cls,g.p.level))return "Classe ou nivel insuficiente";
  uint8_t slot=gearSlot(id);g.equipped[slot]=g.equipped[slot]==id?0:id;
  g.p.maxmp=totalMana(g);g.p.mp=std::min(g.p.mp,g.p.maxmp);return nullptr;
}
inline uint16_t xpNeeded(uint8_t l){return uint16_t(std::min(uint32_t(65535),30u+uint32_t(l)*l*10));}
struct Contract {const char* name;const char* objective;int8_t enemy;uint8_t count,gold,percent;};
inline const Contract& contract(uint8_t id){static const Contract q[]={{"PATRULHA","Vencer 3 inimigos nas ruinas",-1,3,15,25},{"OSSOS DAS RUINAS","Vencer 2 esqueletos",2,2,18,35},{"O GUARDIAO","Vencer 1 Guardiao",3,1,35,60}};return q[id>=1&&id<=3?id-1:0];}
inline uint16_t contractXp(uint8_t id,uint8_t level){return id>=1&&id<=3&&level?uint32_t(xpNeeded(level))*contract(id).percent/100:0;}
inline uint16_t contractGold(uint8_t id,uint8_t level){return id>=1&&id<=3&&level?contract(id).gold+uint16_t(level-1)*2:0;}
inline void levelUp(Game& g){while(g.p.level<99&&g.p.xp>=xpNeeded(g.p.level)){g.p.xp-=xpNeeded(g.p.level);++g.p.level;g.p.maxmp=totalMana(g);g.p.maxhp=uint16_t(std::min(65535,int(g.p.maxhp)+2));if(g.p.atk<255)++g.p.atk;}}
inline bool questComplete(const Game& g){return g.questId>=1&&g.questId<=3&&g.questProgress>=contract(g.questId).count;}
inline const char* acceptQuest(Game& g,uint8_t id){
  if(g.phase!=Phase::Home)return "Volte para a vila";if(id<1||id>3)return "Contrato indisponivel";
  if(g.questId)return "Outra missao ativa";g.questId=id;g.questProgress=0;g.questLevel=g.p.level;return nullptr;
}
inline const char* abandonQuest(Game& g){
  if(g.phase!=Phase::Home)return "Volte para a vila";if(!g.questId)return "Nenhuma missao ativa";
  if(questComplete(g))return "Receba a recompensa";g.questId=g.questProgress=g.questLevel=0;return nullptr;
}
inline const char* claimQuest(Game& g){
  if(g.phase!=Phase::Home)return "Volte para a vila";if(!g.questId)return "Nenhuma missao ativa";
  if(!questComplete(g))return "Missao ainda incompleta";
  uint16_t gold=contractGold(g.questId,g.questLevel),xp=contractXp(g.questId,g.questLevel);
  if(g.p.gold>999999u-gold)return "Gaste ouro antes de receber";
  g.p.gold+=gold;g.p.xp=g.p.xp>UINT32_MAX-xp?UINT32_MAX:g.p.xp+xp;levelUp(g);
  g.questId=g.questProgress=g.questLevel=0;return nullptr;
}
inline uint8_t skillCost(uint8_t c,bool defensive){return defensive?(c==2?3:4):((c==1||c==3)?4:3);}
inline uint16_t recovery(uint16_t v,uint16_t max,bool mana){return v>=max?0:std::min<uint16_t>(max-v,std::max<uint16_t>(1,uint32_t(max)*(mana?50:30)/100));}
inline Game create(uint8_t c,uint32_t seed) {
  Game g;g.p.cls=c<4?c:0;g.randomState=seed?seed:1;
  const uint16_t hp[]={18,22,20,24};const uint8_t atk[]={6,7,8,9},def[]={4,6,5,3};
  g.p.hp=g.p.maxhp=hp[g.p.cls];g.p.atk=atk[g.p.cls];g.p.def=def[g.p.cls];
  g.p.mp=g.p.maxmp=maxMana(g.p.cls,3);return g;
}
inline uint32_t random(Game& g){uint32_t x=g.randomState;x^=x<<13;x^=x>>17;x^=x<<5;return g.randomState=x;}
inline int rollDamage(Game& g,int atk,int def){int d=std::max(1,atk+int(random(g)%3)-def/3);g.crit=random(g)%100<12;return g.crit?d+(d>>1):d;}
inline void clearFeedback(Game& g){g.damage=0;g.crit=g.dodge=false;}
inline bool begin(Game& g,uint8_t id=2){if(g.phase!=Phase::Home||!g.p.hp||id>3||(id==3&&g.ruinsWins<3))return false;g.enemyId=id;g.phase=Phase::Hero;g.enemyHp=enemySpec(id).hp;g.guard=0;g.gainXp=0;g.gainGold=0;g.dropLife=g.dropMana=false;clearFeedback(g);return true;}
inline bool explore(Game& g,bool boss=false){if(g.phase!=Phase::Home||!g.p.hp||(boss&&g.ruinsWins<3))return false;return begin(g,boss?3:uint8_t(random(g)%3));}
inline void finish(Game& g){
  if(g.phase!=Phase::Hero&&g.phase!=Phase::Enemy)return;
  if(!g.enemyHp){
    g.phase=Phase::Won;const auto& e=enemySpec(g.enemyId);g.gainXp=e.xp;g.gainGold=uint8_t(std::min<uint32_t>(e.gold,999999u-g.p.gold));g.p.gold+=g.gainGold;
    g.p.xp=g.p.xp>UINT32_MAX-e.xp?UINT32_MAX:g.p.xp+e.xp;
    if(g.enemyId==3)g.guardianDefeated=true;else if(g.ruinsWins<3)++g.ruinsWins;
    if(g.p.life<99 && (g.enemyId==3||random(g)%100<50)){++g.p.life;g.dropLife=true;}
    if(g.p.mana<99 && (g.enemyId==3||random(g)%100<25)){++g.p.mana;g.dropMana=true;}
    if(g.questId&&g.questProgress<contract(g.questId).count&&(contract(g.questId).enemy<0||contract(g.questId).enemy==g.enemyId))++g.questProgress;
    levelUp(g);
  }else if(!g.p.hp){g.phase=Phase::Lost;g.gainXp=uint16_t(std::min(g.p.xp,uint32_t(xpNeeded(g.p.level)/20)));g.p.xp-=g.gainXp;}
}
// Return nullptr on accepted action; rejected actions do not consume a turn.
inline const char* act(Game& g,Action a){
  if(g.phase!=Phase::Hero)return "Aguarde seu turno";
  if(a==Action::Offensive||a==Action::Defensive){
    uint8_t cost=skillCost(g.p.cls,a==Action::Defensive);if(g.p.mp<cost)return "Mana insuficiente";g.p.mp-=cost;
  }
  if(a==Action::Life||a==Action::Mana){
    bool m=a==Action::Mana;auto &v=m?g.p.mp:g.p.hp;auto &count=m?g.p.mana:g.p.life;
    if(!count)return "Sem pocoes";uint16_t heal=recovery(v,m?g.p.maxmp:g.p.maxhp,m);if(!heal)return "Ja esta cheio";
    v+=heal;--count;clearFeedback(g);g.damage=heal;g.phase=Phase::Enemy;return nullptr;
  }
  clearFeedback(g);
  if(a==Action::Flee){g.phase=random(g)%100<65?Phase::Fled:Phase::Enemy;return nullptr;}
  if(a==Action::Defensive){
    g.guard=g.p.cls<2?75:50;
    if(g.p.cls==3)g.p.hp=std::min<uint16_t>(g.p.maxhp,g.p.hp+std::max<uint16_t>(1,g.p.maxhp/4));
    g.phase=Phase::Enemy;return nullptr;
  }
  bool skill=a==Action::Offensive;int damage=rollDamage(g,effectiveAttack(g),skill&&g.p.cls==0?0:enemySpec(g.enemyId).def);
  if(skill)damage=g.p.cls==0?damage*180/100:g.p.cls==3?damage*2:damage*150/100;
  g.dodge=!(skill&&g.p.cls==2)&&random(g)%100<6;
  if(!g.dodge){g.damage=uint16_t(std::min<int>(g.enemyHp,damage));g.enemyHp-=g.damage;}
  g.phase=Phase::Enemy;finish(g);return nullptr;
}
inline bool enemy(Game& g){
  if(g.phase!=Phase::Enemy)return false;clearFeedback(g);int damage=rollDamage(g,enemySpec(g.enemyId).atk,effectiveDefense(g));
  if(g.guard){damage=(damage*(100-g.guard)+99)/100;g.guard=0;}
  g.dodge=random(g)%100<6;
  if(!g.dodge){g.damage=uint16_t(std::min<int>(g.p.hp,damage));g.p.hp-=g.damage;}
  g.phase=Phase::Hero;finish(g);return true;
}
inline bool home(Game& g){if(g.phase!=Phase::Won&&g.phase!=Phase::Lost&&g.phase!=Phase::Fled)return false;g.phase=Phase::Home;if(!g.p.hp)g.p.hp=1;return true;}
inline bool rest(Game& g){if(g.phase!=Phase::Home)return false;g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;return true;}
// Economy.cpp shopLong / UI.cpp invLong: prices and limits from Heltec.
inline uint8_t potionPrice(bool mana){return mana?12:10;}
inline const char* buyPotion(Game& g,bool mana){
  if(g.phase!=Phase::Home)return "Volte para a vila";
  uint8_t& count=mana?g.p.mana:g.p.life;uint8_t price=potionPrice(mana);
  if(count>=99)return "Bolsa cheia";
  if(g.p.gold<price)return "Ouro insuficiente";
  g.p.gold-=price;++count;return nullptr;
}
inline const char* usePotionAtHome(Game& g,bool mana){
  if(g.phase!=Phase::Home)return "Volte para a vila";
  uint8_t& count=mana?g.p.mana:g.p.life;uint16_t& value=mana?g.p.mp:g.p.hp;
  if(!count)return "Sem pocoes";
  uint16_t amount=recovery(value,mana?g.p.maxmp:g.p.maxhp,mana);
  if(!amount)return "Ja esta cheio";
  value+=amount;--count;return nullptr;
}
}


namespace rpg {
inline const char* joinGuild(Game& g){if(g.phase!=Phase::Home)return "Volte ao refugio";if(g.guildMember)return "Voce ja e membro";if(g.p.gold<100)return "Precisa de 100 ouro";g.p.gold-=100;g.guildMember=true;return nullptr;}
inline const char* reserveFight(Game& g,uint32_t id){if(g.phase!=Phase::Home)return "Volte ao refugio";if(!g.guildMember)return "Entre na guilda primeiro";if(g.p.level<5)return "Clube exige nivel 5";if(g.clubStage==1||g.clubStage==2)return "Duelo ja reservado";if(!id||id==g.clubSession)return "Sessao repetida";if(g.p.gold<25)return "Precisa de 25 ouro";g.p.gold-=25;g.clubSession=id;g.clubStage=1;return nullptr;}
inline bool refundFight(Game& g){if(g.clubStage!=1)return false;g.p.gold=std::min<uint32_t>(999999,g.p.gold+25);g.clubStage=3;return true;}
inline bool startFight(Game& g,uint32_t id){if(g.clubStage!=1||g.clubSession!=id)return false;g.clubStage=2;return true;}
inline bool settleFight(Game& g,uint32_t id,int result){if(g.clubStage!=2||g.clubSession!=id||result<-1||result>1)return false;g.p.gold=std::min<uint32_t>(999999,g.p.gold+(result>0?45:result==0?25:0));g.clubStage=3;return true;}
}
