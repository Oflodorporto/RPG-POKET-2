#pragma once
#include "Save.h"
#include "DailyTravel.h"
#include <initializer_list>

// Production Save30 migrates canonical Save29 records in place and reuses the frozen packed-field encoder and shared core
// invariants from Save.h, never its decoder or journal. No Save1..28 import.
namespace rpg {
constexpr unsigned PERSISTENT_SAVE_FORMAT=30;
constexpr unsigned PERSISTENT_SAVE_SIZE=160;
constexpr unsigned SLOT_LIFECYCLE_SIZE=16;
static_assert(CURRENT_SAVE_FORMAT==PERSISTENT_SAVE_FORMAT,"Update the production codec explicitly for a new format");
enum class SlotLife:uint8_t {Deleted=1,Live=2};

inline bool persistentValid(const Game& g){
  if(g.p.cls>3||g.p.level<1||g.p.level>99||uint8_t(g.phase)>5||g.enemyId>28||
     g.race>3||g.shirt>7||g.trousers>7||g.fatigue>100||(g.tripStage&&g.tripStage!=4))return false;
  if(!dailyTravelValid(g))return false;
  return validCore(g,g.tripStage!=4);
}
inline void encodePersistentVersion(const Game& g,uint32_t seq,uint8_t slot,uint8_t* b,unsigned format){
  uint8_t core[SAVE_SIZE];encode(g,seq,core);
  memset(b,0,PERSISTENT_SAVE_SIZE);memcpy(b,core,124);
  put16(b,4,format);put16(b,6,PERSISTENT_SAVE_SIZE);
  b[124]=g.fatigue;
  // Explicit byte layout, independent of struct padding and compiler ABI.
  const auto& d=g.daily;
  b[125]=uint8_t(d.phase);b[126]=d.origin;b[127]=d.totalDays;b[128]=d.day;
  b[129]=d.rationsPaid;b[130]=d.nightsDone;b[131]=d.region;b[132]=d.roll;
  b[133]=d.difficulty;b[134]=d.survivalBonus;b[135]=d.luckBonus;b[136]=d.kit;
  b[137]=d.total;b[138]=d.enemy;b[139]=d.supplies;b[140]=d.result;
  b[141]=d.policy;b[142]=d.routeVersion;
  // Save29's reserved bytes are assigned ONLY under the new format number.
  // The complete identity, entry cost and masks remain in the same CRC/blob.
  put32(b,144,SAVE_RESET_EPOCH);b[152]=slot;
  if(format==30){b[143]=g.dungeonGenerator;put32(b,148,g.dungeonSeed);
    b[153]=uint8_t(g.dungeonRun);b[154]=uint8_t(g.dungeonRun>>8);b[155]=uint8_t(g.dungeonRun>>16);}
  put32(b,156,crc(b,156));
}
inline void encodePersistent(const Game& g,uint32_t seq,uint8_t slot,uint8_t* b){
  encodePersistentVersion(g,seq,slot,b,PERSISTENT_SAVE_FORMAT);
}
inline Decode decodePersistent(const uint8_t* b,uint8_t slot,Game& g,uint32_t& seq){
  if(memcmp(b,"PKT2",4))return Decode::Corrupt;
  const unsigned format=get16(b,4);
  if(format!=29&&format!=30)return Decode::Unsupported;
  if(get16(b,6)!=PERSISTENT_SAVE_SIZE)return Decode::Corrupt;
  if(get32(b,144)!=SAVE_RESET_EPOCH||b[152]!=slot||slot>2)return Decode::Unsupported;
  if(get32(b,156)!=crc(b,156))return Decode::Corrupt;
  if(b[141]>DAILY_TRAVEL_POLICY||b[142]>DAILY_ROUTE_VERSION)return Decode::Unsupported;
  if(format==29){
    if(b[143])return Decode::Unsupported;
    for(unsigned i=148;i<152;++i)if(b[i])return Decode::Unsupported;
    for(unsigned i=153;i<156;++i)if(b[i])return Decode::Unsupported;
  }else if(b[143]>DUNGEON_GENERATOR_VERSION)return Decode::Unsupported;
  // Guard every array-indexing discriminator before gameplay validation.
  if(b[12]>3||b[13]<1||b[13]>99||b[34]>5||b[48]>28||b[64]>3||b[65]>7||b[66]>7||
     b[100]>1||b[101]>1)return Decode::Corrupt;
  Game t;const uint32_t s=get32(b,8);
  t.p.cls=b[12];t.p.level=b[13];t.p.atk=b[14];t.p.def=b[15];t.p.life=b[16];t.p.mana=b[17];
  t.p.hp=get16(b,18);t.p.maxhp=get16(b,20);t.p.mp=get16(b,22);t.p.maxmp=get16(b,24);
  t.p.xp=get32(b,26);t.p.gold=get32(b,30);t.phase=Phase(b[34]);t.guard=b[35];
  t.enemyHp=get16(b,36);t.randomState=get32(b,38);t.gainXp=get16(b,42);t.damage=get16(b,44);t.gainGold=b[46];
  t.crit=b[47]&1;t.dodge=b[47]&2;t.dropLife=b[47]&4;t.dropMana=b[47]&8;t.tutorial=b[47]&16;
  t.enemyId=b[48];t.ruinsWins=b[49];t.guardianDefeated=b[50]&1;
  uint32_t packed=get32(b,51);t.owned=packed&((1u<<18)-1);t.questId=(packed>>18)&3;
  t.questProgress=(packed>>20)&3;t.city=(packed>>22)&3;memcpy(t.equipped,b+55,3);
  t.forge[0]=b[58]&3;t.forge[1]=(b[58]>>2)&3;t.forge[2]=(b[58]>>4)&3;t.questLevel=b[59];
  t.discovery=b[60]&7;t.chestLock=(b[60]>>3)&3;t.chestTrap=b[60]&32;t.chestTries=b[60]>>6;
  t.discoverLoot=b[61]&7;t.chestRoll=b[61]>>3;t.discoverAmount=b[62]&127;t.chestPick=b[62]&128;
  t.scrap=b[63]&15;t.gazuas=b[63]>>4;t.race=b[64];t.shirt=b[65];t.trousers=b[66];
  t.guildMember=b[67]&1;t.clubStage=b[68];t.crystals=b[69];t.dungeonFlags=b[70];t.dungeonXY=b[71];
  t.clubSession=get32(b,72);t.tripStage=b[76];t.tripTo=b[77];t.tripRoll=b[78];t.tripDifficulty=b[79];
  t.tripTotal=b[80];t.tripEnemy=b[81];t.tripSurvival=b[82];t.tripLuck=b[83];
  t.rations=b[84];t.charts=b[85];t.charms=b[86];t.dungeonLoot=b[87];t.dungeonEnemies=b[88];t.dungeonClears=b[89];
  t.campStage=b[90]&3;t.campRation=b[90]&4;t.campKit=b[90]&8;t.sleepKit=b[90]&16;
  t.campRoll=b[91];t.eventDay=get16(b,92);t.eventLevel=b[94];t.eventProgress=b[95]&7;t.eventFlags=b[95]>>3;
  t.eventStage=b[96]&7;t.eventRoll=b[96]>>3;t.eventTier=b[97]&3;t.eventOfferSlot=(b[97]>>2)&1;
  t.eventKind=(b[97]>>3)&7;t.eventOriginCity=b[98];t.eventOriginPage=b[99];
  t.dndProgression=b[100];t.tough=b[101];t.advancementSpent=b[102];
  if(t.dndProgression)memcpy(t.attributes,b+103,6);
  t.oath=b[109];t.laySpent=b[110];t.channelSpent=b[111];t.sacredTurns=b[112];t.turnedTurns=b[113];
  t.windSpent=b[114];t.surgeSpent=b[115];t.surgePending=b[116]&1;t.surgeTurnUsed=b[116]&2;
  t.questId|=(b[116]>>2)<<2;t.rageSpent=b[117];t.rageTurns=b[118];
  t.enemyBeat=b[119]&3;t.islandTraps=(b[119]>>2)&7;t.islandCleared=b[119]&32;
  t.campaignFlags=b[120];t.campaignStage=b[121]&15;t.campaignEnding=b[121]>>4;
  t.originStory=b[122];t.originPage=b[123];t.fatigue=b[124];
  if(format==30){t.dungeonGenerator=b[143];t.dungeonSeed=get32(b,148);
    t.dungeonRun=uint32_t(b[153])|uint32_t(b[154])<<8|uint32_t(b[155])<<16;}
  auto& d=t.daily;
  d.phase=DailyPhase(b[125]);d.origin=b[126];d.totalDays=b[127];d.day=b[128];
  d.rationsPaid=b[129];d.nightsDone=b[130];d.region=b[131];d.roll=b[132];
  d.difficulty=b[133];d.survivalBonus=b[134];d.luckBonus=b[135];d.kit=b[136];
  d.total=b[137];d.enemy=b[138];d.supplies=b[139];d.result=b[140];
  d.policy=b[141];d.routeVersion=b[142];
  t.seenEnemies=uint32_t(b[54])|uint32_t(b[47]>>5)<<8|uint32_t(b[50]>>1)<<11|
    uint32_t(b[58]>>6)<<18|uint32_t(b[67]>>1)<<20|uint32_t(b[119]>>6)<<27;
  if(!persistentValid(t))return Decode::Corrupt;
  // Reject noncanonical booleans and packed/reserved bits, not just fields
  // which happen to survive unpacking. This also protects future extensions.
  uint8_t check[PERSISTENT_SAVE_SIZE];encodePersistentVersion(t,s,slot,check,format);
  if(memcmp(b,check,PERSISTENT_SAVE_SIZE))return Decode::Corrupt;
  g=t;seq=s;return Decode::Ok;
}
inline void encodeSlotLife(uint8_t slot,SlotLife life,uint8_t* b){
  memset(b,0,SLOT_LIFECYCLE_SIZE);memcpy(b,"SL29",4);b[4]=1;b[5]=slot;b[6]=uint8_t(life);
  put32(b,8,SAVE_RESET_EPOCH);put32(b,12,crc(b,12));
}
inline bool decodeSlotLife(const uint8_t* b,uint8_t slot,SlotLife& life){
  if(memcmp(b,"SL29",4)||b[4]!=1||b[5]!=slot||b[7]||get32(b,8)!=SAVE_RESET_EPOCH||
     get32(b,12)!=crc(b,12)||(b[6]!=1&&b[6]!=2))return false;
  life=SlotLife(b[6]);return true;
}
}

// Store must preserve Missing versus I/O failure, and exposes raw typed blobs.
// Its ready flag is granted only by the completed one-time reset barrier.
// Callers serialize save/delete/create operations and discard journals when a
// slot is deleted or recreated; metadata is not a concurrent-writer lease.
template<class Store> struct PersistentSlotBackend {
  Store& store;uint8_t slot=0;
  explicit PersistentSlotBackend(Store& s):store(s){}
  const char* key(bool side)const{static const char* k[3][2]={{"a","b"},{"a1","b1"},{"a2","b2"}};return k[slot<3?slot:0][side];}
  const char* metaKey()const{static const char* k[]={"m0","m1","m2"};return k[slot<3?slot:0];}
  const char* guideKey()const{static const char* k[]={"h0","h1","h2"};return k[slot<3?slot:0];}
  bool ready()const{return store.ready&&slot<3;}
  rpg::Read readLife(rpg::SlotLife& life){
    if(!ready())return rpg::Read::Error;
    uint8_t b[rpg::SLOT_LIFECYCLE_SIZE];
    auto r=store.read(metaKey(),b,sizeof(b));
    return r==rpg::Read::Ok&&!rpg::decodeSlotLife(b,slot,life)?rpg::Read::Corrupt:r;
  }
  rpg::Read readRaw(bool side,uint8_t* b){return ready()?store.read(key(side),b,rpg::PERSISTENT_SAVE_SIZE):rpg::Read::Error;}
  rpg::Read rawPresence(){
    if(!ready())return rpg::Read::Error;
    bool found=false;for(const char* k:{key(false),key(true),guideKey()}){
      auto r=store.presence(k);if(r==rpg::Read::Error)return r;
      if(r!=rpg::Read::Missing)found=true;
    }return found?rpg::Read::Ok:rpg::Read::Missing;
  }
  bool publishLife(rpg::SlotLife life){
    if(!ready())return false;
    uint8_t b[rpg::SLOT_LIFECYCLE_SIZE],check[rpg::SLOT_LIFECYCLE_SIZE];
    rpg::encodeSlotLife(slot,life,b);store.write(metaKey(),b,sizeof(b));
    // A failed acknowledgement may conceal a committed marker.
    return store.read(metaKey(),check,sizeof(check))==rpg::Read::Ok&&!memcmp(b,check,sizeof(b));
  }
  bool writeRaw(bool side,const uint8_t* b){
    if(!ready())return false;
    uint8_t check[rpg::PERSISTENT_SAVE_SIZE];store.write(key(side),b,sizeof(check));
    return readRaw(side,check)==rpg::Read::Ok&&!memcmp(b,check,sizeof(check));
  }
  bool clearRaw(const char* k){
    if(!ready())return false;
    auto r=store.presence(k);if(r==rpg::Read::Missing)return true;
    if(r==rpg::Read::Error)return false;
    store.erase(k);return store.presence(k)==rpg::Read::Missing;
  }
  bool remove(){
    if(!publishLife(rpg::SlotLife::Deleted))return false;
    // The deletion is committed. Cleanup failure cannot resurrect a character.
    clearRaw(key(false));clearRaw(key(true));clearRaw(guideKey());return true;
  }
  bool create(const uint8_t* b){
    rpg::SlotLife life;auto r=readLife(life);
    if(r==rpg::Read::Missing){if(rawPresence()!=rpg::Read::Missing||!publishLife(rpg::SlotLife::Deleted))return false;}
    else if(r!=rpg::Read::Ok||life!=rpg::SlotLife::Deleted)return false;
    // Deleted stays committed until the replacement has been read-verified.
    if(!clearRaw(key(false))||!clearRaw(key(true))||!clearRaw(guideKey()))return false;
    if(!writeRaw(false,b))return false;
    return publishLife(rpg::SlotLife::Live);
  }
};

namespace rpg {
template<class Backend> struct PersistentJournal {
  Backend& io;int active=-1;uint32_t seq=0,snapshotCrc=0;uint8_t boundSlot=255;bool blocked=true,fatigueReset=false;
  bool pending=false;bool pendingSide=false;uint8_t pendingBytes[PERSISTENT_SAVE_SIZE]{};
  explicit PersistentJournal(Backend& b):io(b){}
  Load load(Game& g){
    blocked=true;active=-1;seq=0;snapshotCrc=0;boundSlot=io.slot;pending=false;
    SlotLife life;auto meta=io.readLife(life);
    if(meta==Read::Missing){if(io.rawPresence()!=Read::Missing)return Load::Blocked;blocked=false;return Load::Empty;}
    if(meta!=Read::Ok)return Load::Blocked;
    if(life==SlotLife::Deleted){blocked=false;return Load::Empty;}
    uint8_t bytes[2][PERSISTENT_SAVE_SIZE];Game states[2];uint32_t s[2]={};bool ok[2]={};Read r[2];
    for(int i=0;i<2;++i){r[i]=io.readRaw(i,bytes[i]);if(r[i]==Read::Error)return Load::Blocked;
      if(r[i]==Read::Ok){auto d=decodePersistent(bytes[i],io.slot,states[i],s[i]);if(d==Decode::Unsupported)return Load::Blocked;ok[i]=d==Decode::Ok;}}
    if(!ok[0]&&!ok[1])return Load::Blocked; // Live never becomes Empty by data loss.
    if(ok[0]&&ok[1]){
      uint32_t distance=s[1]-s[0];
      if(distance==0x80000000u||(distance==0&&memcmp(bytes[0],bytes[1],PERSISTENT_SAVE_SIZE)))return Load::Blocked;
    }
    active=ok[1]&&(!ok[0]||int32_t(s[1]-s[0])>0)?1:0;seq=s[active];snapshotCrc=get32(bytes[active],156);g=states[active];blocked=false;
    return (!ok[1-active]&&r[1-active]!=Read::Missing)?Load::Recovered:Load::Ok;
  }
  bool save(const Game& g){
    if(blocked||!io.ready()||io.slot!=boundSlot||!persistentValid(g))return false;
    uint8_t desired[PERSISTENT_SAVE_SIZE],raw[PERSISTENT_SAVE_SIZE];SlotLife life;
    auto meta=io.readLife(life);
    if(active<0){
      if(meta==Read::Ok&&life==SlotLife::Live){
        // First creation may have committed despite a lost Live acknowledgement.
        // Never erase or replace a visible hero while retrying that operation.
        if(!pending)return false;
        auto r=io.readRaw(false,raw);
        if(r!=Read::Ok||memcmp(raw,pendingBytes,sizeof(raw)))return false;
        encodePersistent(g,1,io.slot,desired);if(memcmp(desired,raw,sizeof(raw)))return false;
        active=0;seq=1;snapshotCrc=get32(raw,156);pending=false;return true;
      }
      if(meta!=Read::Missing&&(meta!=Read::Ok||life!=SlotLife::Deleted))return false;
      encodePersistent(g,1,io.slot,pendingBytes);pending=true;pendingSide=false;
      if(!io.create(pendingBytes))return false;
      active=0;seq=1;snapshotCrc=get32(pendingBytes,156);pending=false;return true;
    }
    if(meta!=Read::Ok||life!=SlotLife::Live)return false;
    if(pending){
      auto r=io.readRaw(pendingSide,raw);if(r==Read::Error)return false;
      if(r==Read::Ok){Game observed;uint32_t n=0;auto d=decodePersistent(raw,io.slot,observed,n);
        if(d==Decode::Unsupported)return false;
        if(d==Decode::Ok&&n==seq+1){
          if(memcmp(raw,pendingBytes,sizeof(raw)))return false;
          active=pendingSide;seq=n;snapshotCrc=get32(raw,156);pending=false;
          encodePersistent(g,seq,io.slot,desired);if(!memcmp(raw,desired,sizeof(raw)))return true;
          // Known committed attempt, followed by an intentional compensation.
        }else if(d==Decode::Ok&&int32_t(n-seq)>0)return false;
      }
      pending=false;
    }
    // Check both current records before writing; a stale observer cannot overwrite
    // an unexpected later commit or a future-format record.
    for(int side=0;side<2;++side){
      auto r=io.readRaw(side,raw);if(r==Read::Error)return false;
      if(r==Read::Ok){Game observed;uint32_t n=0;auto d=decodePersistent(raw,io.slot,observed,n);
        if(d==Decode::Unsupported)return false;
        if(d==Decode::Ok&&(uint32_t(n-seq)==0x80000000u||int32_t(n-seq)>0))return false;
        if(d==Decode::Ok&&n==seq&&get32(raw,156)!=snapshotCrc)return false;
        if(side==active&&(d!=Decode::Ok||n!=seq||get32(raw,156)!=snapshotCrc))return false;
      }else if(side==active)return false;
    }
    pendingSide=active==0;encodePersistent(g,seq+1,io.slot,pendingBytes);pending=true;
    if(!io.writeRaw(pendingSide,pendingBytes))return false;
    active=pendingSide;++seq;snapshotCrc=get32(pendingBytes,156);pending=false;return true;
  }
};
}
