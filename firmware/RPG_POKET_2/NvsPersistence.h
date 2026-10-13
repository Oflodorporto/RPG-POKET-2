#pragma once
#include "Persistence.h"
#include "ResetEpoch.h"
#include <stddef.h>

// Driver results deliberately distinguish absence from transport and type errors.
// The injectable driver has the same key/commit semantics as ESP-IDF NVS below.
enum class NvsStatus {Ok,Missing,TypeMismatch,Error};

template<class Driver> struct NvsPersistenceStore {
  Driver driver;
  bool ready=false;
  NvsPersistenceStore()=default;
  explicit NvsPersistenceStore(const Driver& value):driver(value){}
  NvsPersistenceStore(const NvsPersistenceStore&)=delete;
  NvsPersistenceStore& operator=(const NvsPersistenceStore&)=delete;

  rpg::ResetResult init(){
    ready=false;
    const auto result=rpg::ensureResetEpoch(*this);
    ready=result==rpg::ResetResult::Ready||result==rpg::ResetResult::ResetPerformed;
    return result;
  }
  rpg::Read read(const char* key,uint8_t* out,unsigned expected){
    return ready&&newKey(key)?readBlob(freshNamespace(),key,out,expected):rpg::Read::Error;
  }
  bool write(const char* key,const uint8_t* bytes,unsigned length){
    return ready&&newKey(key)&&writeBlob(freshNamespace(),key,bytes,length);
  }
  rpg::Read presence(const char* key){
    return ready&&newKey(key)?rawPresence(freshNamespace(),key):rpg::Read::Error;
  }
  bool erase(const char* key){return ready&&newKey(key)&&eraseKey(freshNamespace(),key);}
  uint32_t readGuide(unsigned slot){
    if(!guideLive(slot))return 0;
    uint8_t bytes[4];
    return read(guideKey(slot),bytes,sizeof(bytes))==rpg::Read::Ok?rpg::get32(bytes,0):0;
  }
  bool writeGuide(unsigned slot,uint32_t value){
    if(!guideLive(slot))return false;
    uint8_t bytes[4];rpg::put32(bytes,0,value);
    return write(guideKey(slot),bytes,sizeof(bytes));
  }

  // Only the reset barrier uses these ungated operations. Legacy namespace
  // access is lazy: a completed reset never opens or depends on pkt2_slice.
  rpg::Read readEpoch(uint8_t* out){
    return readBlob(freshNamespace(),"epoch",out,rpg::RESET_EPOCH_SIZE);
  }
  bool writeEpoch(const uint8_t* bytes){
    rpg::ResetPhase phase;
    return bytes&&rpg::decodeResetEpoch(bytes,phase)&&
      writeBlob(freshNamespace(),"epoch",bytes,rpg::RESET_EPOCH_SIZE);
  }
  rpg::Read newFamilyPresence(){
    bool found=false;
    for(const char* key:newKeys()){
      const auto result=rawPresence(freshNamespace(),key);
      if(result==rpg::Read::Error)return result;
      if(result!=rpg::Read::Missing)found=true;
    }
    return found?rpg::Read::Ok:rpg::Read::Missing;
  }
  rpg::Read readLegacyDeleted(uint8_t slot,bool& deleted){
    if(slot>2)return rpg::Read::Error;
    Access access(driver,legacyNamespace(),false);
    if(access.status!=NvsStatus::Ok)return absentOrError(access.status);
    uint8_t value=0;
    const auto result=driver.getU8(access.handle,tombKey(slot),value);
    if(result!=NvsStatus::Ok)return absentOrError(result);
    if(value>1)return rpg::Read::Corrupt;
    deleted=value!=0;return rpg::Read::Ok;
  }
  bool writeLegacyDeleted(uint8_t slot){
    if(slot>2)return false;
    Access access(driver,legacyNamespace(),true);
    return access.status==NvsStatus::Ok&&
      driver.setU8(access.handle,tombKey(slot),1)==NvsStatus::Ok&&
      driver.commit(access.handle)==NvsStatus::Ok;
  }
  rpg::Read legacyPresence(const char* key){
    return legacyKey(key)?rawPresence(legacyNamespace(),key):rpg::Read::Error;
  }
  bool eraseLegacy(const char* key){return legacyKey(key)&&eraseKey(legacyNamespace(),key);}

private:
  struct KeyList {
    const char* values[12]={"a","b","a1","b1","a2","b2","m0","m1","m2","h0","h1","h2"};
    const char* const* begin()const{return values;}
    const char* const* end()const{return values+12;}
  };
  static KeyList newKeys(){return KeyList{};}
  static const char* freshNamespace(){return "pkt2_v29";}
  static const char* legacyNamespace(){return "pkt2_slice";}
  static const char* guideKey(unsigned slot){static const char* keys[]={"h0","h1","h2"};return keys[slot];}
  static const char* metaKey(unsigned slot){static const char* keys[]={"m0","m1","m2"};return keys[slot];}
  static const char* tombKey(unsigned slot){static const char* keys[]={"t0","t1","t2"};return keys[slot];}
  static bool newKey(const char* key){
    if(!key)return false;
    for(const char* allowed:newKeys())if(!strcmp(key,allowed))return true;
    return false;
  }
  static bool legacyKey(const char* key){
    if(!key)return false;
    for(const auto& allowed:rpg::RESET_LEGACY_KEYS)if(!strcmp(key,allowed))return true;
    return false;
  }
  static rpg::Read absentOrError(NvsStatus result){
    return result==NvsStatus::Missing?rpg::Read::Missing:rpg::Read::Error;
  }
  bool guideLive(unsigned slot){
    if(!ready||slot>2)return false;
    uint8_t bytes[rpg::SLOT_LIFECYCLE_SIZE];rpg::SlotLife life;
    return read(metaKey(slot),bytes,sizeof(bytes))==rpg::Read::Ok&&
      rpg::decodeSlotLife(bytes,uint8_t(slot),life)&&life==rpg::SlotLife::Live;
  }
  // Each readback gets a fresh handle after the mutation's commit and close.
  // An uncommitted per-handle cache can never acknowledge the reset barrier.
  struct Access {
    Driver& driver;
    typename Driver::Handle handle{};
    NvsStatus status;
    Access(Driver& d,const char* name,bool write):driver(d),status(driver.open(name,write,handle)){}
    ~Access(){if(status==NvsStatus::Ok)driver.close(handle);}
    Access(const Access&)=delete;
    Access& operator=(const Access&)=delete;
  };
  rpg::Read rawPresence(const char* name,const char* key){
    Access access(driver,name,false);
    if(access.status!=NvsStatus::Ok)return absentOrError(access.status);
    size_t length=0;
    const auto result=driver.getBlob(access.handle,key,nullptr,length);
    // get_blob's TYPE_MISMATCH proves that a key exists, even for unknown types.
    if(result==NvsStatus::Ok||result==NvsStatus::TypeMismatch)return rpg::Read::Ok;
    return absentOrError(result);
  }
  rpg::Read readBlob(const char* name,const char* key,uint8_t* out,unsigned expected){
    if(!out||!expected)return rpg::Read::Error;
    Access access(driver,name,false);
    if(access.status!=NvsStatus::Ok)return absentOrError(access.status);
    size_t length=0;
    auto result=driver.getBlob(access.handle,key,nullptr,length);
    if(result!=NvsStatus::Ok)return absentOrError(result);
    // A larger record may belong to future firmware and must be protected.
    if(length>expected)return rpg::Read::Error;
    if(length<expected)return rpg::Read::Corrupt;
    result=driver.getBlob(access.handle,key,out,length);
    // A disappearance, truncation or I/O failure after the size probe is not
    // absence. Never let it open a creation/replacement path.
    return result==NvsStatus::Ok&&length==expected?rpg::Read::Ok:rpg::Read::Error;
  }
  bool writeBlob(const char* name,const char* key,const uint8_t* bytes,unsigned length){
    if(!bytes||!length)return false;
    Access access(driver,name,true);
    return access.status==NvsStatus::Ok&&
      driver.setBlob(access.handle,key,bytes,length)==NvsStatus::Ok&&
      driver.commit(access.handle)==NvsStatus::Ok;
  }
  bool eraseKey(const char* name,const char* key){
    // Probe through the read-only namespace first: deleting an absent key must
    // not create a namespace. Wrong key types are still eligible for cleanup.
    const auto presence=rawPresence(name,key);
    if(presence==rpg::Read::Missing)return true;
    if(presence!=rpg::Read::Ok)return false;
    Access access(driver,name,true);
    if(access.status!=NvsStatus::Ok)return false;
    const auto result=driver.eraseKey(access.handle,key);
    if(result==NvsStatus::Missing)return true;
    return result==NvsStatus::Ok&&driver.commit(access.handle)==NvsStatus::Ok;
  }
};

// Host tests instantiate these same aliases with an SDK-semantics fake driver.
template<class Driver> using NvsStoreFor=NvsPersistenceStore<Driver>;
template<class Driver> using NvsBackendFor=PersistentSlotBackend<NvsStoreFor<Driver>>;
template<class Driver> using NvsJournalFor=rpg::PersistentJournal<NvsBackendFor<Driver>>;

#ifdef ARDUINO
#include <nvs.h>
struct Esp32NvsDriver {
  using Handle=nvs_handle_t;
  static NvsStatus status(esp_err_t error){
    if(error==ESP_OK)return NvsStatus::Ok;
    if(error==ESP_ERR_NVS_NOT_FOUND)return NvsStatus::Missing;
    if(error==ESP_ERR_NVS_TYPE_MISMATCH)return NvsStatus::TypeMismatch;
    return NvsStatus::Error;
  }
  NvsStatus open(const char* name,bool write,Handle& handle){return status(nvs_open(name,write?NVS_READWRITE:NVS_READONLY,&handle));}
  void close(Handle handle){nvs_close(handle);}
  NvsStatus getBlob(Handle handle,const char* key,void* out,size_t& length){return status(nvs_get_blob(handle,key,out,&length));}
  NvsStatus getU8(Handle handle,const char* key,uint8_t& value){return status(nvs_get_u8(handle,key,&value));}
  NvsStatus setBlob(Handle handle,const char* key,const void* bytes,size_t length){return status(nvs_set_blob(handle,key,bytes,length));}
  NvsStatus setU8(Handle handle,const char* key,uint8_t value){return status(nvs_set_u8(handle,key,value));}
  NvsStatus eraseKey(Handle handle,const char* key){return status(nvs_erase_key(handle,key));}
  NvsStatus commit(Handle handle){return status(nvs_commit(handle));}
};
using NvsStore=NvsStoreFor<Esp32NvsDriver>;
using NvsBackend=NvsBackendFor<Esp32NvsDriver>;
using NvsJournal=NvsJournalFor<Esp32NvsDriver>;
#endif
