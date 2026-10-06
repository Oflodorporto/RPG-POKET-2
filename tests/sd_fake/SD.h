#pragma once
#include "Arduino.h"
#include "SPI.h"
#include <cstring>
constexpr const char* FILE_READ="r";
constexpr int CARD_NONE=0,CARD_SDHC=3;
inline bool sdAvailable=true,probeExists=true,probeDirectory=false;
inline size_t probeOffset=0;
inline unsigned sdEnds=0,sdOpens=0,fileCloses=0;
struct File {
  bool opened=false;
  explicit operator bool()const{return opened;}
  bool isDirectory(){return probeDirectory;}
  size_t size(){return 4096;}
  size_t read(uint8_t* out,size_t count){assert(opened);for(size_t i=0;i<count;++i)out[i]=uint8_t((probeOffset+i)*73+19);probeOffset+=count;return count;}
  void close(){assert(opened);opened=false;++fileCloses;}
};
struct FakeSD {
  void end(){++sdEnds;}
  bool begin(uint8_t cs,SPIClass& spi,uint32_t freq,const char* path,uint8_t files,bool format){
    assert(cs==41&&&spi==&SPI&&freq==4000000&&!strcmp(path,"/sd")&&files==1&&!format&&pin41==HIGH&&pin45==HIGH);return sdAvailable;
  }
  int cardType(){return sdAvailable?CARD_SDHC:CARD_NONE;}
  uint64_t cardSize(){return 4000000000ull;}
  File open(const char* path,const char* mode){assert(!strcmp(path,"/RPGPOKET/teste.bin")&&!strcmp(mode,"r"));++sdOpens;probeOffset=0;return File{probeExists};}
};
inline FakeSD SD;
