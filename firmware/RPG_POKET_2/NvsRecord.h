#pragma once
#include "Save.h"
// Malformed records are not transient I/O errors. Keep the other journal copy readable.
template<class Prefs> rpg::Read readNvsRecord(Prefs& prefs,bool ready,const char* key,uint8_t* out){
 if(!ready)return rpg::Read::Error;
 if(!prefs.isKey(key))return rpg::Read::Missing;
 size_t n=prefs.getBytesLength(key);
 // Oversized records may belong to a future format: never replace them automatically.
 if(n>rpg::SAVE_SIZE)return rpg::Read::Error;
 if(!n)return rpg::Read::Corrupt;
 memset(out,0,rpg::SAVE_SIZE);if(prefs.getBytes(key,out,n)!=n)return rpg::Read::Error;
 if(n>=8&&!memcmp(out,"PKT2",4)&&rpg::get16(out,4)>rpg::CURRENT_SAVE_FORMAT)return rpg::Read::Ok;
 return (n==64||n==96||n==rpg::SAVE_SIZE)&&rpg::get16(out,6)==n?rpg::Read::Ok:rpg::Read::Corrupt;
}
