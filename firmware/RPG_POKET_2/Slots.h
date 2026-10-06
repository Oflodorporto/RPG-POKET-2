#pragma once
#include "Save.h"
// Store provides raw read/write, marker and erase. Tombstone commits deletion
// before cleanup; crashes cannot revive a partially removed character.
template<class Store> struct SlotBackend {
  Store& store;uint8_t slot=0;explicit SlotBackend(Store& s):store(s){}
  const char* key(bool b)const{static const char* keys[3][2]={{"a","b"},{"a1","b1"},{"a2","b2"}};return keys[slot<3?slot:0][b];}
  rpg::Read read(const char* k,uint8_t* b){if(slot>2||!store.ready)return rpg::Read::Error;if(store.deleted(slot))return rpg::Read::Missing;return store.read(key(*k=='b'),b);}
  bool write(const char* k,const uint8_t* b){if(slot>2||!store.ready)return false;
    if(store.deleted(slot)){
      if(!store.erase(key(false))||!store.erase(key(true))||!store.write(key(*k=='b'),b))return false;
      uint8_t check[rpg::SAVE_SIZE];if(store.read(key(*k=='b'),check)!=rpg::Read::Ok||memcmp(b,check,rpg::SAVE_SIZE))return false;
      return store.mark(slot,false);
    }return store.write(key(*k=='b'),b);
  }
  bool remove(){if(slot>2||!store.ready||!store.mark(slot,true))return false;store.erase(key(false));store.erase(key(true));return true;}
};
