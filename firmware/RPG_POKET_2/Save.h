#pragma once
#include "Rules.h"
#include <string.h>
namespace rpg {
constexpr unsigned SAVE_SIZE=96;
inline void put16(uint8_t* b,unsigned o,uint16_t v){b[o]=uint8_t(v);b[o+1]=uint8_t(v>>8);}
inline void put32(uint8_t* b,unsigned o,uint32_t v){put16(b,o,uint16_t(v));put16(b,o+2,uint16_t(v>>16));}
inline uint16_t get16(const uint8_t* b,unsigned o){return b[o]|uint16_t(b[o+1])<<8;}
inline uint32_t get32(const uint8_t* b,unsigned o){return get16(b,o)|uint32_t(get16(b,o+2))<<16;}
inline uint32_t crc(const uint8_t* b,unsigned n){uint32_t c=~0u;for(unsigned i=0;i<n;++i){c^=b[i];for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return ~c;}
inline bool valid(const Game& g){
  auto &p=g.p;
  if(g.clubStage&&(!g.guildMember||g.p.level<5))return false;
  if(g.race>3||g.shirt>7||g.trousers>7||g.clubStage>3||(g.clubStage&&!g.clubSession)||g.city>3||g.questId>3)return false;
  if(!g.questId){if(g.questProgress||g.questLevel)return false;}
  else if(g.questLevel<3||g.questLevel>p.level||g.questProgress>contract(g.questId).count)return false;
  if(g.owned>>GEAR_COUNT)return false;
  for(uint8_t slot=0;slot<3;++slot){uint8_t id=g.equipped[slot];
    if(g.forge[slot]>3||(g.forge[slot]&&p.level<forgeRequirement(g.forge[slot]-1)))return false;
    if(id&&(!gearOwns(g.owned,id)||gearSlot(id)!=slot||!gearAllowed(id,p.cls,p.level)))return false;
  }
  if(p.cls>3||p.level<3||p.level>99||!p.maxhp||p.hp>p.maxhp||p.maxmp!=totalMana(g)||p.mp>p.maxmp||!p.atk||p.life>99||p.mana>99||p.gold>999999||!g.randomState)return false;
  if(uint8_t(g.phase)>5||g.enemyId>3||g.ruinsWins>3||(g.guardianDefeated&&g.ruinsWins<3)||(g.enemyId==3&&g.ruinsWins<3)||g.enemyHp>enemySpec(g.enemyId).hp||(g.guard!=0&&g.guard!=50&&g.guard!=75)||g.gainGold>enemySpec(g.enemyId).gold||g.gainXp>3276)return false;
  if((g.phase==Phase::Hero||g.phase==Phase::Enemy)&&(!p.hp||!g.enemyHp))return false;
  if(g.phase==Phase::Won&&(g.enemyHp||!p.hp))return false;
  if(g.phase==Phase::Lost&&(p.hp||!g.enemyHp))return false;
  if((g.phase==Phase::Home||g.phase==Phase::Fled)&&!p.hp)return false;
  return true;
}
inline void encode(const Game& g,uint32_t seq,uint8_t* b){
  memset(b,0,SAVE_SIZE);memcpy(b,"PKT2",4);put16(b,4,7);put16(b,6,SAVE_SIZE);put32(b,8,seq);
  b[12]=g.p.cls;b[13]=g.p.level;b[14]=g.p.atk;b[15]=g.p.def;b[16]=g.p.life;b[17]=g.p.mana;
  put16(b,18,g.p.hp);put16(b,20,g.p.maxhp);put16(b,22,g.p.mp);put16(b,24,g.p.maxmp);
  put32(b,26,g.p.xp);put32(b,30,g.p.gold);b[34]=uint8_t(g.phase);b[35]=g.guard;put16(b,36,g.enemyHp);put32(b,38,g.randomState);
  put16(b,42,g.gainXp);put16(b,44,g.damage);b[46]=g.gainGold;
  b[47]=g.crit|g.dodge<<1|g.dropLife<<2|g.dropMana<<3|g.tutorial<<4;b[48]=g.enemyId;b[49]=g.ruinsWins;b[50]=g.guardianDefeated;put32(b,51,g.owned|uint32_t(g.questId)<<18|uint32_t(g.questProgress)<<20|uint32_t(g.city)<<22);memcpy(b+55,g.equipped,3);b[58]=g.forge[0]|g.forge[1]<<2|g.forge[2]<<4;b[59]=g.questLevel;b[64]=g.race;b[65]=g.shirt;b[66]=g.trousers;b[67]=g.guildMember;b[68]=g.clubStage;put32(b,72,g.clubSession);put32(b,92,crc(b,92));
}
enum class Decode{Ok,Corrupt,Unsupported};
inline Decode decode(const uint8_t* b,Game& g,uint32_t& seq){
  if(memcmp(b,"PKT2",4))return Decode::Corrupt;
  if((get16(b,4)<1||get16(b,4)>7)||get16(b,6)!=(get16(b,4)>=7?SAVE_SIZE:64))return Decode::Unsupported;
  if(get32(b,get16(b,4)>=7?92:60)!=crc(b,get16(b,4)>=7?92:60)||b[47]>31)return Decode::Corrupt;
  Game t;seq=get32(b,8);t.p.cls=b[12];t.p.level=b[13];t.p.atk=b[14];t.p.def=b[15];t.p.life=b[16];t.p.mana=b[17];
  t.p.hp=get16(b,18);t.p.maxhp=get16(b,20);t.p.mp=get16(b,22);t.p.maxmp=get16(b,24);t.p.xp=get32(b,26);t.p.gold=get32(b,30);
  t.phase=Phase(b[34]);t.guard=b[35];t.enemyHp=get16(b,36);t.randomState=get32(b,38);t.gainXp=get16(b,42);t.damage=get16(b,44);t.gainGold=b[46];
  t.crit=b[47]&1;t.dodge=b[47]&2;t.dropLife=b[47]&4;t.dropMana=b[47]&8;t.tutorial=b[47]&16;
  // Version 1 had only Skeleton and no region progress. Preserve every old field.
  if(get16(b,4)>=2){if(b[50]>1)return Decode::Corrupt;t.enemyId=b[48];t.ruinsWins=b[49];t.guardianDefeated=b[50];}
  if(get16(b,4)>=3){t.owned=get32(b,51);memcpy(t.equipped,b+55,3);}
  if(get16(b,4)>=4){if(b[58]>63)return Decode::Corrupt;t.forge[0]=b[58]&3;t.forge[1]=(b[58]>>2)&3;t.forge[2]=(b[58]>>4)&3;}
  if(get16(b,4)>=5){uint32_t packed=get32(b,51);if(packed>>(get16(b,4)>=6?24:22))return Decode::Corrupt;t.owned=packed&((1u<<18)-1);t.questId=(packed>>18)&3;t.questProgress=(packed>>20)&3;t.questLevel=b[59];if(get16(b,4)>=6)t.city=(packed>>22)&3;}
  if(get16(b,4)>=7){if(b[67]>1)return Decode::Corrupt;t.race=b[64];t.shirt=b[65];t.trousers=b[66];t.guildMember=b[67];t.clubStage=b[68];t.clubSession=get32(b,72);for(unsigned i=69;i<92;++i)if((i<72||i>75)&&b[i])return Decode::Corrupt;}
  if(!valid(t))return Decode::Corrupt;g=t;return Decode::Ok;
}
enum class Read{Missing,Ok,Error};
enum class Load{Empty,Ok,Recovered,Blocked};
// Backend reads legacy 64-byte or new 96-byte records into SAVE_SIZE buffers; writes 96 bytes.
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
