#pragma once
#include "../firmware/RPG_POKET_2/Persistence.h"
// Controller harness: already-authorized/initialized fresh namespace. The real
// reset and typed ESP-IDF adapter are covered by persistence29_adapter/reset29.
struct Store {bool ready=true,fail=false;std::map<std::string,std::vector<uint8_t>> blobs;
 rpg::Read read(const char* k,uint8_t* out,unsigned n){if(!ready)return rpg::Read::Error;auto i=blobs.find(k);if(i==blobs.end())return rpg::Read::Missing;if(i->second.size()!=n)return rpg::Read::Corrupt;memcpy(out,i->second.data(),n);return rpg::Read::Ok;}
 bool write(const char* k,const uint8_t* b,unsigned n){if(!ready||fail)return false;blobs[k]={b,b+n};return true;}
 rpg::Read presence(const char* k){return !ready?rpg::Read::Error:blobs.count(k)?rpg::Read::Ok:rpg::Read::Missing;}
 bool erase(const char* k){if(!ready||fail)return false;blobs.erase(k);return true;}
 bool live(unsigned s){uint8_t b[16];rpg::SlotLife life;char k[3]={'m',char('0'+s),0};return read(k,b,16)==rpg::Read::Ok&&rpg::decodeSlotLife(b,s,life)&&life==rpg::SlotLife::Live;}
 uint32_t readGuide(unsigned s){uint8_t b[4];char k[3]={'h',char('0'+s),0};return live(s)&&read(k,b,4)==rpg::Read::Ok?rpg::get32(b,0):0;}
 bool writeGuide(unsigned s,uint32_t v){uint8_t b[4];rpg::put32(b,0,v);char k[3]={'h',char('0'+s),0};return live(s)&&write(k,b,4);}
};
Store nvs;
using NvsBackend=PersistentSlotBackend<Store>;NvsBackend backend(nvs);
using NvsJournal=rpg::PersistentJournal<NvsBackend>;
