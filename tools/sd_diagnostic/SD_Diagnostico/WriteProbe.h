#pragma once
#include "DiagnosticCore.h"
namespace diag {
constexpr const char* PREFIX_SHA="7cdb705f9a9be23511a919cde2e8455f8abde9c8f9abed1de218ad54f8e4a554";
constexpr char PATTERN[]="RPG-POKET-SD-DIAGNOSTIC-0123456789-ABCDEFGHIJKLMNOPQRSTUVWXYZ-abc\n";
inline void fillPrefix(uint8_t* out,size_t n,uint32_t offset){for(size_t i=0;i<n;++i)out[i]=uint8_t(PATTERN[(offset+i)%(sizeof(PATTERN)-1)]);}
struct Write {int32_t bytes=0;int error=0;};
struct WriteResult {bool ok=false;const char* cause="not-run";uint32_t offset=0,requested=0;int32_t written=0;int error=0;};
template<class Writer,class Progress>
WriteResult writePrefix(Writer& writer,uint8_t* buffer,Progress progress){
 WriteResult r;
 while(r.offset<CHECKPOINT){
  if(!progress(r.offset)){r.cause="time-limit";return r;}
  r.requested=std::min<uint32_t>(4096,CHECKPOINT-r.offset);fillPrefix(buffer,r.requested,r.offset);
  Write w=writer.write(buffer,r.requested);r.written=w.bytes;r.error=w.error;
  if(w.bytes<0||!exactWrite(r.requested,size_t(w.bytes),w.error)){r.cause="write";return r;}
  r.offset+=r.requested;
 }
 r.ok=true;r.cause="OK";return r;
}
// Actual W sequencing: a failed local control or incomplete transfer stops B.
// A read error in B is retained while the socket/radio transitions are observed.
template<class IO> bool writeComparison(IO& io){
 if(!io.local())return false;
 if(!io.receive())return false;
 io.readOpen();io.closeHttp();io.readClosed();io.radioOff();io.readOff();return true;
}
}
