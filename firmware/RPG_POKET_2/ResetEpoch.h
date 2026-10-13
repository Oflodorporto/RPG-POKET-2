#pragma once
#include "Save.h"

namespace rpg {
// This epoch is independent of firmware build numbers and save-format revisions.
// The Store must keep the marker and new save family separate from legacy keys.
constexpr unsigned RESET_EPOCH_SIZE=16;
constexpr uint8_t RESET_EPOCH_VERSION=1;
constexpr unsigned RESET_EPOCH_VALUE_OFFSET=8;
constexpr unsigned RESET_EPOCH_CRC_OFFSET=12;
constexpr char RESET_EPOCH_MAGIC[]="RST1";
enum class ResetPhase : uint8_t {Pending=1,Done=2};
enum class ResetResult {Ready,ResetPerformed,Blocked};
constexpr char RESET_LEGACY_KEYS[][4]={
  "a","b","a1","b1","a2","b2",
  "fa","fb","fa1","fb1","fa2","fb2","h0","h1","h2"
};
constexpr unsigned RESET_LEGACY_KEY_COUNT=sizeof(RESET_LEGACY_KEYS)/sizeof(RESET_LEGACY_KEYS[0]);

inline void encodeResetEpoch(ResetPhase phase,uint8_t* bytes){
  memset(bytes,0,RESET_EPOCH_SIZE);
  memcpy(bytes,RESET_EPOCH_MAGIC,4);
  bytes[4]=RESET_EPOCH_VERSION;
  bytes[5]=uint8_t(phase);
  put32(bytes,RESET_EPOCH_VALUE_OFFSET,SAVE_RESET_EPOCH);
  put32(bytes,RESET_EPOCH_CRC_OFFSET,crc(bytes,RESET_EPOCH_CRC_OFFSET));
}
inline bool decodeResetEpoch(const uint8_t* bytes,ResetPhase& phase){
  if(memcmp(bytes,RESET_EPOCH_MAGIC,4)||bytes[4]!=RESET_EPOCH_VERSION||
     bytes[6]||bytes[7]||get32(bytes,RESET_EPOCH_VALUE_OFFSET)!=SAVE_RESET_EPOCH||
     get32(bytes,RESET_EPOCH_CRC_OFFSET)!=crc(bytes,RESET_EPOCH_CRC_OFFSET)||
     (bytes[5]!=uint8_t(ResetPhase::Pending)&&bytes[5]!=uint8_t(ResetPhase::Done)))return false;
  phase=ResetPhase(bytes[5]);
  return true;
}

// Call with exclusive access before loading slots or allowing hero creation.
// Reads must distinguish a genuinely missing key from corruption or I/O errors.
// newFamilyPresence() is Missing ONLY when all new record, metadata and hint keys
// are absent. legacyPresence() inspects the raw key, ignoring old tombstones.
// No erase/write operation below may touch the new family, Wi-Fi, SD or art.
template<class Store> ResetResult ensureResetEpoch(Store& store){
  uint8_t bytes[RESET_EPOCH_SIZE],check[RESET_EPOCH_SIZE];
  const Read marker=store.readEpoch(bytes);
  ResetPhase phase=ResetPhase::Pending;
  if(marker==Read::Ok){
    if(!decodeResetEpoch(bytes,phase))return ResetResult::Blocked;
    // Never repeat cleanup after Done, even if old keys reappear on downgrade.
    if(phase==ResetPhase::Done)return ResetResult::Ready;
  }else if(marker!=Read::Missing)return ResetResult::Blocked;

  if(store.newFamilyPresence()!=Read::Missing)return ResetResult::Blocked;
  if(marker==Read::Missing){
    encodeResetEpoch(ResetPhase::Pending,bytes);
    // A failed acknowledgement may have committed. Exact readback is decisive.
    store.writeEpoch(bytes);
    if(store.readEpoch(check)!=Read::Ok||memcmp(bytes,check,RESET_EPOCH_SIZE))return ResetResult::Blocked;
  }

  // Commit and verify every tombstone before erasing even the first old blob.
  // Previously verified persisted effects are not blindly repeated on resume.
  for(uint8_t slot=0;slot<3;++slot){
    bool deleted=false;
    const Read result=store.readLegacyDeleted(slot,deleted);
    if(result!=Read::Ok&&result!=Read::Missing)return ResetResult::Blocked;
    if(result!=Read::Ok||!deleted){
      store.writeLegacyDeleted(slot);
      if(store.readLegacyDeleted(slot,deleted)!=Read::Ok||!deleted)return ResetResult::Blocked;
    }
  }
  for(unsigned i=0;i<RESET_LEGACY_KEY_COUNT;++i){
    const char* key=RESET_LEGACY_KEYS[i];
    const Read presence=store.legacyPresence(key);
    if(presence==Read::Missing)continue;
    if(presence!=Read::Ok)return ResetResult::Blocked;
    store.eraseLegacy(key);
    if(store.legacyPresence(key)!=Read::Missing)return ResetResult::Blocked;
  }
  encodeResetEpoch(ResetPhase::Done,bytes);
  store.writeEpoch(bytes);
  if(store.readEpoch(check)!=Read::Ok||memcmp(bytes,check,RESET_EPOCH_SIZE))return ResetResult::Blocked;
  return ResetResult::ResetPerformed;
}
}
