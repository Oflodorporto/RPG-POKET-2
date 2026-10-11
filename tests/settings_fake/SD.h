#pragma once
#include <vector>
#include <map>
#include <cassert>
inline constexpr int FILE_READ=1;
inline std::vector<uint8_t> packBytes;inline bool cardAvailable=true,fileExists=true;inline unsigned filePosition=0,cardEnds=0,closes=0,readCalls=0;inline int badRead=-1;
inline std::map<std::string,std::vector<uint8_t>> extraFiles;inline unsigned currentFiles=0,maxFiles=0,peakFiles=0;
struct SPIClass{};inline SPIClass SPI;
struct File {bool exists=false,dir=false;std::string path;std::vector<std::string> entries;unsigned index=0;
 explicit operator bool()const{return exists;}unsigned size(){return path=="/RPGPOKET/artes.pak"?packBytes.size():extraFiles[path].size();}
 int read(uint8_t* out,unsigned n){if(int(readCalls++)==badRead)return 0;auto& bytes=path=="/RPGPOKET/artes.pak"?packBytes:extraFiles[path];assert(filePosition+n<=bytes.size());memcpy(out,bytes.data()+filePosition,n);filePosition+=n;return n;}
 void close(){if(exists){++closes;--currentFiles;exists=false;}}bool isDirectory(){return dir;}const char* name(){auto pos=path.find_last_of('/');return path.c_str()+(pos==std::string::npos?0:pos+1);}File openNextFile(){if(index>=entries.size()||currentFiles>=maxFiles)return {};auto name=entries[index++];++currentFiles;peakFiles=std::max(peakFiles,currentFiles);return {true,false,name};}};
struct SdFake {
 bool begin(int cs,SPIClass& spi,unsigned hz,const char* mount,unsigned files,bool format){assert(cs==41&&&spi==&SPI&&hz==4000000&&!strcmp(mount,"/sd")&&files>=1&&files<=2&&!format);maxFiles=files;return cardAvailable;}
 bool exists(const char* p){return !strcmp(p,"/RPGPOKET/artes.pak")?fileExists:extraFiles.count(p);}bool remove(const char* p){if(!strcmp(p,"/RPGPOKET/artes.pak")){fileExists=false;return true;}return extraFiles.erase(p)>0;}
 File open(const char* path){assert(!strcmp(path,"/RPGPOKET"));if(extraFiles.empty()||currentFiles>=maxFiles)return {};File f{true,true,path};for(auto& it:extraFiles)f.entries.push_back(it.first);++currentFiles;return f;}
 File open(const char* path,int mode){assert(mode==FILE_READ);if(!exists(path)||currentFiles>=maxFiles)return {};filePosition=readCalls=0;++currentFiles;peakFiles=std::max(peakFiles,currentFiles);return {true,false,path};}
 void end(){assert(!currentFiles);++cardEnds;}
};inline SdFake SD;
