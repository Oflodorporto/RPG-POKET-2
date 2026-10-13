// Compiled unchanged twice against real old/new catalog fixtures by test_enemy_transition.py.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "AssetStore.h"
#include "SpriteArt.h"
#include "PersonalArt.h"

// Hardware entry points used by the unchanged DeviceSettings adapter.
constexpr int OUTPUT=1,HIGH=1,MALLOC_CAP_SPIRAM=1,MALLOC_CAP_8BIT=2;
constexpr uint16_t UI_INK=0x1083,UI_GOLD=0xd5aa,UI_WHITE=0xef1b,UI_MUTED=0x9cf4,
  UI_GREEN=0x6e0c,UI_RED=0xe28b,UI_BLUE=0x549f;
uint32_t millis(){return 0;}void delay(unsigned){}
void pinMode(int pin,int mode){assert(pin==41&&mode==OUTPUT);}
void digitalWrite(int pin,int value){assert((pin==41||pin==45)&&value==HIGH);}
bool ledcAttach(int pin,int hz,int bits){assert(pin==1&&hz==5000&&bits==8);return true;}
void ledcWrite(int pin,unsigned value){assert(pin==1&&value<=255);}
void* heap_caps_malloc(unsigned n,int flags){assert(flags==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return malloc(n);}
void heap_caps_free(void* p){free(p);}
#include "DeviceSettings.h"

struct MemoryReader {
  const std::vector<uint8_t>& data;
  unsigned position=0,calls=0;
  int shortCall=-1;
  unsigned size()const{return data.size();}
  int read(uint8_t* out,unsigned requested){
    unsigned n=std::min<unsigned>(requested,data.size()-position);
    if(int(calls++)==shortCall&&n)--n;
    memcpy(out,data.data()+position,n);position+=n;return n;
  }
};
std::vector<uint8_t> load(const char* path){std::ifstream stream(path,std::ios::binary);assert(stream);return {std::istreambuf_iterator<char>(stream),{}};}

int main(int argc,char** argv){
  assert(argc==4);
  const auto own=load(argv[1]),other=load(argv[2]);
  assert(own.size()==ART_BYTES+16&&rpg::get32(own.data(),8)==ART_CRC);
  assert(other.size()!=own.size()&&rpg::get32(other.data(),8)!=ART_CRC);
  assert(rpg::CURRENT_SAVE_FORMAT==28);
  auto game=rpg::create(0,0x7129u);
  std::array<uint8_t,rpg::SAVE_SIZE> save{},check{};
  rpg::encode(game,27,save.data());
  const std::vector<uint8_t> saveFile(save.begin(),save.end());
  constexpr unsigned guard=64;
  std::vector<uint8_t> guarded(ART_BYTES+2*guard,0xa5);
  artMemory=guarded.data()+guard;
  makeFallback();
  const std::vector<uint8_t> fallback(artMemory,artMemory+ART_BYTES);
  unsigned gates=0;
  auto verify=[&](bool ready){
    for(unsigned i=0;i<guard;++i)assert(guarded[i]==0xa5&&guarded[guard+ART_BYTES+i]==0xa5);
    if(ready)assert(std::equal(own.begin()+16,own.end(),artMemory));
    else assert(std::equal(fallback.begin(),fallback.end(),artMemory));
    rpg::encode(game,27,check.data());assert(check==save);
    if(extraFiles.count("/RPGPOKET/slot1_a.rpg"))assert(extraFiles.at("/RPGPOKET/slot1_a.rpg")==saveFile);
    ++gates;
  };
  auto read=[&](const std::vector<uint8_t>& bytes,ArtStatus expected,int shortCall=-1){
    makeFallback();MemoryReader reader{bytes,0,0,shortCall};
    assert(readArt(reader)==expected);verify(expected==ArtStatus::Ready);return reader.calls;
  };
  read(own,ArtStatus::Ready);
  assert(read(other,ArtStatus::Invalid)==0); // Reject wrong-sized pack before even its header.
  for(unsigned field:{0u,4u,8u,12u}){auto bad=own;bad[field]^=1;assert(read(bad,ArtStatus::Invalid)==1);}
  read(own,ArtStatus::Invalid,0); // Short header.
  auto physicallyShort=own;physicallyShort.pop_back();assert(read(physicallyShort,ArtStatus::Invalid)==0);
  const unsigned chunks=(ART_BYTES+4095)/4096;
  // Representative first/middle/last payload chunks, not every chunk boundary.
  for(unsigned call:{1u,1u+chunks/2,chunks})read(own,ArtStatus::Invalid,int(call));
  std::vector<unsigned> corruptOffsets={assetMeta[92].offset,ART_BYTES/2,ART_BYTES-1};
  if constexpr(ART_COUNT==636)corruptOffsets={assetMeta[92].offset,assetMeta[544].offset,assetMeta[600].offset};
  for(unsigned offset:corruptOffsets){auto bad=own;bad[16+offset]^=0x80;read(bad,ArtStatus::Invalid);}

  char ownPath[48],otherPath[48];
  updater::artPath(ownPath,sizeof(ownPath),ART_CRC);
  updater::artPath(otherPath,sizeof(otherPath),rpg::get32(other.data(),8));
  char exact[48];snprintf(exact,sizeof(exact),"/RPGPOKET/artes_%08x.pak",ART_CRC);assert(!strcmp(exact,ownPath));
  auto reset=[&](){
    assert(!currentFiles&&!currentDirs);
    cardAvailable=true;fileExists=false;badRead=-1;packBytes.clear();extraFiles.clear();
    extraFiles["/RPGPOKET/slot1_a.rpg"]=saveFile;
    extraFiles["/RPGPOKET/artes.part"]={0x39};
    extraFiles["/RPGPOKET/artes_11111111.part"]={0x41};
    extraFiles["/RPGPOKET/not-ours.pak"]={0x53};
    memset(artMemory,0x77,ART_BYTES); // Every boot must initialize safe fallback.
  };
  auto boot=[&](ArtStatus expected){
    unsigned ends=cardEnds;assert(loadSdArt()==expected);assert(cardEnds==ends+unsigned(!cardAvailable||badRead>=0?2:1));
    assert(!currentFiles&&!currentDirs);assert(extraFiles.at("/RPGPOKET/slot1_a.rpg")==saveFile);
    assert(extraFiles.at("/RPGPOKET/artes.part")==std::vector<uint8_t>{0x39});
    assert(extraFiles.at("/RPGPOKET/artes_11111111.part")==std::vector<uint8_t>{0x41});
    assert(extraFiles.at("/RPGPOKET/not-ours.pak")==std::vector<uint8_t>{0x53});
    verify(expected==ArtStatus::Ready);
  };
  reset();cardAvailable=false;boot(ArtStatus::Missing);
  reset();boot(ArtStatus::Missing);
  reset();fileExists=true;packBytes=own;boot(ArtStatus::Ready);assert(fileExists);
  reset();fileExists=true;packBytes=other;boot(ArtStatus::Invalid);
  reset();extraFiles[otherPath]=other;boot(ArtStatus::Missing);assert(extraFiles.count(otherPath));
  reset();extraFiles[ownPath]=own;extraFiles[otherPath]=other;boot(ArtStatus::Ready);
  assert(extraFiles.count(ownPath)&&extraFiles.count(otherPath)&&!fileExists);
  reset();fileExists=true;packBytes=own;extraFiles[ownPath]=own;extraFiles[ownPath][16+ART_BYTES/2]^=1;extraFiles[otherPath]=other;
  auto filesBefore=extraFiles;boot(ArtStatus::Ready);assert(extraFiles==filesBefore&&fileExists); // Generic recovery must not clean up.
  reset();fileExists=true;packBytes=other;extraFiles[ownPath]=own;extraFiles[ownPath][0]^=1;boot(ArtStatus::Invalid);
  reset();extraFiles[ownPath]=own;extraFiles[ownPath][16+ART_BYTES-1]^=1;boot(ArtStatus::Invalid);
  reset();extraFiles[ownPath]=other;boot(ArtStatus::Invalid);
  for(unsigned call:{1u,1u+chunks/2,chunks}){reset();extraFiles[ownPath]=own;badRead=int(call);boot(ArtStatus::Invalid);}
  reset();fileExists=true;packBytes=own;extraFiles[ownPath]=own;extraFiles[otherPath]=other;extraFiles["/RPGPOKET/artes_00000001.pak"]={1};
  boot(ArtStatus::Ready);assert(fileExists&&extraFiles.count(otherPath)&&extraFiles.count("/RPGPOKET/artes_00000001.pak")&&extraFiles.count(ownPath));
  assert(peakFiles==1);
  // Null art allocation has the existing distinct Memory result, with no SD activity.
  unsigned ends=cardEnds;artMemory=nullptr;assert(loadSdArt()==ArtStatus::Memory&&cardEnds==ends);
  rpg::encode(game,27,check.data());assert(check==save);++gates;
  printf("PASS %s: %u paired artifact gates; actual AssetStore + DeviceSettings; Save28/canaries; representative first/middle/last short reads.\n",argv[3],gates);
  return 0;
}
