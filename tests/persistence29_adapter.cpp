#include "../firmware/RPG_POKET_2/NvsPersistence.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
using namespace rpg;

using Bytes=std::vector<uint8_t>;
enum class Type {Blob,U8,U32,String};
struct Value {
  Type type=Type::Blob;Bytes bytes;
  bool operator==(const Value& other)const{return type==other.type&&bytes==other.bytes;}
};
using Namespace=std::map<std::string,Value>;
using Image=std::map<std::string,Namespace>;
struct Call {std::string operation,name,key;};
enum class Fault {None,Io,LostAck,NoEffect,ShortRead,Missing};
struct PowerLoss {};

// Unlike a map-only Store mock, this fake implements SDK handles, typed keys,
// size probes, pending writes, explicit commit, failed/lost acknowledgements and
// close discarding uncommitted changes. Tests run the exact production adapter.
struct FakeNvsDisk {
  struct Session {std::string name;bool writable=false;Namespace pending;std::vector<std::string> erased;};
  Image image;
  std::map<unsigned,Session> handles;
  unsigned nextHandle=1;
  std::vector<Call> calls;
  std::string faultOperation,faultNamespace,faultKey;
  Fault fault=Fault::None;
  int faultAt=-1,cut=-1,boundary=0;
  void checkpoint(){if(boundary++==cut)throw PowerLoss{};}
  Fault begin(const char* operation,const std::string& name,const std::string& key=""){
    checkpoint();calls.push_back({operation,name,key});
    const bool selected=faultAt>=0?int(calls.size())-1==faultAt:
      operation==faultOperation&&(faultNamespace.empty()||name==faultNamespace)&&
      (faultKey.empty()||key==faultKey);
    return selected?fault:Fault::None;
  }
  NvsStatus end(NvsStatus result){checkpoint();return result;}
  void disarm(){fault=Fault::None;faultAt=cut=-1;boundary=0;faultOperation.clear();faultNamespace.clear();faultKey.clear();calls.clear();handles.clear();}
  void fail(const char* op,const char* ns,const char* key,Fault mode){
    disarm();faultOperation=op;faultNamespace=ns;faultKey=key;fault=mode;
  }
};

struct FakeNvsDriver {
  using Handle=unsigned;
  FakeNvsDisk* disk=nullptr;
  explicit FakeNvsDriver(FakeNvsDisk& value):disk(&value){}
  NvsStatus open(const char* name,bool write,Handle& handle){
    const auto fault=disk->begin(write?"openWrite":"openRead",name);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(!disk->image.count(name)){
      if(!write)return disk->end(NvsStatus::Missing);
      disk->image[name]={};
    }
    handle=disk->nextHandle++;disk->handles[handle]={name,write,{}, {}};
    return disk->end(NvsStatus::Ok);
  }
  void close(Handle handle){disk->handles.erase(handle);}
  const Value* find(Handle handle,const char* key){
    auto& session=disk->handles.at(handle);
    auto pending=session.pending.find(key);if(pending!=session.pending.end())return &pending->second;
    for(const auto& erased:session.erased)if(erased==key)return nullptr;
    const auto& space=disk->image.at(session.name);auto value=space.find(key);
    return value==space.end()?nullptr:&value->second;
  }
  NvsStatus getBlob(Handle handle,const char* key,void* out,size_t& length){
    const auto fault=disk->begin(out?"readBlob":"probeBlob",disk->handles.at(handle).name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(fault==Fault::Missing)return disk->end(NvsStatus::Missing);
    const auto* value=find(handle,key);
    if(!value)return disk->end(NvsStatus::Missing);
    if(value->type!=Type::Blob)return disk->end(NvsStatus::TypeMismatch);
    if(!out){length=value->bytes.size();return disk->end(NvsStatus::Ok);}
    if(length<value->bytes.size()){length=value->bytes.size();return disk->end(NvsStatus::Error);}
    length=value->bytes.size();if(fault==Fault::ShortRead&&length)--length;
    if(length)memcpy(out,value->bytes.data(),length);
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
  NvsStatus getU8(Handle handle,const char* key,uint8_t& value){
    const auto fault=disk->begin("readU8",disk->handles.at(handle).name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    const auto* found=find(handle,key);
    if(!found)return disk->end(NvsStatus::Missing);
    if(found->type!=Type::U8)return disk->end(NvsStatus::TypeMismatch);
    assert(found->bytes.size()==1);value=found->bytes[0];return disk->end(NvsStatus::Ok);
  }
  NvsStatus setBlob(Handle handle,const char* key,const void* bytes,size_t length){
    auto& session=disk->handles.at(handle);assert(session.writable);
    const auto fault=disk->begin("setBlob",session.name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(fault!=Fault::NoEffect){const auto* begin=static_cast<const uint8_t*>(bytes);session.pending[key]={Type::Blob,Bytes(begin,begin+length)};}
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
  NvsStatus setU8(Handle handle,const char* key,uint8_t value){
    auto& session=disk->handles.at(handle);assert(session.writable);
    const auto fault=disk->begin("setU8",session.name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(fault!=Fault::NoEffect)session.pending[key]={Type::U8,{value}};
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
  NvsStatus eraseKey(Handle handle,const char* key){
    auto& session=disk->handles.at(handle);assert(session.writable);
    const auto fault=disk->begin("erase",session.name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(!find(handle,key))return disk->end(NvsStatus::Missing);
    if(fault!=Fault::NoEffect)session.erased.push_back(key);
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
  NvsStatus commit(Handle handle){
    auto& session=disk->handles.at(handle);assert(session.writable);
    const auto fault=disk->begin("commit",session.name);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(fault!=Fault::NoEffect){
      auto& space=disk->image[session.name];
      for(const auto& erased:session.erased)space.erase(erased);
      for(const auto& value:session.pending)space[value.first]=value.second;
    }
    session.pending.clear();session.erased.clear();
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
};
using Store=NvsStoreFor<FakeNvsDriver>;
using Backend=NvsBackendFor<FakeNvsDriver>;
using Journal29=NvsJournalFor<FakeNvsDriver>;
static_assert(std::is_same<Backend,PersistentSlotBackend<Store>>::value,"Production backend alias");
static_assert(std::is_same<Journal29,PersistentJournal<Backend>>::value,"Production journal alias");
static_assert(PERSISTENT_SAVE_SIZE==160&&CURRENT_SAVE_FORMAT==30,"Save29 record contract");
constexpr const char* newKeys[]={"a","b","a1","b1","a2","b2","m0","m1","m2","h0","h1","h2"};
unsigned checks=0,cutChecks=0,faultChecks=0;

static Bytes packed(const Game& game,unsigned slot,uint32_t seq=1){
  Bytes bytes(PERSISTENT_SAVE_SIZE);encodePersistent(game,seq,uint8_t(slot),bytes.data());return bytes;
}
static Bytes epoch(ResetPhase phase){Bytes bytes(RESET_EPOCH_SIZE);encodeResetEpoch(phase,bytes.data());return bytes;}
static bool epochIs(const FakeNvsDisk& disk,ResetPhase phase){
  const auto space=disk.image.find("pkt2_v29");if(space==disk.image.end())return false;
  const auto value=space->second.find("epoch");
  return value!=space->second.end()&&value->second==Value{Type::Blob,epoch(phase)};
}
static Game hero(unsigned slot){
  auto game=create(slot,77+slot);game.p.gold=300+slot;game.fatigue=20+slot;
  game.race=slot;game.shirt=slot+1;game.trousers=slot+2;return game;
}
static FakeNvsDisk legacyDisk(){
  FakeNvsDisk disk;
  for(const auto& key:RESET_LEGACY_KEYS)disk.image["pkt2_slice"][key]={Type::Blob,Bytes(128,0x42)};
  for(unsigned slot=0;slot<3;++slot)disk.image["pkt2_slice"]["t"+std::to_string(slot)]={Type::U8,{0}};
  disk.image["pkt2_slice"]["a3"]={Type::Blob,{1,2,3}};
  disk.image["pkt2_slice"]["h3"]={Type::U32,{1,2,3,4}};
  disk.image["pkt2_slice"]["wifi"]={Type::String,{5,6,7,0}};
  disk.image["pkt2_slice"]["m0"]={Type::Blob,{8}};
  disk.image["pkt2_ui"]["wifi"]={Type::Blob,{20,30,40}};
  disk.image["nvs.net80211"]["ssid"]={Type::String,{60,70,0}};
  disk.image["sd-art-sentinel"]["artes.pak"]={Type::Blob,{90,100,110}};
  return disk;
}
static void preserved(const Image& before,const FakeNvsDisk& disk){
  for(const auto& name:before){
    if(name.first=="pkt2_v29"||name.first=="pkt2_slice")continue;
    assert(disk.image.at(name.first)==name.second);
  }
  const auto legacy=before.find("pkt2_slice");
  if(legacy!=before.end())for(const auto& entry:legacy->second){
    bool allowed=entry.first=="t0"||entry.first=="t1"||entry.first=="t2";
    for(const auto& key:RESET_LEGACY_KEYS)if(entry.first==key)allowed=true;
    if(!allowed)assert(disk.image.at("pkt2_slice").at(entry.first)==entry.second);
  }
  for(const auto& call:disk.calls)assert(call.name=="pkt2_v29"||call.name=="pkt2_slice");
  ++checks;
}
static void resetComplete(const Image& before,const FakeNvsDisk& disk){
  preserved(before,disk);assert(epochIs(disk,ResetPhase::Done));
  for(const auto& key:RESET_LEGACY_KEYS)assert(!disk.image.at("pkt2_slice").count(key));
  for(unsigned slot=0;slot<3;++slot)assert((disk.image.at("pkt2_slice").at("t"+std::to_string(slot))==Value{Type::U8,{1}}));
}
static void assertClosed(const FakeNvsDisk& disk){assert(disk.handles.empty());}
static Store storeFor(FakeNvsDisk& disk){return Store(FakeNvsDriver(disk));}

static void bootCreateSaveReload(){
  auto disk=legacyDisk();const auto original=disk.image;auto store=storeFor(disk);
  uint8_t scratch[PERSISTENT_SAVE_SIZE]{};
  // Every normal operation is closed until the reset barrier has been verified.
  for(const char* key:newKeys){
    assert(store.read(key,scratch,sizeof(scratch))==Read::Error);
    assert(store.presence(key)==Read::Error&&!store.erase(key)&&!store.write(key,scratch,sizeof(scratch)));
  }
  assert(!store.ready&&store.readGuide(0)==0&&!store.writeGuide(0,42)&&disk.calls.empty());
  assert(store.init()==ResetResult::ResetPerformed&&store.ready);resetComplete(original,disk);assertClosed(disk);
  Backend backend(store);Game loaded;
  for(unsigned slot=0;slot<3;++slot){
    backend.slot=slot;Journal29 preview(backend);assert(preview.load(loaded)==Load::Empty);
    assert(!preview.blocked&&preview.active==-1&&preview.seq==0&&!preview.fatigueReset);
    assert(store.readGuide(slot)==0&&!store.writeGuide(slot,0x8000));
  }
  for(unsigned slot=0;slot<3;++slot){
    backend.slot=slot;Journal29 journal(backend);auto game=hero(slot);
    assert(journal.load(loaded)==Load::Empty&&journal.save(game));
    assert(journal.active==0&&journal.seq==1&&!journal.fatigueReset);
    // Hint activation follows the first verified Live character publication.
    assert(store.writeGuide(slot,0xaaaa8000u+slot));
    game.p.gold+=100;game.fatigue+=10;assert(journal.save(game)&&journal.seq==2&&journal.active==1);
    assert(disk.image["pkt2_v29"][backend.key(true)].bytes==packed(game,slot,2));
  }
  auto saved=disk.image;disk.disarm();
  // Downgrade leftovers and even an inaccessible legacy namespace cannot make
  // a completed epoch repeat cleanup or prevent ordinary Save29 boot.
  disk.image["pkt2_slice"]["a"]={Type::Blob,{0xaa}};
  disk.fail("openRead","pkt2_slice","",Fault::Io);auto reboot=storeFor(disk);
  assert(reboot.init()==ResetResult::Ready&&reboot.ready);
  assert(disk.image["pkt2_slice"]["a"].bytes==Bytes{0xaa});
  for(const auto& call:disk.calls)assert(call.name=="pkt2_v29");
  Backend reloaded(reboot);
  for(unsigned slot=0;slot<3;++slot){
    reloaded.slot=slot;Journal29 journal(reloaded);auto expected=hero(slot);expected.p.gold+=100;expected.fatigue+=10;
    assert(journal.load(loaded)==Load::Ok&&packed(loaded,slot,2)==packed(expected,slot,2));
    assert(journal.seq==2&&journal.active==1&&reboot.readGuide(slot)==0xaaaa8000u+slot);
    assert(!journal.fatigueReset);
  }
  for(unsigned slot=0;slot<3;++slot)assert(disk.image["pkt2_slice"]["t"+std::to_string(slot)]==saved["pkt2_slice"]["t"+std::to_string(slot)]);
  preserved(original,disk);assertClosed(disk);
}

static void exactReadsAndProtection(){
  FakeNvsDisk disk;auto store=storeFor(disk);assert(store.init()==ResetResult::ResetPerformed);
  uint8_t bytes[PERSISTENT_SAVE_SIZE]{};
  assert(store.read("a",bytes,sizeof(bytes))==Read::Missing);
  for(unsigned length:{0u,1u,63u,96u,127u,128u,159u,160u,161u,256u,4096u}){
    disk.image["pkt2_v29"]["a"]={Type::Blob,Bytes(length,0x5a)};
    const auto expected=length==160?Read::Ok:length<160?Read::Corrupt:Read::Error;
    assert(store.read("a",bytes,sizeof(bytes))==expected&&store.presence("a")==Read::Ok);++checks;
  }
  for(Type type:{Type::U8,Type::U32,Type::String}){
    disk.image["pkt2_v29"]["a"]={type,{1,2,3,4}};
    assert(store.read("a",bytes,sizeof(bytes))==Read::Error&&store.presence("a")==Read::Ok);++checks;
  }
  disk.image["pkt2_v29"]["a"]={Type::Blob,packed(hero(0),0)};
  for(const char* op:{"openRead","probeBlob","readBlob"}){
    disk.fail(op,"pkt2_v29",strcmp(op,"openRead")?"a":"",Fault::Io);
    assert(store.read("a",bytes,sizeof(bytes))==Read::Error);++checks;
  }
  for(Fault fault:{Fault::ShortRead,Fault::Missing,Fault::LostAck}){
    disk.fail("readBlob","pkt2_v29","a",fault);assert(store.read("a",bytes,sizeof(bytes))==Read::Error);++checks;
  }
  disk.disarm();const auto before=disk.image;
  for(const char* key:{"epoch","t0","a3","wifi","ssid","/RPGPOKET/artes.pak",""}){
    assert(store.read(key,bytes,sizeof(bytes))==Read::Error&&!store.write(key,bytes,sizeof(bytes)));
    assert(store.presence(key)==Read::Error&&!store.erase(key));
    assert(store.legacyPresence(key)==Read::Error&&!store.eraseLegacy(key));
  }
  assert(!store.write(nullptr,bytes,sizeof(bytes))&&!store.erase(nullptr));
  assert(!store.write("a",nullptr,160)&&!store.write("a",bytes,0));
  assert(store.read("a",nullptr,160)==Read::Error&&store.read("a",bytes,0)==Read::Error);
  assert(!store.writeGuide(3,1)&&store.readGuide(3)==0&&!store.writeLegacyDeleted(3));
  assert(disk.image==before&&disk.calls.empty());assertClosed(disk);
}

static void protectedMarkersAndFamily(){
  for(bool pending:{false,true})for(const char* key:newKeys)for(Type type:{Type::Blob,Type::U8,Type::U32,Type::String}){
    auto disk=legacyDisk();if(pending)disk.image["pkt2_v29"]["epoch"]={Type::Blob,epoch(ResetPhase::Pending)};
    disk.image["pkt2_v29"][key]={type,{}};const auto before=disk.image;auto store=storeFor(disk);
    assert(store.init()==ResetResult::Blocked&&!store.ready&&disk.image==before);assertClosed(disk);++checks;
  }
  for(const char* key:newKeys){
    auto disk=legacyDisk();disk.image["pkt2_v29"]={};disk.fail("probeBlob","pkt2_v29",key,Fault::Io);
    const auto before=disk.image;auto store=storeFor(disk);assert(store.init()==ResetResult::Blocked&&!store.ready&&disk.image==before);++checks;
  }
  for(unsigned length:{0u,1u,15u,17u,160u}){
    auto disk=legacyDisk();disk.image["pkt2_v29"]["epoch"]={Type::Blob,Bytes(length,0)};
    const auto before=disk.image;auto store=storeFor(disk);assert(store.init()==ResetResult::Blocked&&!store.ready&&disk.image==before);++checks;
  }
  for(Type type:{Type::U8,Type::U32,Type::String}){
    auto disk=legacyDisk();disk.image["pkt2_v29"]["epoch"]={type,{1}};
    const auto before=disk.image;auto store=storeFor(disk);assert(store.init()==ResetResult::Blocked&&!store.ready&&disk.image==before);++checks;
  }
  for(unsigned offset:{0u,4u,5u,6u,7u,8u,9u,10u,11u,12u}){
    auto disk=legacyDisk();auto unknown=epoch(ResetPhase::Done);unknown[offset]^=0x80;
    if(offset<12)put32(unknown.data(),12,crc(unknown.data(),12));
    disk.image["pkt2_v29"]["epoch"]={Type::Blob,unknown};const auto before=disk.image;auto store=storeFor(disk);
    assert(store.init()==ResetResult::Blocked&&!store.ready&&disk.image==before);++checks;
  }
  for(const char* op:{"openRead","probeBlob","readBlob"}){
    auto disk=legacyDisk();disk.image["pkt2_v29"]["epoch"]={Type::Blob,epoch(ResetPhase::Done)};
    disk.fail(op,"pkt2_v29",strcmp(op,"openRead")?"epoch":"",Fault::Io);const auto before=disk.image;auto store=storeFor(disk);
    assert(store.init()==ResetResult::Blocked&&!store.ready&&disk.image==before);++checks;
  }
  // Legacy tombstones are typed u8, never guessed from malformed bytes.
  for(Type type:{Type::Blob,Type::U32,Type::String}){
    auto disk=legacyDisk();disk.image["pkt2_slice"]["t0"]={type,{1}};const auto before=disk.image;auto store=storeFor(disk);
    assert(store.init()==ResetResult::Blocked&&!store.ready);preserved(before,disk);
    for(const auto& key:RESET_LEGACY_KEYS)assert(disk.image["pkt2_slice"][key]==before.at("pkt2_slice").at(key));
  }
  auto disk=legacyDisk();disk.image["pkt2_slice"]["t0"]={Type::U8,{2}};auto store=storeFor(disk);
  assert(store.init()==ResetResult::Blocked&&!store.ready);++checks;
}

static void commitAndCleanupFailures(){
  // Failed or falsely successful commit must never be satisfied by uncommitted
  // reads from the same handle. Exact readback through a fresh handle is tested.
  for(Fault fault:{Fault::Io,Fault::NoEffect}){
    auto disk=legacyDisk();const auto before=disk.image;disk.fail("commit","pkt2_v29","",fault);auto store=storeFor(disk);
    assert(store.init()==ResetResult::Blocked&&!store.ready&&!disk.image["pkt2_v29"].count("epoch"));
    assert(disk.image["pkt2_slice"]==before.at("pkt2_slice"));assertClosed(disk);++checks;
  }
  for(Fault fault:{Fault::Io,Fault::NoEffect}){
    auto disk=legacyDisk();const auto before=disk.image;disk.fail("commit","pkt2_slice","",fault);auto store=storeFor(disk);
    assert(store.init()==ResetResult::Blocked&&!store.ready&&epochIs(disk,ResetPhase::Pending));
    assert(disk.image["pkt2_slice"]==before.at("pkt2_slice"));assertClosed(disk);++checks;
  }
  for(const char* ns:{"pkt2_v29","pkt2_slice"}){
    auto disk=legacyDisk();const auto before=disk.image;disk.fail("commit",ns,"",Fault::LostAck);auto store=storeFor(disk);
    assert(store.init()==ResetResult::ResetPerformed&&store.ready);resetComplete(before,disk);assertClosed(disk);
  }
  for(Type type:{Type::U8,Type::U32,Type::String}){
    auto disk=legacyDisk();disk.image["pkt2_slice"]["a"]={type,{9,8,7}};const auto before=disk.image;auto store=storeFor(disk);
    assert(store.init()==ResetResult::ResetPerformed);resetComplete(before,disk);
  }
  for(const char* op:{"erase","setU8"})for(Fault fault:{Fault::Io,Fault::NoEffect}){
    auto disk=legacyDisk();const auto before=disk.image;disk.fail(op,"pkt2_slice",strcmp(op,"erase")?"t1":"a",fault);auto store=storeFor(disk);
    assert(store.init()==ResetResult::Blocked&&!store.ready&&epochIs(disk,ResetPhase::Pending));
    for(const auto& key:RESET_LEGACY_KEYS)assert(disk.image["pkt2_slice"][key]==before.at("pkt2_slice").at(key));
    disk.disarm();assert(store.init()==ResetResult::ResetPerformed&&store.ready);resetComplete(before,disk);
  }
}

static void journalProtectionAndLifecycle(){
  for(unsigned slot=0;slot<3;++slot){
    FakeNvsDisk disk;auto store=storeFor(disk);assert(store.init()==ResetResult::ResetPerformed);
    Backend backend(store);backend.slot=slot;Journal29 journal(backend);Game loaded;auto game=hero(slot);
    assert(journal.load(loaded)==Load::Empty&&journal.save(game));
    const auto clean=disk.image;const std::string inactive=backend.key(true);
    for(unsigned length:{0u,1u,128u,159u}){
      disk.image=clean;disk.image["pkt2_v29"][inactive]={Type::Blob,Bytes(length,0x55)};
      Journal29 recovered(backend);assert(recovered.load(loaded)==Load::Recovered&&packed(loaded,slot)==packed(game,slot));++checks;
    }
    for(unsigned length:{161u,256u}){
      disk.image=clean;disk.image["pkt2_v29"][inactive]={Type::Blob,Bytes(length,0x55)};const auto before=disk.image;
      Journal29 blocked(backend);assert(blocked.load(loaded)==Load::Blocked&&!blocked.save(game)&&disk.image==before);++checks;
    }
    for(Type type:{Type::U8,Type::U32,Type::String}){
      disk.image=clean;disk.image["pkt2_v29"][inactive]={type,{1}};const auto before=disk.image;
      Journal29 blocked(backend);assert(blocked.load(loaded)==Load::Blocked&&!blocked.save(game)&&disk.image==before);++checks;
    }
    disk.image=clean;auto future=packed(game,slot);put16(future.data(),4,CURRENT_SAVE_FORMAT+1);put32(future.data(),156,crc(future.data(),156));
    disk.image["pkt2_v29"][inactive]={Type::Blob,future};auto before=disk.image;Journal29 unsupported(backend);
    assert(unsupported.load(loaded)==Load::Blocked&&!unsupported.save(game)&&disk.image==before);
    disk.image=clean;disk.image["pkt2_v29"][inactive]={Type::Blob,packed(game,slot,2)};
    disk.fail("readBlob","pkt2_v29",inactive.c_str(),Fault::Io);before=disk.image;Journal29 unreadable(backend);
    assert(unreadable.load(loaded)==Load::Blocked&&!unreadable.save(game)&&disk.image==before);disk.disarm();
    disk.image=clean;assert(store.writeGuide(slot,0x12345678)&&store.readGuide(slot)==0x12345678);
    // Committed deletion remains Empty even if raw cleanup fails. Old hints
    // cannot leak through; replacement clears leftovers before publishing Live.
    disk.fail("erase","pkt2_v29","",Fault::Io);assert(backend.remove());
    assert(store.readGuide(slot)==0&&!store.writeGuide(slot,0xabcdef01));
    Journal29 deleted(backend);assert(deleted.load(loaded)==Load::Empty);
    disk.disarm();game.p.gold+=50;assert(deleted.save(game)&&deleted.seq==1);
    Journal29 recreated(backend);assert(recreated.load(loaded)==Load::Ok&&packed(loaded,slot)==packed(game,slot));
    assert(store.readGuide(slot)==0);assertClosed(disk);++checks;
  }
}

static void resetCutsAndErrors(){
  auto baseline=legacyDisk();const auto before=baseline.image;auto store=storeFor(baseline);
  assert(store.init()==ResetResult::ResetPerformed);const int count=baseline.boundary;const auto calls=baseline.calls;
  for(int cut=0;cut<count;++cut){
    auto disk=legacyDisk();disk.cut=cut;auto interrupted=storeFor(disk);
    try{(void)interrupted.init();}catch(const PowerLoss&){}
    disk.disarm();auto reboot=storeFor(disk);const auto result=reboot.init();
    assert(result==ResetResult::Ready||result==ResetResult::ResetPerformed);assert(reboot.ready);
    resetComplete(before,disk);assertClosed(disk);++cutChecks;
  }
  for(unsigned index=0;index<calls.size();++index){
    auto disk=legacyDisk();disk.faultAt=int(index);disk.fault=Fault::Io;auto interrupted=storeFor(disk);
    const auto result=interrupted.init();assert((result==ResetResult::Blocked)==!interrupted.ready);
    preserved(before,disk);assertClosed(disk);disk.disarm();auto reboot=storeFor(disk);
    const auto retry=reboot.init();assert(retry==ResetResult::Ready||retry==ResetResult::ResetPerformed);
    resetComplete(before,disk);assertClosed(disk);++faultChecks;
  }
}

static uint32_t safeRollSeed(){for(uint32_t seed=1;;++seed){Game probe;probe.randomState=seed;if(1+random(probe)%20==20)return seed;}}
static void dailyJournalCuts(){
  auto game=hero(0);game.p.xp=dndXp[19];levelUp(game);game.p.hp=game.p.maxhp;game.p.mp=game.p.maxmp;
  game.tutorial=true;game.rations=9;game.charts=2;game.charms=2;game.sleepKit=true;
  std::vector<std::pair<Game,Game>> transitions;
  game.randomState=safeRollSeed();auto before=game;assert(!prepareDailyTrip(game,3));transitions.push_back({before,game});
  assert(acceptDailyRoad(game));game.randomState=safeRollSeed();before=game;assert(finishDailyLeg(game));transitions.push_back({before,game});
  before=game;assert(acceptDailyNight(game));transitions.push_back({before,game});
  assert(finishDailyRest(game));before=game;assert(continueDailyTrip(game));transitions.push_back({before,game});
  unsigned dailyBoundaries=0,dailyFaults=0;
  for(unsigned slot=0;slot<3;++slot)for(const auto& step:transitions){
    FakeNvsDisk seeded;auto setup=storeFor(seeded);assert(setup.init()==ResetResult::ResetPerformed);
    Backend output(setup);Game loaded;
    for(unsigned other=0;other<3;++other){output.slot=other;Journal29 j(output);assert(j.load(loaded)==Load::Empty&&j.save(other==slot?step.first:hero(other)));}
    const auto image=seeded.image;seeded.disarm();
    auto reference=storeFor(seeded);assert(reference.init()==ResetResult::Ready);
    Backend base(reference);base.slot=slot;Journal29 write(base);assert(write.load(loaded)==Load::Ok);seeded.disarm();
    assert(write.save(step.second));const int count=seeded.boundary;const auto calls=seeded.calls;
    auto verify=[&](FakeNvsDisk& disk){
      disk.disarm();auto reboot=storeFor(disk);assert(reboot.init()==ResetResult::Ready);Backend backend(reboot);backend.slot=slot;Journal29 read(backend);
      const auto result=read.load(loaded);assert(result==Load::Ok||result==Load::Recovered);
      assert(packed(loaded,slot)==packed(step.first,slot)||packed(loaded,slot)==packed(step.second,slot));
      for(unsigned other=0;other<3;++other)if(other!=slot){backend.slot=other;Journal29 unchanged(backend);assert(unchanged.load(loaded)==Load::Ok&&packed(loaded,other)==packed(hero(other),other));}
      assert(epochIs(disk,ResetPhase::Done));assertClosed(disk);
    };
    for(int cut=0;cut<count;++cut){
      FakeNvsDisk disk;disk.image=image;auto store=storeFor(disk);assert(store.init()==ResetResult::Ready);
      Backend backend(store);backend.slot=slot;Journal29 j(backend);assert(j.load(loaded)==Load::Ok);disk.disarm();disk.cut=cut;
      try{(void)j.save(step.second);}catch(const PowerLoss&){}
      verify(disk);++dailyBoundaries;
    }
    for(unsigned index=0;index<calls.size();++index)for(Fault fault:{Fault::Io,Fault::LostAck,Fault::NoEffect}){
      FakeNvsDisk disk;disk.image=image;auto store=storeFor(disk);assert(store.init()==ResetResult::Ready);
      Backend backend(store);backend.slot=slot;Journal29 j(backend);assert(j.load(loaded)==Load::Ok);disk.disarm();disk.faultAt=int(index);disk.fault=fault;
      (void)j.save(step.second);disk.disarm();
      // A committed write whose acknowledgment was lost is reconciled once.
      assert(j.save(step.second));Journal29 retry(backend);assert(retry.load(loaded)==Load::Ok&&packed(loaded,slot)==packed(step.second,slot));
      verify(disk);++dailyFaults;
    }
  }
  printf("Save29 daily production adapter: %u save power-cut boundaries, %u SDK fault/retry checks; three-slot ration/roll/recovery checkpoints passed\n",dailyBoundaries,dailyFaults);
}

int main(){
  dailyJournalCuts();
  bootCreateSaveReload();exactReadsAndProtection();protectedMarkersAndFamily();
  commitAndCleanupFailures();journalProtectionAndLifecycle();resetCutsAndErrors();
  printf("Save29 production NVS adapter: %u checks, %u reset power-cut boundaries, %u SDK I/O faults passed\n",checks,cutChecks,faultChecks);
}
