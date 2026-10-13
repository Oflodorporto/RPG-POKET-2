#pragma once
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <deque>
#include <cerrno>
struct String {std::string value;String(const char* s=""):value(s){}String(std::string s):value(s){}const char* c_str()const{return value.c_str();}bool startsWith(const char* s)const{return value.rfind(s,0)==0;}};
inline uint32_t fakeNow=0;inline uint32_t millis(){return fakeNow;}
constexpr int HIGH=1,WL_CONNECTED=3;inline void digitalWrite(int pin,int state){assert((pin==41||pin==45)&&state==HIGH);}
struct WifiFake {int state=WL_CONNECTED;bool sleep=true;int status(){return state;}int RSSI(){return -60;}void reconnect(){state=WL_CONNECTED;}bool getSleep(){return sleep;}void setSleep(bool s){sleep=s;}};inline WifiFake WiFi;
inline void configTzTime(const char*,const char*,const char*){}
using SemaphoreHandle_t=int*;inline int mutexToken=1;constexpr unsigned portMAX_DELAY=~0u;inline SemaphoreHandle_t xSemaphoreCreateMutex(){return &mutexToken;}inline void xSemaphoreTake(SemaphoreHandle_t s,unsigned){assert(s);}inline void xSemaphoreGive(SemaphoreHandle_t s){assert(s);}inline void vTaskDelay(unsigned n){fakeNow+=n;}inline unsigned pdMS_TO_TICKS(unsigned n){return n;}inline void vTaskDelete(void*){}constexpr int pdPASS=1;inline int taskResult=pdPASS;inline int xTaskCreate(void(*)(void*),const char*,unsigned,void*,int,void*){return taskResult;}
struct SPIClass{};inline SPIClass SPI;
constexpr int FILE_READ=1,FILE_WRITE=2,FILE_APPEND=3;
inline std::map<std::string,std::vector<uint8_t>> disk;inline bool card=true,writeFault=false,renameFault=false,readFault=false;inline unsigned openFiles=0,maxFiles=0;inline std::vector<std::string> deleted;
inline std::string shortWritePath;inline size_t shortWriteAt=~size_t(0),shortWriteBytes=0;inline int shortWriteErrno=0;
// Opt-in read-back/flush failures model accepted writes that cannot be verified.
inline std::string readBackFaultPath,flushFaultPath;
inline size_t readBackFaultAt=~size_t(0),readBackBytes=0,readBackFlipAt=~size_t(0),flushTrim=0;
inline bool otaOpen=false,hashFinishFaultOnOta=false,copyFlipOnOta=false;inline std::string renameFailTo;
inline std::string lookupFaultPath,openMetaFaultPath;inline int lookupErrno=EIO,openMetaErrno=EIO;
inline int readBackErrno=0,readBackFailures=-1;inline unsigned lastSdHz=0,sdMounts=0;inline bool recoverOnSlow=false;inline std::string renameFailFrom;
inline bool readBackOpenFail=false;inline int reportedSizeDelta=0;
struct File {std::string path;size_t pos=0;bool opened=false;explicit operator bool()const{return opened;}unsigned size(){if(path==openMetaFaultPath)return 0;return disk[path].size()+(path==readBackFaultPath?reportedSizeDelta:0);}int read(uint8_t* out,unsigned n){if(readFault)return 0;auto& b=disk[path];n=std::min<size_t>(n,b.size()-std::min(pos,b.size()));if(path==readBackFaultPath&&pos>=readBackFaultAt&&readBackFailures!=0&&!(recoverOnSlow&&lastSdHz==1000000)){n=std::min<size_t>(n,readBackBytes);errno=readBackErrno;if(readBackFailures>0)--readBackFailures;}
 memcpy(out,b.data()+pos,n);if(copyFlipOnOta&&otaOpen&&pos<=100&&pos+n>100)out[100-pos]^=1;if(path==readBackFaultPath&&readBackFlipAt>=pos&&readBackFlipAt<pos+n)out[readBackFlipAt-pos]^=1;pos+=n;return n;}size_t write(const uint8_t* in,size_t n){if(writeFault)return 0;auto& b=disk[path];if(path==shortWritePath&&b.size()>=shortWriteAt){size_t wrote=std::min(n,shortWriteBytes);b.insert(b.end(),in,in+wrote);pos+=wrote;errno=shortWriteErrno;return wrote;}b.insert(b.end(),in,in+n);pos+=n;return n;}void flush(){if(path==flushFaultPath&&flushTrim){auto& b=disk[path];b.resize(b.size()-std::min(flushTrim,b.size()));}}void close(){if(opened){opened=false;--openFiles;}}};
struct SdFake {uint64_t freeBytes=1ull<<30;bool begin(int cs,SPIClass& spi,unsigned hz,const char* mount,unsigned files,bool format){assert(cs==41&&&spi==&SPI&&(hz==4000000||hz==1000000)&&!strcmp(mount,"/sd")&&!format);maxFiles=files;lastSdHz=hz;++sdMounts;return card;}void end(){assert(!openFiles);}bool exists(const char* p){if(p==lookupFaultPath&&!(recoverOnSlow&&lastSdHz==1000000)){errno=lookupErrno;return false;}bool found=disk.count(p);if(!found)errno=ENOENT;return found;}bool mkdir(const char*){return true;}uint64_t totalBytes(){return freeBytes+100;}uint64_t usedBytes(){return 100;}File open(const char* p,int mode){if(mode==FILE_READ&&readBackOpenFail&&p==readBackFaultPath){errno=readBackErrno;return {};}if(openFiles>=maxFiles)return {};if(mode==FILE_READ&&!disk.count(p)){errno=ENOENT;return {};}if(mode==FILE_WRITE)disk[p].clear();if(p==openMetaFaultPath)errno=openMetaErrno;++openFiles;return {p,mode==FILE_APPEND?disk[p].size():0,true};}bool remove(const char* p){deleted.push_back(p);return disk.erase(p)>0;}bool rename(const char* a,const char* b){if(renameFault||renameFailFrom==a||renameFailTo==b||!disk.count(a)||disk.count(b))return false;disk[b]=disk[a];disk.erase(a);return true;}};inline SdFake SD;
struct NetworkClientSecure {void useBuiltinCACertBundle(){}void setHandshakeTimeout(unsigned n){assert(n==25);}};
struct Reply {int rc=200;std::vector<uint8_t> data;int length=-2;std::string range,location;size_t dropAt=~size_t(0);};
inline std::deque<Reply> replies;inline std::vector<std::string> ranges,urls;constexpr int HTTPC_DISABLE_FOLLOW_REDIRECTS=0;
struct HTTPClient {Reply reply;size_t pos=0;std::string range;bool begin(NetworkClientSecure&,String s){urls.push_back(s.value);return true;}void setConnectTimeout(int){}void setTimeout(int){}void setFollowRedirects(int){}void collectHeaders(const char**,int){}void addHeader(const char*,const char* s){range=s;}int GET(){ranges.push_back(range);assert(!replies.empty());reply=replies.front();replies.pop_front();pos=0;return reply.rc;}int getSize(){return reply.length==-2?reply.data.size():reply.length;}String header(const char* key){return String(!strcmp(key,"Location")?reply.location:reply.range);}HTTPClient* getStreamPtr(){return this;}int available(){return pos>=reply.dropAt?0:std::min<size_t>(reply.data.size()-pos,reply.dropAt-pos);}int read(uint8_t* b,size_t n){n=std::min<size_t>(n,available());memcpy(b,reply.data.data()+pos,n);pos+=n;return n;}bool connected(){return pos<reply.data.size()&&pos<reply.dropAt;}void end(){}};
#ifdef RPG_TEST_REAL_SHA256
// Optional host-only cryptographic backend: the installer still calls its real
// streaming mbedTLS-shaped API; bytes are hashed by installed OpenSSL SHA-256.
#include <openssl/sha.h>
struct mbedtls_sha256_context {SHA256_CTX value;};
inline void mbedtls_sha256_init(mbedtls_sha256_context* c){SHA256_Init(&c->value);}
inline int mbedtls_sha256_starts(mbedtls_sha256_context* c,int){return SHA256_Init(&c->value)==1?0:-1;}
inline int mbedtls_sha256_update(mbedtls_sha256_context* c,const uint8_t* b,size_t n){return SHA256_Update(&c->value,b,n)==1?0:-1;}
inline int mbedtls_sha256_finish(mbedtls_sha256_context* c,uint8_t* b){if(hashFinishFaultOnOta&&otaOpen)return -55;return SHA256_Final(b,&c->value)==1?0:-1;}
inline void mbedtls_sha256_free(mbedtls_sha256_context*){}
#else
// Test digest is deterministic; verifies hash checks/call ordering, not mbedTLS cryptography.
struct mbedtls_sha256_context {uint32_t h=2166136261u;};inline void mbedtls_sha256_init(mbedtls_sha256_context* c){c->h=2166136261u;}inline int mbedtls_sha256_starts(mbedtls_sha256_context* c,int){mbedtls_sha256_init(c);return 0;}inline int mbedtls_sha256_update(mbedtls_sha256_context* c,const uint8_t* b,size_t n){for(size_t i=0;i<n;++i)c->h=(c->h^b[i])*16777619u;return 0;}inline int mbedtls_sha256_finish(mbedtls_sha256_context* c,uint8_t* b){for(unsigned i=0;i<32;++i)b[i]=uint8_t((c->h>>(i%4*8))^i);return 0;}inline void mbedtls_sha256_free(mbedtls_sha256_context*){}
#endif
constexpr int ESP_OK=0;using esp_ota_handle_t=unsigned;struct esp_partition_t {uint32_t size=0x300000;};inline esp_partition_t target;inline bool beginFault=false,otaWriteFault=false,endFault=false,bootFault=false;inline unsigned beginCalls=0,bootCalls=0,abortCalls=0,endCalls=0;inline std::vector<uint8_t> flashed;
inline const esp_partition_t* esp_ota_get_next_update_partition(void*){return &target;}inline int esp_ota_begin(const esp_partition_t* p,unsigned bytes,unsigned* h){++beginCalls;assert(bytes<=p->size);if(beginFault)return -1;*h=1;otaOpen=true;flashed.clear();return ESP_OK;}inline int esp_ota_write(unsigned,const uint8_t* b,size_t n){assert(otaOpen);if(otaWriteFault)return -1;flashed.insert(flashed.end(),b,b+n);return ESP_OK;}inline int esp_ota_end(unsigned){assert(otaOpen);otaOpen=false;++endCalls;return endFault?-1:ESP_OK;}inline int esp_ota_abort(unsigned){assert(otaOpen);otaOpen=false;++abortCalls;return ESP_OK;}inline int esp_ota_set_boot_partition(const esp_partition_t*){assert(!otaOpen);++bootCalls;return bootFault?-1:ESP_OK;}
struct EspFake {bool restarted=false;uint32_t getPsramSize(){return 8*1048576;}void restart(){restarted=true;}};inline EspFake ESP;
