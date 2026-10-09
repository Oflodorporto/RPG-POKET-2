#pragma once
#include <stdint.h>
#include <algorithm>
#include <stdlib.h>
#include "Gear.h"
// Adapted from Heltec 2026.09.24-polimento1: Combat, Skills, State,
// Economy, UI, World and Gear. Network/calendar are not part of this slice.
namespace rpg {
enum class Phase:uint8_t { Home, Hero, Enemy, Won, Lost, Fled };
enum class Action:uint8_t { Attack, Offensive, Defensive, Life, Mana, Flee, MagicMissile, BurningHands, ShieldSpell, ScorchingRay, Fireball, LayHands, SacredWeapon, TurnUndead, SecondWind, ActionSurge, RagePower, DivineSmite };
struct EnemySpec {const char* name;uint8_t hp,atk,def,xp,gold;};
inline const EnemySpec& enemySpec(uint8_t id){static const EnemySpec e[]={{"GOBLIN",12,4,2,8,4},{"LOBO",14,5,2,10,5},{"ESQUELETO",17,5,4,12,6},{"GUARDIAO",28,6,6,25,15},{"JAVALI",20,6,3,14,6},{"ESPECTRO",42,9,7,38,15},{"SAQUEADOR",76,13,10,85,28},{"SENTINELA",124,19,16,180,48},{"ARCONTE",92,10,7,190,65},{"VIGIA OSSUDO",35,7,5,30,9},{"HIPOGRIFO JOVEM",20,5,3,0,0},{"HIPOGRIFO",45,8,6,0,0},{"HIPOGRIFO MARCADO",90,13,10,0,0},{"HIPOGRIFO ALFA",160,21,16,0,0},{"MIMICO DO BOSQUE",18,5,2,14,9},{"MIMICO DAS RUINAS",45,9,6,40,20},{"MIMICO DO PORTO",85,14,10,95,35},{"MIMICO DO CASTELO",150,20,16,190,55}};return e[id<18?id:2];}
struct Player {
  uint8_t cls=0, level=3, atk=6, def=4, life=0, mana=2;
  uint16_t hp=18,maxhp=18,mp=14,maxmp=14;
  uint32_t xp=0,gold=0;
};
struct Game {
  Player p;
  uint8_t race=0,shirt=0,trousers=0,clubStage=0;bool guildMember=false;uint32_t clubSession=0;
  uint8_t questId=0,questProgress=0,questLevel=0,city=0;
  uint8_t tripStage=0,tripTo=0,tripRoll=0,tripDifficulty=0,tripTotal=0,tripEnemy=0,tripSurvival=0,tripLuck=0;
  uint8_t rations=0,charts=0,charms=0;
  uint8_t campStage=0,campRoll=0;bool campRation=false,campKit=false,sleepKit=false;
  uint32_t eventDay=0;uint8_t eventStage=0,eventTier=0,eventOriginCity=0,eventOriginPage=0;
  uint8_t crystals=0,dungeonFlags=0,dungeonXY=0,dungeonLoot=0,dungeonEnemies=0,dungeonClears=0;
  uint32_t owned=0;
  uint8_t equipped[3]={},forge[3]={};
  Phase phase=Phase::Home;
  uint16_t enemyHp=17;
  uint8_t guard=0,enemyId=2,ruinsWins=0;
  bool guardianDefeated=false;bool dndProgression=false,tough=false;uint8_t attributes[6]={8,12,14,16,12,10},advancementSpent=0;
  uint8_t oath=0,laySpent=0,sacredTurns=0,turnedTurns=0;bool channelSpent=false;
  uint8_t enemyBeat=0;
  uint8_t campaignFlags=0,campaignStage=0;
  bool originStory=false;uint8_t originPage=0;
  uint8_t discovery=0,discoverLoot=0,discoverAmount=0,scrap=0;
  uint8_t gazuas=0,chestLock=0,chestTries=0,chestRoll=0;bool chestPick=false,chestTrap=false;
  uint8_t windSpent=0,surgeSpent=0,rageSpent=0,rageTurns=0;bool surgePending=false,surgeTurnUsed=false;
  uint32_t randomState=1;
  uint16_t gainXp=0,damage=0;
  uint8_t gainGold=0;
  bool crit=false,dodge=false,dropLife=false,dropMana=false,tutorial=false;
};

constexpr uint32_t dndXp[]={0,300,900,2700,6500,14000,23000,34000,48000,64000,85000,100000,120000,140000,165000,195000,225000,265000,305000,355000};
inline int abilityMod(unsigned score){return int(score)/2-5;}
inline unsigned proficiency(unsigned level){return 2+(std::min(20u,std::max(1u,level))-1)/4;}
inline unsigned primaryAbility(unsigned cls){return cls==0?3:0;}
inline unsigned improvementCount(unsigned cls,unsigned level){unsigned n=0;for(unsigned l:{4u,8u,12u,16u,19u})n+=level>=l;if(cls==2)n+=(level>=6)+(level>=14);return n;}
inline unsigned advancementPoints(const Game& g){return g.dndProgression?improvementCount(g.p.cls,g.p.level)*2-g.advancementSpent:0;}
inline const char* abilityName(unsigned i){const char* n[]={"Forca","Destreza","Constituicao","Inteligencia","Sabedoria","Carisma"};return n[i%6];}
inline uint8_t cityLevel(uint8_t c){static const uint8_t n[]={3,5,10,18};return n[c<4?c:0];}
inline const char* citySpecialty(uint8_t c){static const char* n[]={"Bosque / suprimentos baratos","Ruinas / reliquias raras","Porto / armas importadas","Castelo / armaduras superiores"};return n[c<4?c:0];}
inline uint16_t priced(uint16_t base,unsigned percent){return (uint32_t(base)*percent+99)/100;}
inline bool cityStock(const Game& g,uint8_t id){if(!gearId(id))return false;unsigned t=gearTier(id),slot=gearSlot(id);return g.city==0?t==1:g.city==1?(slot==2||t==1):g.city==2?(t<=2):true;}
inline uint8_t cityGearCount(const Game& g){uint8_t n=0;for(unsigned i=0;i<9;++i)if(cityStock(g,gearOffer(g.p.cls,i)))++n;return n;}
inline uint8_t cityGearOffer(const Game& g,unsigned index){for(unsigned i=0;i<9;++i){uint8_t id=gearOffer(g.p.cls,i);if(cityStock(g,id)&&!index--)return id;}return 0;}
inline uint16_t cityGearPrice(const Game& g,uint8_t id){unsigned slot=gearSlot(id);unsigned pct=g.city==0?100:g.city==1?(slot==2?75:150):g.city==2?(slot==0?85:115):(slot==1?90:140);return priced(gearPrice(id),pct);}
inline uint8_t survival(const Game& g){static const uint8_t n[]={1,3,3,5};return std::min(10u,unsigned(n[g.p.cls])+g.p.level/5+(g.dndProgression?std::max(0,abilityMod(g.attributes[4])):0));}
inline uint8_t luck(const Game& g){static const uint8_t n[]={3,2,2,1};return std::min(8u,unsigned(n[g.p.cls])+g.p.level/8);}
inline uint8_t localGood(uint8_t city){return city==2?1:city==3?2:0;}
inline const char* goodName(uint8_t id){static const char* n[]={"RACAO DE VIAGEM","MAPA DO VIAJANTE","TALISMA DA SORTE"};return n[id<3?id:0];}
inline const char* goodBonus(uint8_t id){static const char* n[]={"+2 sobrevivencia","+1 sobrevivencia / +1 sorte","+2 sorte"};return n[id<3?id:0];}
inline uint8_t goodPrice(uint8_t city){static const uint8_t n[]={6,12,12,25};return n[city<4?city:0];}
inline uint8_t& goodCount(Game& g,uint8_t id){return id==1?g.charts:id==2?g.charms:g.rations;}
inline const char* buyGood(Game& g){if(g.phase!=Phase::Home||g.tripStage)return "Termine a viagem";auto& n=goodCount(g,localGood(g.city));if(n>=9)return "Limite de 9 unidades";if(g.p.gold<goodPrice(g.city))return "Ouro insuficiente";g.p.gold-=goodPrice(g.city);++n;return nullptr;}
inline void clearTrip(Game& g){g.tripStage=g.tripTo=g.tripRoll=g.tripDifficulty=g.tripTotal=g.tripEnemy=g.tripSurvival=g.tripLuck=0;}
inline bool tripSafe(const Game& g){return g.tripRoll==20||(g.tripRoll!=1&&g.tripTotal>=g.tripDifficulty);}
inline uint8_t routeDifficulty(uint8_t a,uint8_t b){return 12+2*abs(int(a)-int(b))+std::max(cityLevel(a),cityLevel(b))/6;}

inline const char* className(uint8_t c) {static const char* n[]={"Mago","Cavaleiro","Guerreiro","Barbaro"};return n[c<4?c:0];}
inline const char* skillName(uint8_t c,bool defensive) {static const char* n[][2]={{"Raio","Barreira"},{"Investida","Escudo"},{"Precisao","Aparar"},{"Furia","Vigor"}};return n[c<4?c:0][defensive];}
inline uint16_t maxMana(uint8_t c,uint8_t l){return (c==0?12:8)+l-1;}
inline int effectiveAttack(const Game& g){return int(g.p.atk)+(g.dndProgression?abilityMod(g.attributes[primaryAbility(g.p.cls)])+int(proficiency(g.p.level))-2:0)+g.forge[0]*2+gearBonus(g.equipped[0]);}
inline int effectiveDefense(const Game& g){return int(g.p.def)+(g.dndProgression?std::max(0,abilityMod(g.attributes[1])):0)+g.forge[1]*3+gearBonus(g.equipped[1]);}
inline uint16_t totalMana(const Game& g){return maxMana(g.p.cls,g.p.level)+(g.dndProgression&&g.p.cls<2?std::max(0,abilityMod(g.attributes[g.p.cls==0?3:5])):0)+g.forge[2]*3+gearBonus(g.equipped[2]);}
inline const char* forgeName(uint8_t slot){static const char* n[]={"ARMA","ARMADURA","AMULETO"};return slot<3?n[slot]:"INVALIDO";}
inline uint8_t forgeRequirement(uint8_t tier){return tier==0?1:tier==1?4:tier==2?8:255;}
inline uint16_t forgePrice(const Game& g,uint8_t slot){static const uint16_t base[]={40,45,35};return slot<3&&g.forge[slot]<3?priced(base[slot]*(g.forge[slot]+1)*(g.forge[slot]+1),g.city==0?100:g.city==1?160:g.city==2?120:85):0;}
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
  if(!cityStock(g,id))return "Item ausente nesta cidade";
  if(gearOwns(g.owned,id))return "Voce ja possui este item";
  if(!gearAllowed(id,g.p.cls,g.p.level))return "Classe ou nivel insuficiente";
  if(g.p.gold<cityGearPrice(g,id))return "Ouro insuficiente";
  return nullptr;
}
inline const char* buyGear(Game& g,uint8_t id){
  const char* err=gearBuyError(g,id);if(err)return err;
  g.p.gold-=cityGearPrice(g,id);g.owned|=1u<<(id-1);return nullptr;
}
// Toggle the selected owned item. Replacing an item never removes it from the bag.
inline const char* equipGear(Game& g,uint8_t id){
  if(g.phase!=Phase::Home)return "Volte para a vila";
  if(!gearOwns(g.owned,id))return "Item nao esta na bolsa";
  if(!gearAllowed(id,g.p.cls,g.p.level))return "Classe ou nivel insuficiente";
  uint8_t slot=gearSlot(id);g.equipped[slot]=g.equipped[slot]==id?0:id;
  g.p.maxmp=totalMana(g);g.p.mp=std::min(g.p.mp,g.p.maxmp);return nullptr;
}
inline uint16_t dndXpNeeded(unsigned l){return l<20?uint16_t(dndXp[std::max(1u,l)]-dndXp[std::max(1u,l)-1]):1;}
inline uint16_t xpNeeded(uint8_t l){return uint16_t(std::min(uint32_t(65535),30u+uint32_t(l)*l*10));}
inline uint16_t xpNeeded(const Game& g){return g.dndProgression?dndXpNeeded(g.p.level):xpNeeded(g.p.level);}
inline uint32_t totalExperience(const Game& g){return g.dndProgression?dndXp[std::min(20u,unsigned(g.p.level))-1]+g.p.xp:g.p.xp;}
inline unsigned attacksPerAction(const Game& g){if(!g.dndProgression||g.p.cls==0)return 1;if(g.p.cls==2)return g.p.level>=20?4:g.p.level>=11?3:g.p.level>=5?2:1;return g.p.level>=5?2:1;}
inline unsigned hpPerLevel(const Game& g){const unsigned avg[]={4,6,6,7};return std::max(1,int(avg[g.p.cls])+abilityMod(g.attributes[2]))+unsigned(g.tough)*2;}
inline unsigned spellCircle(const Game& g){return g.p.cls==0?std::min(9u,(unsigned(g.p.level)+1)/2):g.p.cls==1&&g.p.level>=2?std::min(5u,(unsigned(g.p.level)+3)/4):0;}
inline const char* improveAttribute(Game& g,unsigned i){if(g.phase!=Phase::Home||g.tripStage||g.campStage||g.dungeonFlags)return "Termine a acao atual";if(i>=6||!advancementPoints(g))return "Sem pontos disponiveis";if(g.attributes[i]>=20)return "Limite de atributo: 20";int old=abilityMod(g.attributes[i]);++g.attributes[i];++g.advancementSpent;if(i==2){unsigned gain=(abilityMod(g.attributes[i])-old)*g.p.level;g.p.maxhp+=gain;g.p.hp+=gain;}g.p.maxmp=totalMana(g);return nullptr;}
inline const char* learnTough(Game& g){if(g.phase!=Phase::Home||g.tripStage||g.campStage||g.dungeonFlags)return "Termine a acao atual";if(g.tough)return "Talento ja aprendido";if(advancementPoints(g)<2||g.advancementSpent%2)return "Requer 2 pontos do mesmo marco";g.tough=true;g.advancementSpent+=2;g.p.maxhp+=g.p.level*2;g.p.hp+=g.p.level*2;return nullptr;}
struct Contract {const char* name;const char* objective;int8_t enemy;uint8_t count,gold,percent;};
inline const Contract& contract(uint8_t id){static const Contract q[]={{"PATRULHA","Vencer 3 inimigos nas ruinas",-1,3,15,25},{"OSSOS DAS RUINAS","Vencer 2 esqueletos",2,2,18,35},{"O GUARDIAO","Vencer 1 Guardiao",3,1,35,60}};return q[id>=1&&id<=3?id-1:0];}
inline uint16_t contractXp(uint8_t id,uint8_t level){return id>=1&&id<=3&&level?uint32_t(xpNeeded(level))*contract(id).percent/100:0;}
inline uint16_t contractXp(const Game& g,uint8_t id,uint8_t level){return g.dndProgression&&id>=1&&id<=3&&level?uint32_t(dndXpNeeded(level))*contract(id).percent/100:contractXp(id,level);}
inline uint16_t contractGold(uint8_t id,uint8_t level){return id>=1&&id<=3&&level?contract(id).gold+uint16_t(level-1)*2:0;}
inline void levelUp(Game& g){while(g.p.level<(g.dndProgression?20:99)&&g.p.xp>=xpNeeded(g)){g.p.xp-=xpNeeded(g);++g.p.level;g.p.maxmp=totalMana(g);g.p.maxhp=uint16_t(std::min(65535u,unsigned(g.p.maxhp)+(g.dndProgression?hpPerLevel(g):2u)));if(g.p.atk<255)++g.p.atk;}if(g.dndProgression&&g.p.level==20){g.p.xp=0;if(g.p.cls==3)g.rageSpent=0;}}
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
  uint16_t gold=contractGold(g.questId,g.questLevel),xp=contractXp(g,g.questId,g.questLevel);
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
  g.dndProgression=true;g.p.level=1;
  const uint8_t scores[][6]={{8,14,14,16,12,10},{16,10,14,8,12,14},{16,14,14,8,12,10},{16,14,16,8,12,8}};
  for(unsigned i=0;i<6;++i)g.attributes[i]=scores[g.p.cls][i];
  const unsigned die[]={6,10,10,12};g.p.hp=g.p.maxhp=die[g.p.cls]+abilityMod(g.attributes[2]);
  g.p.life=2;g.rations=1;g.p.mp=g.p.maxmp=totalMana(g);return g;
}
inline uint32_t random(Game& g){uint32_t x=g.randomState;x^=x<<13;x^=x>>17;x^=x<<5;return g.randomState=x;}
inline unsigned surgeUses(const Game& g){return g.p.level<2?0:g.p.level>=17?2:1;}
inline unsigned rageUses(const Game& g){return g.p.level>=20?255:g.p.level>=17?6:g.p.level>=12?5:g.p.level>=6?4:g.p.level>=3?3:2;}
inline unsigned rageBonus(const Game& g){return g.p.level>=16?4:g.p.level>=9?3:2;}
// Spectres and the Arconte deal magical damage; other current encounters are physical.
inline bool physicalEnemy(unsigned id){return id!=5&&id!=8;}
inline void clearMartialCombat(Game& g){g.rageTurns=0;g.surgePending=g.surgeTurnUsed=false;}
inline void endHeroAction(Game& g){if(g.surgePending){g.surgePending=false;g.phase=Phase::Hero;}else g.phase=Phase::Enemy;}
inline bool powerAction(Action a){return uint8_t(a)>=uint8_t(Action::MagicMissile)&&uint8_t(a)<=uint8_t(Action::DivineSmite);}
inline const char* powerName(Action a){switch(a){case Action::MagicMissile:return "Misseis magicos";case Action::BurningHands:return "Maos flamejantes";case Action::ShieldSpell:return "Escudo arcano";case Action::ScorchingRay:return "Raios abrasadores";case Action::Fireball:return "Bola de fogo";case Action::LayHands:return "Impor as maos";case Action::SacredWeapon:return "Arma sagrada";case Action::TurnUndead:return "Expulsar profanos";case Action::SecondWind:return "Segundo folego";case Action::ActionSurge:return "Surto de acao";case Action::RagePower:return "Furia de batalha";case Action::DivineSmite:return "Punicao divina";default:return "Poder";}}
inline unsigned powerLevel(Action a){return a==Action::ActionSurge||a==Action::DivineSmite?2:a==Action::ScorchingRay||a==Action::SacredWeapon||a==Action::TurnUndead?3:a==Action::Fireball?5:1;}
inline unsigned powerCost(Action a){return a==Action::ScorchingRay?5:a==Action::Fireball?7:a==Action::MagicMissile||a==Action::BurningHands||a==Action::ShieldSpell||a==Action::DivineSmite?3:0;}
inline bool undead(unsigned id){return id==2||id==5||id==8||id==9;}
inline unsigned layRemaining(const Game& g){return g.dndProgression&&g.p.cls==1?5u*g.p.level-g.laySpent:0;}
inline void refreshPowers(Game& g){g.laySpent=0;g.channelSpent=false;g.sacredTurns=g.turnedTurns=0;g.windSpent=g.surgeSpent=g.rageSpent=0;clearMartialCombat(g);}
inline const char* swearDevotion(Game& g){if(!g.dndProgression||g.p.cls!=1)return "Somente novo Paladino";if(g.p.level<3)return "Juramento no nivel 3";if(g.oath)return "Juramento ja firmado";if(g.phase!=Phase::Home||g.tripStage||g.campStage||g.dungeonFlags||g.clubStage==1||g.clubStage==2)return "Firme na cidade ou abrigo";g.oath=1;return nullptr;}
inline const char* powerError(const Game& g,Action a){
 if(!powerAction(a))return "Poder invalido";if(!g.dndProgression)return "Heroi usa regras antigas";
 unsigned cls=a==Action::RagePower?3:a==Action::SecondWind||a==Action::ActionSurge?2:uint8_t(a)>=uint8_t(Action::LayHands)?1:0;if(g.p.cls!=cls)return "Poder de outra classe";
 if(g.p.level<powerLevel(a))return "Nivel insuficiente";
 if(g.phase!=Phase::Hero&&!((a==Action::LayHands||a==Action::SecondWind)&&g.phase==Phase::Home&&!g.tripStage&&!g.campStage&&g.clubStage!=1&&g.clubStage!=2))return "Use no seu turno";
 if(g.p.mp<powerCost(a))return "Mana insuficiente";
 if(a==Action::SecondWind){if(g.windSpent)return "Folego esgotado: descanse";if(g.p.hp>=g.p.maxhp)return "HP ja esta cheio";}
 if(a==Action::ActionSurge){if(g.surgeSpent>=surgeUses(g))return "Surto esgotado: descanse";if(g.surgeTurnUsed)return "Um surto por turno";}
 if(a==Action::RagePower){if(g.rageTurns)return "Furia ja esta ativa";if(g.p.level<20&&g.rageSpent>=rageUses(g))return "Furias esgotadas: descanse";}
 if(a==Action::LayHands){if(!layRemaining(g))return "Cura esgotada: descanse";if(g.p.hp>=g.p.maxhp)return "HP ja esta cheio";}
 if(a==Action::SacredWeapon||a==Action::TurnUndead){if(!g.oath)return "Firme o juramento antes";if(g.channelSpent)return "Canalizar esgotado: descanse";}
 if(a==Action::TurnUndead&&!undead(g.enemyId))return "Alvo nao e morto-vivo";return nullptr;
}
inline const char* act(Game& g,Action a);
inline const char* usePower(Game& g,Action a){
 if(auto err=powerError(g,a))return err;
 if(a==Action::DivineSmite)return act(g,a);
 g.p.mp-=powerCost(a);g.damage=0;g.crit=g.dodge=false;
 if(a==Action::SecondWind){g.damage=std::min<unsigned>(g.p.maxhp-g.p.hp,1+random(g)%10+g.p.level);g.p.hp+=g.damage;g.windSpent=1;return nullptr;}
 if(a==Action::ActionSurge){++g.surgeSpent;g.surgePending=g.surgeTurnUsed=true;return nullptr;}
 if(a==Action::RagePower){if(g.p.level<20)++g.rageSpent;g.rageTurns=3;return nullptr;}
 if(a==Action::LayHands){g.damage=std::min<unsigned>(layRemaining(g),g.p.maxhp-g.p.hp);g.p.hp+=g.damage;g.laySpent+=g.damage;if(g.phase==Phase::Hero)g.phase=Phase::Enemy;return nullptr;}
 if(a==Action::ShieldSpell){g.guard=75;g.phase=Phase::Enemy;return nullptr;}
 if(a==Action::SacredWeapon){g.channelSpent=true;g.sacredTurns=3;g.phase=Phase::Enemy;return nullptr;}
 if(a==Action::TurnUndead){g.channelSpent=true;g.turnedTurns=2;g.phase=Phase::Enemy;return nullptr;}
 unsigned damage=0;
 if(a==Action::MagicMissile){for(unsigned i=0;i<3;++i)damage+=2+random(g)%4;}
 else if(a==Action::ScorchingRay){for(unsigned i=0;i<3;++i){unsigned roll=random(g)%20+1;if(roll!=1&&(roll==20||int(roll)+abilityMod(g.attributes[3])+int(proficiency(g.p.level))>=10+enemySpec(g.enemyId).def/3))damage+=2+random(g)%6+random(g)%6;}}
 else {for(unsigned i=0;i<(a==Action::Fireball?8u:3u);++i)damage+=1+random(g)%6;if(int(random(g)%20+1)+2>=8+int(proficiency(g.p.level))+abilityMod(g.attributes[3]))damage/=2;}
 g.damage=std::min<unsigned>(g.enemyHp,damage);g.enemyHp-=g.damage;g.phase=Phase::Enemy;return nullptr;
}
// Percentage combat adaptation of the Champion milestones in SRD 5.1.
inline unsigned weaponCriticalChance(const Game& g){return g.dndProgression&&g.p.cls==2&&g.p.level>=3?(g.p.level>=15?30:20):12;}
inline unsigned survivorRecovery(const Game& g){return g.dndProgression&&g.p.cls==2&&g.p.level>=18&&g.phase==Phase::Hero&&g.p.hp&&unsigned(g.p.hp)*2<=g.p.maxhp?std::min<unsigned>(g.p.maxhp-g.p.hp,std::max(0,5+abilityMod(g.attributes[2]))):0;}
inline void startHeroTurn(Game& g){g.p.hp+=survivorRecovery(g);}
inline int rollDamage(Game& g,int atk,int def,unsigned criticalChance=12){int d=std::max(1,atk+int(random(g)%3)-def/3);g.crit=random(g)%100<criticalChance;return g.crit?d+(d>>1):d;}
inline void clearFeedback(Game& g){g.damage=0;g.crit=g.dodge=false;}
inline unsigned encounterHp(const Game& g,uint8_t id){return g.dndProgression&&g.city==0&&g.p.level<3&&(id<2||id==14)?(g.p.level==1?6:10):enemySpec(id).hp;}
inline unsigned encounterAttack(const Game& g){return g.dndProgression&&g.city==0&&g.p.level<3&&(g.enemyId<2||g.enemyId==14)?std::min<unsigned>(g.p.level+1,enemySpec(g.enemyId).atk):enemySpec(g.enemyId).atk;}
inline bool begin(Game& g,uint8_t id=2){if(g.phase!=Phase::Home||!g.p.hp||id>17||(id>=10&&id<14&&g.eventStage!=2)||(id>=14&&(g.discovery!=4||id!=14+g.city))||(id==3&&g.ruinsWins<3))return false;g.enemyId=id;g.phase=Phase::Hero;g.enemyHp=encounterHp(g,id);g.guard=0;g.enemyBeat=0;g.sacredTurns=g.turnedTurns=0;clearMartialCombat(g);g.gainXp=0;g.gainGold=0;g.dropLife=g.dropMana=false;clearFeedback(g);startHeroTurn(g);return true;}
inline bool explore(Game& g,bool boss=false){if(g.phase!=Phase::Home||g.campStage||g.tripStage||g.dungeonFlags||g.discovery||!g.p.hp||(boss&&(g.city!=1||g.ruinsWins<3)))return false;unsigned n=random(g);uint8_t id=g.city==0?(g.dndProgression&&g.p.level<3?(n%2?0:1):(n%2?4:1)):g.city==1?(n%2?5:2):g.city==2?6:7;return begin(g,boss?3:id);}
inline const char* prepareTrip(Game& g,uint8_t dest){if(g.phase!=Phase::Home||g.campStage||g.tripStage||g.dungeonFlags||g.clubStage==1||g.clubStage==2)return "Termine a acao atual";if(dest>3||dest==g.city)return "Destino invalido";g.tripStage=1;g.tripTo=dest;g.tripSurvival=survival(g);g.tripLuck=luck(g);if(g.rations){--g.rations;g.tripSurvival+=2;}if(g.charts){--g.charts;++g.tripSurvival;++g.tripLuck;}if(g.charms){--g.charms;g.tripLuck+=2;}g.tripRoll=1+random(g)%20;g.tripTotal=g.tripRoll+g.tripSurvival+g.tripLuck;g.tripDifficulty=routeDifficulty(g.city,dest);unsigned danger=std::max(cityLevel(g.city),cityLevel(dest));g.tripEnemy=danger>=18?7:danger>=10?6:danger>=5?5:4;return nullptr;}
inline bool acceptTrip(Game& g){if(g.tripStage!=1||g.phase!=Phase::Home)return false;if(tripSafe(g)){g.tripStage=3;return true;}if(!begin(g,g.tripEnemy))return false;g.tripStage=2;return true;}
inline bool arriveTrip(Game& g){if(g.tripStage!=3||g.phase!=Phase::Home)return false;g.city=g.tripTo;clearTrip(g);return true;}
inline void finish(Game& g){
  if(g.phase!=Phase::Hero&&g.phase!=Phase::Enemy)return;
  if(!g.enemyHp){
    g.phase=Phase::Won;const auto& e=enemySpec(g.enemyId);g.gainXp=uint16_t(e.xp)*(g.dndProgression?10:1);g.gainGold=uint8_t(std::min<uint32_t>(e.gold,999999u-g.p.gold));g.p.gold+=g.gainGold;
    g.p.xp=g.p.xp>UINT32_MAX-g.gainXp?UINT32_MAX:g.p.xp+g.gainXp;
    if(g.eventStage!=2&&!g.tripStage&&!g.dungeonFlags&&g.city==1){if(g.enemyId==3){g.guardianDefeated=true;if(g.crystals<9)++g.crystals;}else if(g.ruinsWins<3)++g.ruinsWins;}
    if(g.eventStage!=2&&g.p.life<99 && (g.enemyId==3||random(g)%100<50)){++g.p.life;g.dropLife=true;}
    if(g.eventStage!=2&&g.p.mana<99 && (g.enemyId==3||random(g)%100<25)){++g.p.mana;g.dropMana=true;}
    if(g.eventStage!=2&&!g.tripStage&&!g.dungeonFlags&&g.city==1&&g.questId&&g.questProgress<contract(g.questId).count&&(contract(g.questId).enemy<0||contract(g.questId).enemy==g.enemyId))++g.questProgress;
    levelUp(g);clearMartialCombat(g);
  }else if(!g.p.hp){g.phase=Phase::Lost;g.gainXp=uint16_t(std::min(g.p.xp,uint32_t(xpNeeded(g)/20)));g.p.xp-=g.gainXp;clearMartialCombat(g);}
}
// Return nullptr on accepted action; rejected actions do not consume a turn.
inline const char* act(Game& g,Action a){
  if(g.phase!=Phase::Hero)return "Aguarde seu turno";
  if(uint8_t(a)>uint8_t(Action::DivineSmite))return "Acao invalida";
  if(g.dndProgression&&g.p.cls==0&&a==Action::Offensive)a=Action::MagicMissile;
  if(g.dndProgression&&g.p.cls==0&&a==Action::Defensive)a=Action::ShieldSpell;
  if(a==Action::DivineSmite){if(auto err=powerError(g,a))return err;}
  if(powerAction(a)&&a!=Action::DivineSmite){auto err=usePower(g,a);if(!err)finish(g);return err;}
  if(a==Action::Offensive||a==Action::Defensive){
    if(g.dndProgression&&g.p.cls==1&&a==Action::Offensive&&g.p.level<2)return "Desbloqueia no nivel 2";
    uint8_t cost=skillCost(g.p.cls,a==Action::Defensive);if(g.p.mp<cost)return "Mana insuficiente";g.p.mp-=cost;
  }
  if(a==Action::Life||a==Action::Mana){
    bool m=a==Action::Mana;auto &v=m?g.p.mp:g.p.hp;auto &count=m?g.p.mana:g.p.life;
    if(!count)return "Sem pocoes";uint16_t heal=recovery(v,m?g.p.maxmp:g.p.maxhp,m);if(!heal)return "Ja esta cheio";
    v+=heal;--count;clearFeedback(g);g.damage=heal;endHeroAction(g);return nullptr;
  }
  clearFeedback(g);
  if(a==Action::Flee){g.phase=random(g)%100<65?Phase::Fled:Phase::Enemy;if(g.phase==Phase::Fled)clearMartialCombat(g);else if(g.surgePending){g.surgePending=false;g.phase=Phase::Hero;}return nullptr;}
  if(a==Action::Defensive){
    g.guard=g.p.cls<2?75:50;
    if(g.p.cls==3)g.p.hp=std::min<uint16_t>(g.p.maxhp,g.p.hp+std::max<uint16_t>(1,g.p.maxhp/4));
    endHeroAction(g);return nullptr;
  }
  bool skill=a==Action::Offensive;int damage=rollDamage(g,effectiveAttack(g),skill&&g.p.cls==0?0:enemySpec(g.enemyId).def,weaponCriticalChance(g));
  if(g.dndProgression&&!skill)damage*=attacksPerAction(g);
  if(g.dndProgression&&g.p.cls==3&&g.crit&&g.p.level>=9)damage+=g.p.level>=17?3:g.p.level>=13?2:1;
  if(skill)damage=g.p.cls==0?damage*180/100:g.p.cls==3?damage*2:damage*150/100;
  if(g.rageTurns)damage+=rageBonus(g)*(skill?1:attacksPerAction(g));
  if(g.sacredTurns){damage+=std::max(1,abilityMod(g.attributes[5]));--g.sacredTurns;}
  g.dodge=!(skill&&g.p.cls==2)&&random(g)%100<6;
  if(!g.dodge){
    // SRD 5.1 adaptation: select before the attack; charge mana only on a hit.
    // Extra Attack is aggregated by the existing engine. Smite applies once;
    // Improved Divine Smite applies to each melee hit in that action.
    if(g.dndProgression&&g.p.cls==1){
      unsigned dice=g.p.level>=11?(skill?1:attacksPerAction(g)):0;
      if(a==Action::DivineSmite){g.p.mp-=powerCost(a);dice+=2+unsigned(undead(g.enemyId));}
      if(g.crit)dice*=2;
      for(unsigned i=0;i<dice;++i)damage+=1+random(g)%8;
    }
    g.damage=uint16_t(std::min<int>(g.enemyHp,damage));g.enemyHp-=g.damage;
  }
  endHeroAction(g);finish(g);return nullptr;
}
enum class Intent:uint8_t {Strike,Prepare,Heavy,Mend,Drain};
inline Intent enemyIntent(const Game& g){
 if(!g.dndProgression||g.turnedTurns)return Intent::Strike;
 if(g.enemyId==2||g.enemyId==9)return g.enemyBeat==1?Intent::Mend:Intent::Strike;
 if(g.enemyId==5)return g.enemyBeat==2?Intent::Drain:Intent::Strike;
 if(g.enemyId==1||g.enemyId==3||g.enemyId==4||g.enemyId==7||g.enemyId==8||g.enemyId>=10)return g.enemyBeat==1?Intent::Prepare:g.enemyBeat==2?Intent::Heavy:Intent::Strike;
 return Intent::Strike;
}
inline const char* intentName(const Game& g){if(g.turnedTurns)return "Expulso: sem ataque";switch(enemyIntent(g)){case Intent::Prepare:return "Preparar golpe / sem dano";case Intent::Heavy:return "Golpe forte / defenda!";case Intent::Mend:return "Recompor: cura ate 4 HP";case Intent::Drain:return "Drenar: HP e ate 2 MP";default:return physicalEnemy(g.enemyId)?"Ataque fisico":"Ataque magico";}}
inline bool powersSpent(const Game& g){return g.laySpent||g.channelSpent||g.windSpent||g.surgeSpent||g.rageSpent;}
// Exact D20 safety probability for the current supplies, with natural1/20.
inline unsigned tripSafety(const Game& g,unsigned dest){unsigned bonus=survival(g)+luck(g)+(g.rations?2:0)+(g.charts?2:0)+(g.charms?2:0),n=0;for(unsigned die=1;die<=20;++die)n+=die==20||(die!=1&&die+bonus>=routeDifficulty(g.city,dest));return n*5;}
inline const char* tripError(const Game& g,unsigned dest){if(g.phase!=Phase::Home||g.campStage||g.tripStage||g.dungeonFlags||g.clubStage==1||g.clubStage==2)return "Termine a acao atual";if(dest>3||dest==g.city)return "Destino invalido";return nullptr;}
// Read-only preview of wearing an item, independent of ownership/purchase.
inline Game gearPreview(const Game& g,unsigned id){auto next=g;if(gearAllowed(id,g.p.cls,g.p.level)){next.equipped[gearSlot(id)]=id;next.p.maxmp=totalMana(next);next.p.mp=std::min(next.p.mp,next.p.maxmp);}return next;}
// Public upper bound includes heavy attacks and does not advance RNG.
inline unsigned incomingCeiling(const Game& g,unsigned guard=0){
 if(g.turnedTurns||enemyIntent(g)==Intent::Prepare||enemyIntent(g)==Intent::Mend)return 0;
 int damage=std::max(1,int(encounterAttack(g))+2-effectiveDefense(g)/3);
 damage+=damage/2;if(enemyIntent(g)==Intent::Heavy)damage=damage*3/2;damage=(damage*(100-std::min(75u,guard))+99)/100;if(g.rageTurns&&physicalEnemy(g.enemyId))damage/=2;return damage;
}
inline bool enemy(Game& g){
 if(g.phase!=Phase::Enemy)return false;clearFeedback(g);g.surgeTurnUsed=false;
 if(g.turnedTurns){--g.turnedTurns;g.guard=0;g.phase=Phase::Hero;startHeroTurn(g);return true;}
 auto intent=enemyIntent(g);if(g.dndProgression)g.enemyBeat=(g.enemyBeat+1)%3;
 if(intent==Intent::Prepare||intent==Intent::Mend){if(intent==Intent::Mend)g.enemyHp=std::min<unsigned>(encounterHp(g,g.enemyId),g.enemyHp+4);g.guard=0;if(g.rageTurns)--g.rageTurns;g.phase=Phase::Hero;startHeroTurn(g);return true;}
 int damage=rollDamage(g,encounterAttack(g),effectiveDefense(g));if(intent==Intent::Heavy)damage=damage*3/2;
 unsigned guard=g.guard;if(g.guard){damage=(damage*(100-g.guard)+99)/100;g.guard=0;}
 if(g.rageTurns){if(physicalEnemy(g.enemyId))damage/=2;--g.rageTurns;}
 g.dodge=random(g)%100<6;
 if(!g.dodge){g.damage=uint16_t(std::min<int>(g.p.hp,damage));g.p.hp-=g.damage;if(intent==Intent::Drain&&guard<75)g.p.mp-=std::min<unsigned>(2,g.p.mp);}
 g.phase=Phase::Hero;finish(g);if(g.phase==Phase::Hero)startHeroTurn(g);return true;
}
inline bool home(Game& g){if(g.phase!=Phase::Won&&g.phase!=Phase::Lost&&g.phase!=Phase::Fled)return false;bool lost=g.phase==Phase::Lost;g.phase=Phase::Home;g.enemyBeat=0;g.sacredTurns=g.turnedTurns=0;clearMartialCombat(g);if(!g.p.hp)g.p.hp=1;if(g.discovery==4){g.discovery=g.discoverLoot=g.discoverAmount=0;g.chestLock=g.chestTries=g.chestRoll=0;g.chestPick=g.chestTrap=false;}if(g.tripStage==2){if(lost)clearTrip(g);else g.tripStage=3;}return true;}
inline bool rest(Game& g){if(g.phase!=Phase::Home)return false;g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;refreshPowers(g);return true;}
// Economy.cpp shopLong / UI.cpp invLong: prices and limits from Heltec.
inline uint8_t potionPrice(bool mana){return mana?12:10;}
inline uint8_t potionPrice(const Game& g,bool mana){return priced(potionPrice(mana),g.city==0?100:g.city==1?170:g.city==2?(mana?85:120):140);}
inline const char* buyPotion(Game& g,bool mana){
  if(g.phase!=Phase::Home)return "Volte para a vila";
  uint8_t& count=mana?g.p.mana:g.p.life;uint8_t price=potionPrice(g,mana);
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
