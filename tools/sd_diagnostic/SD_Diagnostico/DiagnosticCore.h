#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
namespace diag {
constexpr uint32_t FILE_BYTES=6661456, PAYLOAD_BYTES=6661440, CRC=0x0b84d93f, ASSETS=636;
constexpr const char* SHA="5416f6d507601bc131751e89106332f291f8d1d09642faa264a9b61215dd43f1";
constexpr uint32_t TAIL=6660112, LAST_SECTOR=6661120, CHECKPOINT=1048576;
constexpr size_t LOG_CAP=24576;
struct Report {char data[LOG_CAP]={};size_t used=0;bool truncated=false;
 void append(const char* s){size_t n=strlen(s);if(n>LOG_CAP-1-used){n=LOG_CAP-1-used;truncated=true;}memcpy(data+used,s,n);used+=n;data[used]=0;}
};
struct Session {bool storage=false,network=false,download=false,report=false,probe=false;
 bool take(char c){if(c=='P'){if(storage||network||download||report||probe)return false;probe=true;return true;}if(probe)return false;bool* p=c=='4'||c=='1'?&storage:c=='N'?&network:c=='D'?&download:c=='S'?&report:nullptr;if(!p||*p)return false;*p=true;return true;}
};
inline bool diagnosticTarget(const char* p){constexpr const char* prefix="/RPGDIAG_";constexpr const char* suffix="/download.pak";constexpr size_t start=9,hex=16; if(!p||strlen(p)!=start+hex+strlen(suffix)||strncmp(p,prefix,start)||strcmp(p+start+hex,suffix))return false;for(size_t i=start;i<start+hex;++i)if(!((p[i]>='0'&&p[i]<='9')||(p[i]>='a'&&p[i]<='f')))return false;return true;}
inline bool radioMatches(bool on,unsigned mode,bool connected){return on?(mode==1&&connected):(mode==0&&!connected);}
inline bool commandAllowed(bool privateFailure,char c){return !privateFailure||c=='R'||c=='H';}
enum class InputResult {Pending,Done,Invalid};
struct PrivateInput {char* out;size_t cap,used=0;bool invalid=false;
 InputResult feed(int c){if(invalid)return InputResult::Invalid;if(c=='\r')return InputResult::Pending;if(c=='\n'){out[used]=0;return InputResult::Done;}if(c>=32&&c!=127){if(used+1>=cap){memset(out,0,cap);invalid=true;return InputResult::Invalid;}out[used++]=char(c);}return InputResult::Pending;}
};
struct Read {size_t bytes=0;int error=0;};
struct Result {bool ok=false;const char* cause="not-run";uint32_t offset=0,requested=0,received=0;int error=0;char sha[65]={};};
inline uint32_t le32(const uint8_t* b){return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
inline bool validHeader(const uint8_t* h){return !memcmp(h,"PKA1",4)&&le32(h+4)==PAYLOAD_BYTES&&le32(h+8)==CRC&&le32(h+12)==ASSETS;}
inline bool safeHttps(const char* s){constexpr const char* host="https://raw.githubusercontent.com/";return !strncmp(s,host,strlen(host));}
inline bool exactWrite(size_t wanted,size_t got,int error){return wanted==got&&error==0;}
// One request per block: retain first short/error; no silent retry or acceptance.
// Reader size/read, Hasher begin/update/finish, Progress(offset) controls time/display.
template<class Reader,class Hasher,class Progress>
Result verify(Reader& r,Hasher& hash,uint8_t* buffer,Progress progress,uint32_t bytes=FILE_BYTES,const char* expected=SHA,bool header=true){
 Result out;if(r.size()!=bytes){out.cause="size";out.requested=bytes;out.received=r.size();return out;}
 if(!hash.begin()){out.cause="hash-begin";return out;}
 uint32_t offset=0;
 while(offset<bytes){
  if(!progress(offset)){out.cause="time-limit";out.offset=offset;return out;}
  uint32_t n=header&&offset==0?16:std::min<uint32_t>(4096,bytes-offset);
  Read v=r.read(buffer,n);if(v.bytes!=n||v.error){out.cause="read";out.offset=offset;out.requested=n;out.received=uint32_t(v.bytes);out.error=v.error;return out;}
  if(header&&offset==0&&!validHeader(buffer)){out.cause="header";return out;}
  if(!hash.update(buffer,n)){out.cause="hash-update";out.offset=offset;return out;}offset+=n;
 }
 if(!hash.finish(out.sha)){out.cause="hash-finish";return out;}
 out.offset=bytes;out.ok=!strcmp(out.sha,expected);out.cause=out.ok?"OK":"SHA-mismatch";return out;
}
}
