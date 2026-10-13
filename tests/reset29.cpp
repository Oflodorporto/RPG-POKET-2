#include "../firmware/RPG_POKET_2/ResetEpoch.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>
using namespace rpg;

using Bytes=std::vector<uint8_t>;
using Keys=std::map<std::string,Bytes>;
constexpr const char* NEW_KEYS[]={"a","b","a1","b1","a2","b2","m0","m1","m2","h0","h1","h2"};
struct Disk {
  Keys legacy,fresh,settings,art;
  bool hasMarker=false;
  Bytes marker;
  bool operator==(const Disk& other)const{
    return legacy==other.legacy&&fresh==other.fresh&&settings==other.settings&&
      art==other.art&&hasMarker==other.hasMarker&&marker==other.marker;
  }
};
enum class Fault {None,Io,LostAck,FalseAck,CorruptWrite,ReadCorrupt,ReadMissing,ReadFalse};
struct PowerCut {};
struct Operation {std::string name,key;bool write;};
static bool oldKey(const std::string& key){
  for(const auto& allowed:RESET_LEGACY_KEYS)if(key==allowed)return true;
  return false;
}
static Bytes markerBytes(ResetPhase phase){
  Bytes bytes(RESET_EPOCH_SIZE);encodeResetEpoch(phase,bytes.data());return bytes;
}
static void setMarker(Disk& disk,ResetPhase phase){disk.hasMarker=true;disk.marker=markerBytes(phase);}
static bool markerIs(const Disk& disk,ResetPhase phase){
  ResetPhase decoded=ResetPhase::Pending;
  return disk.hasMarker&&disk.marker.size()==RESET_EPOCH_SIZE&&
    decodeResetEpoch(disk.marker.data(),decoded)&&decoded==phase;
}
static Disk legacyDisk(){
  Disk disk;
  for(const auto& key:RESET_LEGACY_KEYS)disk.legacy[key]=Bytes(128,0x42);
  for(unsigned slot=0;slot<3;++slot)disk.legacy["t"+std::to_string(slot)]={0};
  // Prefix-like keys must never be swept. These are outside the exact whitelist.
  disk.legacy["a3"]={1,2,3};disk.legacy["h3"]={4};disk.legacy["fatigue"]={5};
  disk.legacy["wifi"]={6};disk.legacy["art"]={7};disk.legacy["m0"]={8};
  disk.settings["wifi"]={10,20,30};disk.settings["ssid"]={40,50};
  disk.art["/RPGPOKET/artes.pak"]={60,70,80};
  return disk;
}
static bool tombstones(const Disk& disk){
  for(unsigned slot=0;slot<3;++slot){
    const auto found=disk.legacy.find("t"+std::to_string(slot));
    if(found==disk.legacy.end()||found->second!=Bytes{1})return false;
  }
  return true;
}
static void preserved(const Disk& before,const Disk& after){
  assert(before.fresh==after.fresh&&before.settings==after.settings&&before.art==after.art);
  for(const auto& item:before.legacy)
    if(!oldKey(item.first)&&item.first!="t0"&&item.first!="t1"&&item.first!="t2")
      assert(after.legacy.at(item.first)==item.second);
}
static void completed(const Disk& before,const Disk& after){
  preserved(before,after);assert(markerIs(after,ResetPhase::Done)&&tombstones(after));
  for(const auto& key:RESET_LEGACY_KEYS)assert(!after.legacy.count(key));
}

// Atomic key operations model NVS. A cut can occur before/after every read,
// lookup, write and erase; no volatile verification survives a new Store.
struct Store {
  Disk& disk;
  std::vector<Operation> operations;
  int cut=-1,boundary=0,faultAt=-1;
  Fault fault=Fault::None;
  std::map<unsigned,Fault> injectedFaults;
  std::string newLookupError;
  unsigned verifiedTombs=0;
  std::set<std::string> verifiedAbsent;
  explicit Store(Disk& value):disk(value){}
  void checkpoint(){if(boundary++==cut)throw PowerCut{};}
  Fault start(const char* name,const std::string& key,bool write=false){
    operations.push_back({name,key,write});checkpoint();
    const auto injected=injectedFaults.find(unsigned(operations.size()-1));
    if(injected!=injectedFaults.end())return injected->second;
    return int(operations.size())-1==faultAt?fault:Fault::None;
  }
  template<class T>T finish(T result){checkpoint();return result;}
  void mayChangeLegacy()const{
    assert(markerIs(disk,ResetPhase::Pending)&&disk.fresh.empty());
  }
  Read readEpoch(uint8_t* bytes){
    const Fault f=start("readEpoch","");
    if(f==Fault::Io)return finish(Read::Error);
    if(f==Fault::ReadCorrupt)return finish(Read::Corrupt);
    if(f==Fault::ReadMissing)return finish(Read::Missing);
    if(!disk.hasMarker)return finish(Read::Missing);
    if(disk.marker.size()!=RESET_EPOCH_SIZE)return finish(Read::Corrupt);
    memcpy(bytes,disk.marker.data(),RESET_EPOCH_SIZE);
    return finish(Read::Ok);
  }
  bool writeEpoch(const uint8_t* bytes){
    const Fault f=start("writeEpoch",bytes[5]==uint8_t(ResetPhase::Done)?"Done":"Pending",true);
    ResetPhase phase=ResetPhase::Pending;assert(decodeResetEpoch(bytes,phase));
    assert(disk.fresh.empty());
    if(phase==ResetPhase::Done){
      assert(tombstones(disk)&&verifiedTombs==7);
      for(const auto& key:RESET_LEGACY_KEYS)assert(!disk.legacy.count(key)&&verifiedAbsent.count(key));
    }else assert(!disk.hasMarker);
    if(f==Fault::Io)return finish(false);
    if(f==Fault::FalseAck)return finish(true);
    disk.hasMarker=true;disk.marker.assign(bytes,bytes+RESET_EPOCH_SIZE);
    if(f==Fault::CorruptWrite)disk.marker[0]^=1;
    return finish(f!=Fault::LostAck);
  }
  Read newFamilyPresence(){
    const Fault f=start("newFamilyPresence","");
    if(f==Fault::Io)return finish(Read::Error);
    if(f==Fault::ReadCorrupt)return finish(Read::Corrupt);
    for(const char* key:NEW_KEYS){
      if(newLookupError==key)return finish(Read::Error);
      if(disk.fresh.count(key))return finish(Read::Ok);
    }
    return finish(Read::Missing);
  }
  Read readLegacyDeleted(uint8_t slot,bool& deleted){
    assert(slot<3);const std::string key="t"+std::to_string(slot);
    const Fault f=start("readLegacyDeleted",key);
    if(f==Fault::Io)return finish(Read::Error);
    if(f==Fault::ReadCorrupt)return finish(Read::Corrupt);
    if(f==Fault::ReadMissing)return finish(Read::Missing);
    const auto found=disk.legacy.find(key);
    if(found==disk.legacy.end())return finish(Read::Missing);
    if(found->second.size()!=1||found->second[0]>1)return finish(Read::Corrupt);
    deleted=f==Fault::ReadFalse?false:found->second[0]!=0;
    if(deleted)verifiedTombs|=1u<<slot;
    return finish(Read::Ok);
  }
  bool writeLegacyDeleted(uint8_t slot){
    assert(slot<3);const std::string key="t"+std::to_string(slot);
    const Fault f=start("writeLegacyDeleted",key,true);mayChangeLegacy();
    if(f==Fault::Io)return finish(false);
    if(f==Fault::FalseAck)return finish(true);
    disk.legacy[key]={uint8_t(f==Fault::CorruptWrite?2:1)};
    return finish(f!=Fault::LostAck);
  }
  Read legacyPresence(const char* key){
    const Fault f=start("legacyPresence",key);assert(oldKey(key));
    if(f==Fault::Io)return finish(Read::Error);
    if(f==Fault::ReadCorrupt)return finish(Read::Corrupt);
    if(disk.legacy.count(key))return finish(Read::Ok);
    verifiedAbsent.insert(key);return finish(Read::Missing);
  }
  bool eraseLegacy(const char* key){
    const Fault f=start("eraseLegacy",key,true);mayChangeLegacy();
    assert(oldKey(key)&&tombstones(disk)&&verifiedTombs==7);
    if(f==Fault::Io)return finish(false);
    if(f==Fault::FalseAck)return finish(true);
    disk.legacy.erase(key);return finish(f!=Fault::LostAck);
  }
};

static unsigned checks=0,cuts=0,faults=0;
static std::vector<Operation> run(Disk& disk){
  const Disk before=disk;Store store(disk);
  const auto result=ensureResetEpoch(store);
  assert(result==ResetResult::ResetPerformed||result==ResetResult::Ready);
  completed(before,disk);++checks;return store.operations;
}
static void blockedUnchanged(Disk& disk){
  const Disk before=disk;Store store(disk);assert(ensureResetEpoch(store)==ResetResult::Blocked);
  assert(disk==before);++checks;
}
static void markerTests(){
  static_assert(SAVE_RESET_EPOCH==1,"Epoch must not track firmware builds or reflashing");
  static_assert(RESET_EPOCH_SIZE==16&&RESET_LEGACY_KEY_COUNT==15,"Persistent schema changed");
  const auto bytes=markerBytes(ResetPhase::Done);
  assert(memcmp(bytes.data(),"RST1",4)==0&&bytes[4]==1&&bytes[5]==2&&!bytes[6]&&!bytes[7]);
  assert(get32(bytes.data(),8)==1&&get32(bytes.data(),12)==crc(bytes.data(),12));
  for(unsigned bit=0;bit<RESET_EPOCH_SIZE*8;++bit){
    Disk disk=legacyDisk();disk.hasMarker=true;disk.marker=bytes;disk.marker[bit/8]^=1u<<(bit%8);
    blockedUnchanged(disk);
  }
  // CRC-valid but unknown marker semantics must not be treated as no marker.
  for(unsigned offset:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u}){
    Disk disk=legacyDisk();setMarker(disk,ResetPhase::Pending);disk.marker[offset]^=0x80;
    put32(disk.marker.data(),12,crc(disk.marker.data(),12));blockedUnchanged(disk);
  }
  for(unsigned epoch:{0u,2u,0xffffffffu}){
    Disk disk=legacyDisk();setMarker(disk,ResetPhase::Done);put32(disk.marker.data(),8,epoch);
    put32(disk.marker.data(),12,crc(disk.marker.data(),12));blockedUnchanged(disk);
  }
  for(unsigned size:{0u,1u,15u,17u,64u}){
    Disk disk=legacyDisk();disk.hasMarker=true;disk.marker=Bytes(size,0);blockedUnchanged(disk);
  }
}
static void protectNewFamily(){
  for(const char* key:NEW_KEYS)for(bool pending:{false,true})for(unsigned size:{0u,1u,128u}){
    Disk disk=legacyDisk();if(pending)setMarker(disk,ResetPhase::Pending);
    disk.fresh[key]=Bytes(size,0xa5);blockedUnchanged(disk);
  }
  for(const char* key:NEW_KEYS)for(bool pending:{false,true}){
    Disk disk=legacyDisk();if(pending)setMarker(disk,ResetPhase::Pending);
    const Disk before=disk;Store store(disk);store.newLookupError=key;
    assert(ensureResetEpoch(store)==ResetResult::Blocked&&disk==before);++checks;
  }
  // After Done, settings, art, old/downgraded saves and all new characters survive.
  // New Store instances simulate later boots/reflashes without firmware identities.
  Disk disk=legacyDisk();setMarker(disk,ResetPhase::Done);
  for(const char* key:NEW_KEYS)disk.fresh[key]={0x29,1,2,3};
  const Disk before=disk;
  for(unsigned boot=0;boot<10;++boot){
    Store store(disk);store.newLookupError="a";
    assert(ensureResetEpoch(store)==ResetResult::Ready&&disk==before);
    assert(store.operations.size()==1&&store.operations[0].name=="readEpoch");++checks;
  }
}
static void sweepCuts(const Disk& initial,std::vector<Disk>* resumed=nullptr){
  Disk success=initial;const auto trace=run(success);
  for(unsigned boundary=0;boundary<trace.size()*2;++boundary){
    Disk disk=initial;Store store(disk);store.cut=int(boundary);bool interrupted=false;
    try{(void)ensureResetEpoch(store);}catch(const PowerCut&){interrupted=true;}
    assert(interrupted);preserved(initial,disk);
    // An incomplete reset is never Ready. No deleted blob can exist before all
    // tombstones are durable. A new boot must resume or observe committed Done.
    bool erased=false;for(const auto& key:RESET_LEGACY_KEYS)
      if(initial.legacy.count(key)&&!disk.legacy.count(key))erased=true;
    if(erased)assert(tombstones(disk));
    if(resumed){bool seen=false;for(const auto& state:*resumed)if(state==disk)seen=true;
      if(!seen)resumed->push_back(disk);}
    run(disk);completed(initial,disk);++cuts;
  }
}
static void faultTests(){
  Disk initial=legacyDisk(),success=initial;const auto trace=run(success);
  assert(trace.size()==60);
  unsigned markerWrites=0,tombWrites=0,erases=0;
  for(const auto& op:trace){
    markerWrites+=op.name=="writeEpoch";tombWrites+=op.name=="writeLegacyDeleted";
    erases+=op.name=="eraseLegacy";
  }
  assert(markerWrites==2&&tombWrites==3&&erases==15);
  for(unsigned index=0;index<trace.size();++index){
    const Operation& op=trace[index];
    for(Fault fault:op.write?std::vector<Fault>{Fault::Io,Fault::FalseAck,Fault::LostAck}:
                             std::vector<Fault>{Fault::Io,Fault::ReadCorrupt}){
      Disk disk=initial;Store store(disk);store.faultAt=int(index);store.fault=fault;
      const auto result=ensureResetEpoch(store);preserved(initial,disk);
      if(fault==Fault::LostAck){
        assert(result==ResetResult::ResetPerformed);completed(initial,disk);
        // Reconcile once by readback, with exactly the same number of effects.
        assert(store.operations.size()==trace.size());
      }else{
        assert(result==ResetResult::Blocked);
        assert(store.operations.size()==index+(op.write?2u:1u));
        // No subsequent destructive work after a failed read/readback.
        if(op.name=="writeEpoch"&&op.key=="Pending")assert(disk==initial);
        if(op.name=="writeLegacyDeleted"||op.name=="readLegacyDeleted")
          for(const auto& key:RESET_LEGACY_KEYS)assert(disk.legacy.at(key)==initial.legacy.at(key));
        run(disk);completed(initial,disk);
      }
      ++faults;
    }
    if(op.write){
      // A lost acknowledgement followed by unreadable verification must block,
      // even though the effect is durable. The next boot resumes from that fact.
      Disk disk=initial;Store store(disk);
      store.injectedFaults={{index,Fault::LostAck},{index+1,Fault::Io}};
      assert(ensureResetEpoch(store)==ResetResult::Blocked);
      assert(store.operations.size()==index+2);preserved(initial,disk);
      Store reboot(disk);const auto result=ensureResetEpoch(reboot);
      assert(result==ResetResult::Ready||result==ResetResult::ResetPerformed);
      completed(initial,disk);
      for(const auto& next:reboot.operations)
        assert(!(next.write&&next.name==op.name&&next.key==op.key));
      ++faults;
    }
    if(op.name=="writeEpoch"||op.name=="writeLegacyDeleted"){
      Disk disk=initial;Store store(disk);store.faultAt=int(index);store.fault=Fault::CorruptWrite;
      assert(ensureResetEpoch(store)==ResetResult::Blocked);preserved(initial,disk);
      const Disk damaged=disk;Store reboot(disk);
      assert(ensureResetEpoch(reboot)==ResetResult::Blocked&&disk==damaged);++faults;
    }
    // A missing/false readback cannot acknowledge a successful write.
    if(index&&trace[index-1].write&&
       (op.name=="readEpoch"||op.name=="readLegacyDeleted")){
      for(Fault fault:op.name=="readEpoch"?std::vector<Fault>{Fault::ReadMissing}:
                                             std::vector<Fault>{Fault::ReadMissing,Fault::ReadFalse}){
        Disk disk=initial;Store store(disk);store.faultAt=int(index);store.fault=fault;
        assert(ensureResetEpoch(store)==ResetResult::Blocked);
        assert(store.operations.size()==index+1);run(disk);completed(initial,disk);++faults;
      }
    }
  }
}
static void resumeTests(){
  Disk disk=legacyDisk();run(disk);
  // A fresh device with genuinely missing legacy keys still installs all three
  // tombstones. Every absent erase target is checked without an erase request.
  Disk blank;Store fresh(blank);assert(ensureResetEpoch(fresh)==ResetResult::ResetPerformed);
  completed(Disk{},blank);for(const auto& op:fresh.operations)assert(op.name!="eraseLegacy");++checks;
  Disk pending=legacyDisk();setMarker(pending,ResetPhase::Pending);
  for(unsigned slot=0;slot<3;++slot)pending.legacy["t"+std::to_string(slot)]={1};
  for(const auto& key:RESET_LEGACY_KEYS)pending.legacy.erase(key);
  Store resume(pending);assert(ensureResetEpoch(resume)==ResetResult::ResetPerformed);
  for(const auto& op:resume.operations){assert(op.name!="writeLegacyDeleted"&&op.name!="eraseLegacy");
    assert(op.name!="writeEpoch"||op.key=="Done");}++checks;
  for(unsigned slot=0;slot<3;++slot){
    Disk corrupt=legacyDisk();corrupt.legacy["t"+std::to_string(slot)]={2};
    Store store(corrupt);assert(ensureResetEpoch(store)==ResetResult::Blocked);
    for(const auto& key:RESET_LEGACY_KEYS)assert(corrupt.legacy.count(key));
    ++checks;
  }
}
int main(){
  markerTests();protectNewFamily();resumeTests();faultTests();
  std::vector<Disk> resumed;sweepCuts(legacyDisk(),&resumed);
  for(const auto& state:resumed)sweepCuts(state);
  printf("PASS: reset29; %u state/schema checks, %u before/after power cuts across %zu persisted resume states, %u I/O/readback/lost-ACK faults; exact legacy whitelist; three tombstones before erases; new records/meta/hints and Wi-Fi/art preserved; Done never resets again.\n",checks,cuts,resumed.size(),faults);
}
