#pragma once
#include <vector>
#include <cassert>
inline constexpr int FILE_READ=1;
inline std::vector<uint8_t> packBytes;inline bool cardAvailable=true,fileExists=true;inline unsigned filePosition=0,cardEnds=0,closes=0,readCalls=0;inline int badRead=-1;
struct SPIClass{};inline SPIClass SPI;
struct File {bool exists=false;explicit operator bool()const{return exists;}unsigned size(){return packBytes.size();}int read(uint8_t* out,unsigned n){if(int(readCalls++)==badRead)return 0;assert(filePosition+n<=packBytes.size());memcpy(out,packBytes.data()+filePosition,n);filePosition+=n;return n;}void close(){++closes;} bool isDirectory(){return false;} const char* name(){return "";} File openNextFile(){return {}; }};
struct SdFake {
 bool begin(int cs,SPIClass& spi,unsigned hz,const char* mount,unsigned files,bool format){assert(cs==41&&&spi==&SPI&&hz==4000000&&!strcmp(mount,"/sd")&&files==1&&!format);return cardAvailable;}
 bool exists(const char*){return false;} bool remove(const char*){return true;} File open(const char* path){assert(!strcmp(path,"/RPGPOKET"));return {};}
 File open(const char* path,int mode){assert(!strcmp(path,"/RPGPOKET/artes.pak")&&mode==FILE_READ);filePosition=0;readCalls=0;return {fileExists};}
 void end(){++cardEnds;}
};inline SdFake SD;
