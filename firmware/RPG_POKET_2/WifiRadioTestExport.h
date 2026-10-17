#pragma once
#include "WifiRadioTestCsv.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

namespace wifi_radio_test {
enum class ExportError:uint8_t {None,Empty,Mount,Names,Open,Encode,Write,Flush,Readback};
struct ExportResult {ExportError error=ExportError::None;uint32_t bytes=0,crc=0;char path[48]{};};
inline uint32_t csvCrc(uint32_t crc,const uint8_t* bytes,size_t n){
 for(size_t i=0;i<n;++i){crc^=bytes[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&uint32_t(-int(crc&1)));}return crc;
}
// Main-loop only, including the fixed workspace. Never called during a radio
// measurement. Storage must preserve existing files and never format a card.
inline char exportWorkspace[4096]{};
template<class Storage>ExportResult exportReports(Storage& storage,const Report*const*reports,size_t count,const CsvContext& context,uint32_t nonce){
 ExportResult result;
 if(!count||count>kMaxReports+1){result.error=ExportError::Empty;return result;}
 if(!storage.mount()){result.error=ExportError::Mount;return result;}
 bool opened=false;uint32_t crc=~0u;
 auto finish=[&](ExportError error){if(opened)storage.close();storage.unmount();result.error=error;result.crc=~crc;return result;};
 bool chosen=false;
 for(unsigned attempt=0;attempt<16;++attempt){std::snprintf(result.path,sizeof(result.path),"/RPGPOKET/radio-%08lx.csv",static_cast<unsigned long>(nonce+attempt));
  if(!storage.exists(result.path)){chosen=true;break;}}
 if(!chosen)return finish(ExportError::Names);
 if(!storage.openNew(result.path))return finish(ExportError::Open);
 opened=true;
 auto write=[&](size_t n){if(!n||n>=sizeof(exportWorkspace))return ExportError::Encode;
  if(storage.write(reinterpret_cast<const uint8_t*>(exportWorkspace),n)!=n)return ExportError::Write;
  crc=csvCrc(crc,reinterpret_cast<const uint8_t*>(exportWorkspace),n);result.bytes+=uint32_t(n);return ExportError::None;};
 auto error=write(csvHeader(exportWorkspace,sizeof(exportWorkspace)));
 if(error!=ExportError::None)return finish(error);
 for(size_t i=0;i<count;++i){if(!reports[i])return finish(ExportError::Encode);
  error=write(csvRow(*reports[i],context,exportWorkspace,sizeof(exportWorkspace)));
  if(error!=ExportError::None)return finish(error);}
 if(!storage.flushClose()){opened=false;return finish(ExportError::Flush);}opened=false;
 if(!storage.verify(result.path,result.bytes,~crc))return finish(ExportError::Readback);
 return finish(ExportError::None);
}
inline uint32_t reportFingerprint(const Report*const*reports,size_t count,const CsvContext&context){
 uint32_t crc=~0u;for(size_t i=0;i<count;++i){size_t n=reports[i]?csvRow(*reports[i],context,exportWorkspace,sizeof(exportWorkspace)):0;
  if(!n)return 0;
  crc=csvCrc(crc,reinterpret_cast<const uint8_t*>(exportWorkspace),n);}return ~crc;
}
}
