#pragma once
#include "Dungeon.h"
#include "Camp.h"
#include "GuildEvents.h"
#include "Campaign.h"
#include "Exploration.h"
#include <string.h>
namespace rpg {
constexpr unsigned SAVE_SIZE=128;
inline void put16(uint8_t* b,unsigned o,uint16_t v){b[o]=uint8_t(v);b[o+1]=uint8_t(v>>8);}
inline void put32(uint8_t* b,unsigned o,uint32_t v){put16(b,o,uint16_t(v));put16(b,o+2,uint16_t(v>>16));}
inline uint16_t get16(const uint8_t* b,unsigned o){return b[o]|uint16_t(b[o+1])<<8;}
inline uint32_t get32(const uint8_t* b,unsigned o){return get16(b,o)|uint32_t(get16(b,o+2))<<16;}
inline uint32_t crc(const uint8_t* b,unsigned n){uint32_t c=~0u;for(unsigned i=0;i<n;++i){c^=b[i];for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return ~c;}
inline bool valid(const Game& g){
  if(g.seenEnemies>>29)return false;
  if(!discoveryValid(g))return false;
  if(g.originPage>8||(!g.originStory&&g.originPage))return false;
  auto &p=g.p;if(g.enemyBeat>2||(!g.dndProgression&&g.enemyBeat)||(g.phase==Phase::Home&&g.enemyBeat))return false;
    if(g.windSpent>1||g.surgeSpent>surgeUses(g)||g.rageTurns>3||(p.level<20&&g.rageSpent>rageUses(g))||(p.level>=20&&g.rageSpent))return false;
    if(!g.dndProgression||p.cls!=2){if(g.windSpent||g.surgeSpent||g.surgePending||g.surgeTurnUsed)return false;}
    if(!g.dndProgression||p.cls!=3){if(g.rageSpent||g.rageTurns)return false;}
    if(g.surgePending&&(!g.surgeTurnUsed||!g.surgeSpent||g.phase!=Phase::Hero))return false;
    if(g.surgeTurnUsed&&(!g.surgeSpent||(g.phase!=Phase::Hero&&g.phase!=Phase::Enemy)))return false;
    if(g.rageTurns&&((p.level<20&&!g.rageSpent)||(g.phase!=Phase::Hero&&g.phase!=Phase::Enemy)))return false;
  if(g.oath>1||g.sacredTurns>3||g.turnedTurns>2||(g.sacredTurns&&g.turnedTurns))return false;
  if(!g.dndProgression||p.cls!=1){if(g.oath||g.laySpent||g.channelSpent||g.sacredTurns||g.turnedTurns)return false;}
  else {if(g.laySpent>5u*p.level||(g.oath&&p.level<3)||((g.channelSpent||g.sacredTurns||g.turnedTurns)&&!g.oath))return false;}
  if((g.sacredTurns||g.turnedTurns)&&(g.phase==Phase::Home||!g.channelSpent))return false;
  if(g.turnedTurns&&!undead(g.enemyId))return false;
  if(g.dndProgression){if(g.p.level>20||g.advancementSpent>improvementCount(g.p.cls,g.p.level)*2||(g.tough&&g.advancementSpent<2))return false;for(auto score:g.attributes)if(score<8||score>20)return false;}
  else if(g.tough||g.advancementSpent)return false;
  if(!campaignValid(g)||!eventValid(g)||!dungeonValid(g)||!campValid(g))return false;
  if(g.rations>9||g.charts>9||g.charms>9||g.tripStage>3)return false;
  if(!g.tripStage){if(g.tripTo||g.tripRoll||g.tripDifficulty||g.tripTotal||g.tripEnemy||g.tripSurvival||g.tripLuck)return false;}
  else {if(g.tripTo>3||g.tripTo==g.city||g.tripRoll<1||g.tripRoll>20||g.tripEnemy<4||g.tripEnemy>7||g.tripSurvival>13||g.tripLuck>11||g.tripTotal!=g.tripRoll+g.tripSurvival+g.tripLuck||g.tripDifficulty!=routeDifficulty(g.city,g.tripTo)||g.clubStage==1||g.clubStage==2)return false;
    if(g.tripStage==2){if(g.phase==Phase::Home||tripSafe(g)||g.enemyId!=g.tripEnemy)return false;}else if(g.phase!=Phase::Home)return false;}

  if(g.clubStage&&(!g.guildMember||g.p.level<5))return false;
  if(g.race>3||g.shirt>7||g.trousers>7||g.clubStage>3||(g.clubStage&&!g.clubSession)||g.city>3||g.questId>15)return false;
  if(!g.questId){if(g.questProgress||g.questLevel)return false;}
  else if(g.questLevel<1||g.questLevel>p.level||g.questProgress>contract(g.questId).count)return false;
  if(g.owned>>GEAR_COUNT)return false;
  for(uint8_t slot=0;slot<3;++slot){uint8_t id=g.equipped[slot];
    if(g.forge[slot]>3||(g.forge[slot]&&p.level<forgeRequirement(g.forge[slot]-1)))return false;
    if(id&&(!gearOwns(g.owned,id)||gearSlot(id)!=slot||!gearAllowed(id,p.cls,p.level)))return false;
  }
  if(p.cls>3||p.level<1||p.level>99||!p.maxhp||p.hp>p.maxhp||p.maxmp!=totalMana(g)||p.mp>p.maxmp||!p.atk||p.life>99||p.mana>99||p.gold>999999||!g.randomState)return false;
  if(uint8_t(g.phase)>5||g.enemyId>28||g.ruinsWins>3||(g.guardianDefeated&&g.ruinsWins<3)||(g.enemyId==3&&g.ruinsWins<3)||g.enemyHp>enemySpec(g.enemyId).hp||(g.guard!=0&&g.guard!=50&&g.guard!=75)||g.gainGold>enemySpec(g.enemyId).gold||g.gainXp>3276)return false;
  if((g.phase==Phase::Hero||g.phase==Phase::Enemy)&&(!p.hp||!g.enemyHp))return false;
  if(g.phase==Phase::Won&&(g.enemyHp||!p.hp))return false;
  if(g.phase==Phase::Lost&&(p.hp||!g.enemyHp))return false;
  if((g.phase==Phase::Home||g.phase==Phase::Fled)&&!p.hp)return false;
  return true;
}
inline void encode(const Game& g,uint32_t seq,uint8_t* b){
  memset(b,0,SAVE_SIZE);memcpy(b,"PKT2",4);put16(b,4,25);put16(b,6,SAVE_SIZE);put32(b,8,seq);
  b[12]=g.p.cls;b[13]=g.p.level;b[14]=g.p.atk;b[15]=g.p.def;b[16]=g.p.life;b[17]=g.p.mana;
  put16(b,18,g.p.hp);put16(b,20,g.p.maxhp);put16(b,22,g.p.mp);put16(b,24,g.p.maxmp);
  put32(b,26,g.p.xp);put32(b,30,g.p.gold);b[34]=uint8_t(g.phase);b[35]=g.guard;put16(b,36,g.enemyHp);put32(b,38,g.randomState);
  put16(b,42,g.gainXp);put16(b,44,g.damage);b[46]=g.gainGold;
  b[47]=g.crit|g.dodge<<1|g.dropLife<<2|g.dropMana<<3|g.tutorial<<4;b[48]=g.enemyId;b[49]=g.ruinsWins;b[50]=g.guardianDefeated;put32(b,51,g.owned|uint32_t(g.questId&3)<<18|uint32_t(g.questProgress)<<20|uint32_t(g.city)<<22);memcpy(b+55,g.equipped,3);b[58]=g.forge[0]|g.forge[1]<<2|g.forge[2]<<4;b[59]=g.questLevel;b[64]=g.race;b[65]=g.shirt;b[66]=g.trousers;b[67]=g.guildMember|((g.seenEnemies>>20)&127)<<1;b[68]=g.clubStage;put32(b,72,g.clubSession);b[76]=g.tripStage;b[77]=g.tripTo;b[78]=g.tripRoll;b[79]=g.tripDifficulty;b[80]=g.tripTotal;b[81]=g.tripEnemy;b[82]=g.tripSurvival;b[83]=g.tripLuck;b[84]=g.rations;b[85]=g.charts;b[86]=g.charms;b[69]=g.crystals;b[70]=g.dungeonFlags;b[71]=g.dungeonXY;b[87]=g.dungeonLoot;b[88]=g.dungeonEnemies;b[89]=g.dungeonClears;b[90]=g.campStage|(g.campRation<<2)|(g.campKit<<3)|(g.sleepKit<<4);b[91]=g.campRoll;put32(b,92,g.eventDay);b[96]=g.eventStage;b[97]=g.eventTier;b[98]=g.eventOriginCity;b[99]=g.eventOriginPage;b[100]=g.dndProgression;b[101]=g.tough;b[102]=g.advancementSpent;if(g.dndProgression)memcpy(b+103,g.attributes,6);b[109]=g.oath;b[110]=g.laySpent;b[111]=g.channelSpent;b[112]=g.sacredTurns;b[113]=g.turnedTurns;b[114]=g.windSpent;b[115]=g.surgeSpent;b[116]=g.surgePending|(g.surgeTurnUsed<<1)|((g.questId>>2)<<2);b[117]=g.rageSpent;b[118]=g.rageTurns;b[119]=g.enemyBeat|(g.islandTraps<<2)|(g.islandCleared<<5)|(((g.seenEnemies>>27)&3)<<6);b[120]=g.campaignFlags;b[121]=g.campaignStage|(g.campaignEnding<<4);b[60]=g.discovery|(g.chestLock<<3)|(g.chestTrap<<5)|(g.chestTries<<6);b[61]=g.discoverLoot|(g.chestRoll<<3);b[62]=g.discoverAmount|(g.chestPick<<7);b[63]=g.scrap|(g.gazuas<<4);b[122]=g.originStory;b[123]=g.originPage;// Save20:18 discovery bits in formerly reserved high bits, no byte growth.
  b[58]|=((g.seenEnemies>>18)&3)<<6;b[47]|=((g.seenEnemies>>8)&7)<<5;b[50]|=((g.seenEnemies>>11)&127)<<1;b[54]|=g.seenEnemies&255;put32(b,124,crc(b,124));
}
enum class Decode{Ok,Corrupt,Unsupported};
inline Decode decode(const uint8_t* b,Game& g,uint32_t& seq){
  if(memcmp(b,"PKT2",4))return Decode::Corrupt;
  if((get16(b,4)<1||get16(b,4)>25)||get16(b,6)!=(get16(b,4)>=11?SAVE_SIZE:get16(b,4)>=7?96:64))return Decode::Unsupported;
  if(get32(b,get16(b,4)>=11?124:get16(b,4)>=7?92:60)!=crc(b,get16(b,4)>=11?124:get16(b,4)>=7?92:60)||(get16(b,4)<20&&b[47]>31))return Decode::Corrupt;
  if(get16(b,4)==24&&(b[48]>24||b[67]>63||b[119]>63||b[116]>3||b[60]%8>5))return Decode::Corrupt;
  if(get16(b,4)<24&&(b[48]>19||b[70]>15||(get16(b,4)>=15&&b[119]>2)))return Decode::Corrupt;
  if(get16(b,4)>=2&&get16(b,4)<23&&b[48]>17)return Decode::Corrupt;
  if(get16(b,4)<8&&get16(b,4)>=2&&b[48]>3)return Decode::Corrupt;
  if(get16(b,4)==8&&b[48]>7)return Decode::Corrupt;
  Game t;seq=get32(b,8);t.p.cls=b[12];t.p.level=b[13];t.p.atk=b[14];t.p.def=b[15];t.p.life=b[16];t.p.mana=b[17];
  t.p.hp=get16(b,18);t.p.maxhp=get16(b,20);t.p.mp=get16(b,22);t.p.maxmp=get16(b,24);t.p.xp=get32(b,26);t.p.gold=get32(b,30);
  t.phase=Phase(b[34]);t.guard=b[35];t.enemyHp=get16(b,36);t.randomState=get32(b,38);t.gainXp=get16(b,42);t.damage=get16(b,44);t.gainGold=b[46];
  t.crit=b[47]&1;t.dodge=b[47]&2;t.dropLife=b[47]&4;t.dropMana=b[47]&8;t.tutorial=b[47]&16;
  // Version 1 had only Skeleton and no region progress. Preserve every old field.
  if(get16(b,4)>=2){if(get16(b,4)<20&&b[50]>1)return Decode::Corrupt;t.enemyId=b[48];t.ruinsWins=b[49];t.guardianDefeated=b[50]&1;}
  if(get16(b,4)>=3){t.owned=get32(b,51);memcpy(t.equipped,b+55,3);}
  if(get16(b,4)>=4){if(get16(b,4)<23&&b[58]>63)return Decode::Corrupt;t.forge[0]=b[58]&3;t.forge[1]=(b[58]>>2)&3;t.forge[2]=(b[58]>>4)&3;}
  if(get16(b,4)>=5){uint32_t packed=get32(b,51);if(get16(b,4)<20&&packed>>(get16(b,4)>=6?24:22))return Decode::Corrupt;t.owned=packed&((1u<<18)-1);t.questId=(packed>>18)&3;t.questProgress=(packed>>20)&3;t.questLevel=b[59];if(get16(b,4)>=6)t.city=(packed>>22)&3;}
  if(get16(b,4)>=7){if(b[67]>(get16(b,4)>=25?255:get16(b,4)>=24?63:1))return Decode::Corrupt;t.race=b[64];t.shirt=b[65];t.trousers=b[66];t.guildMember=b[67]&1;t.clubStage=b[68];t.clubSession=get32(b,72);for(unsigned i=69;i<92;++i)if((i<72||i>75)&&(get16(b,4)<8||i<76||i>86)&&(get16(b,4)<9||!((i>=69&&i<=71)||(i>=87&&i<=89)))&&(get16(b,4)<10||i<90)&&b[i])return Decode::Corrupt;}
  if(get16(b,4)>=8){t.tripStage=b[76];t.tripTo=b[77];t.tripRoll=b[78];t.tripDifficulty=b[79];t.tripTotal=b[80];t.tripEnemy=b[81];t.tripSurvival=b[82];t.tripLuck=b[83];t.rations=b[84];t.charts=b[85];t.charms=b[86];}
  if(get16(b,4)>=9){t.crystals=b[69];t.dungeonFlags=b[70];t.dungeonXY=b[71];t.dungeonLoot=b[87];t.dungeonEnemies=b[88];t.dungeonClears=b[89];}
  if(get16(b,4)>=10){if(b[90]>31)return Decode::Corrupt;t.campStage=b[90]&3;t.campRation=b[90]&4;t.campKit=b[90]&8;t.sleepKit=b[90]&16;t.campRoll=b[91];}
  if(get16(b,4)<18&&b[48]>13)return Decode::Corrupt;
  if(get16(b,4)>=19){t.discovery=b[60]&7;t.chestLock=(b[60]>>3)&3;t.chestTrap=b[60]&32;t.chestTries=b[60]>>6;t.discoverLoot=b[61]&7;t.chestRoll=b[61]>>3;t.discoverAmount=b[62]&127;t.chestPick=b[62]&128;t.scrap=b[63]&15;t.gazuas=b[63]>>4;}
  else if(get16(b,4)>=18){t.discovery=b[60];t.discoverLoot=b[61];t.discoverAmount=b[62];t.scrap=b[63];}
  if(get16(b,4)>=17){if(b[122]>1)return Decode::Corrupt;t.originStory=b[122];t.originPage=b[123];}
  if(get16(b,4)>=16){if(get16(b,4)<23&&(b[120]&128||b[121]>7||b[48]>17))return Decode::Corrupt;if(get16(b,4)<22&&(b[120]>7||b[121]>3))return Decode::Corrupt;if(get16(b,4)<21&&(b[120]>3||b[121]>2))return Decode::Corrupt;t.campaignFlags=b[120];t.campaignStage=b[121]&15;if(get16(b,4)>=23)t.campaignEnding=b[121]>>4;}
  if(get16(b,4)>=24){if(get16(b,4)<25&&b[119]>63)return Decode::Corrupt;t.enemyBeat=b[119]&3;t.islandTraps=(b[119]>>2)&7;t.islandCleared=b[119]&32;}else if(get16(b,4)>=15)t.enemyBeat=b[119];
    if(get16(b,4)>=14){if(b[116]>(get16(b,4)>=25?15:3))return Decode::Corrupt;if(get16(b,4)>=25)t.questId|=(b[116]>>2)<<2;t.windSpent=b[114];t.surgeSpent=b[115];t.surgePending=b[116]&1;t.surgeTurnUsed=b[116]&2;t.rageSpent=b[117];t.rageTurns=b[118];}
    if(get16(b,4)>=13){if(b[111]>1)return Decode::Corrupt;t.oath=b[109];t.laySpent=b[110];t.channelSpent=b[111];t.sacredTurns=b[112];t.turnedTurns=b[113];}
  if(get16(b,4)>=11){t.eventDay=get32(b,92);t.eventStage=b[96];t.eventTier=b[97];t.eventOriginCity=b[98];t.eventOriginPage=b[99];for(unsigned i=get16(b,4)>=17?124:get16(b,4)>=16?122:get16(b,4)>=15?120:get16(b,4)>=14?119:get16(b,4)>=13?114:get16(b,4)>=12?109:100;i<124;++i)if(b[i])return Decode::Corrupt;
    if(get16(b,4)>=12){if(b[100]>1||b[101]>1)return Decode::Corrupt;t.dndProgression=b[100];t.tough=b[101];t.advancementSpent=b[102];if(t.dndProgression)memcpy(t.attributes,b+103,6);else for(unsigned i=103;i<109;++i)if(b[i])return Decode::Corrupt;}}
  if(get16(b,4)>=20)t.seenEnemies=uint32_t(b[54])|uint32_t(b[47]>>5)<<8|uint32_t(b[50]>>1)<<11;
  else {if(t.enemyId<18&&t.phase!=Phase::Home)t.seenEnemies|=1u<<t.enemyId;if(t.guardianDefeated)t.seenEnemies|=1u<<3;}
  if(get16(b,4)>=23)t.seenEnemies|=uint32_t(b[58]>>6)<<18;  if(get16(b,4)>=24)t.seenEnemies|=uint32_t(b[67]>>1)<<20;
  if(get16(b,4)>=25)t.seenEnemies|=uint32_t(b[119]>>6)<<27;
  if(!valid(t))return Decode::Corrupt;g=t;return Decode::Ok;
}
enum class Read{Missing,Ok,Error};
enum class Load{Empty,Ok,Recovered,Blocked};
// Backend pads legacy 64/96-byte records into 128-byte buffers; writes save25/128 bytes.
// Two atomic NVS blobs with CRC and readback. No erase, format or legacy import.
template<class Backend> struct Journal {
  Backend& io;int active=-1;uint32_t seq=0;bool blocked=true;
  explicit Journal(Backend& b):io(b){}
  Load load(Game& g){
    uint8_t bytes[SAVE_SIZE];Game states[2];uint32_t s[2]={};bool ok[2]={};Read r[2];
    for(int i=0;i<2;++i){r[i]=io.read(i?"b":"a",bytes);if(r[i]==Read::Error){blocked=true;return Load::Blocked;}
      if(r[i]==Read::Ok){Decode d=decode(bytes,states[i],s[i]);if(d==Decode::Unsupported){blocked=true;return Load::Blocked;}ok[i]=d==Decode::Ok;}}
    if(!ok[0]&&!ok[1]){blocked=!(r[0]==Read::Missing&&r[1]==Read::Missing);active=-1;seq=0;return blocked?Load::Blocked:Load::Empty;}
    active=ok[1]&&(!ok[0]||int32_t(s[1]-s[0])>0)?1:0;seq=s[active];g=states[active];blocked=false;
    return (!ok[1-active]&&r[1-active]!=Read::Missing)?Load::Recovered:Load::Ok;
  }
  bool save(const Game& g){
    if(blocked||!valid(g))return false;uint8_t b[SAVE_SIZE],check[SAVE_SIZE];encode(g,seq+1,b);
    int target=active==0?1:0;const char* key=target?"b":"a";
    if(!io.write(key,b)||io.read(key,check)!=Read::Ok||memcmp(b,check,SAVE_SIZE))return false;
    active=target;++seq;return true;
  }
};
}
