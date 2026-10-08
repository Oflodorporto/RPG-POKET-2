#include "TestHero.h"
#include "../firmware/RPG_POKET_2/CardCheck.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
// The production controller uses the same read-only interface as this fault fixture.
struct Reader {
  bool available=true,exists=true,opened=false,mounted=false;
  size_t length=CARD_PROBE_SIZE,position=0,reads=0,failRead=99;
  int corrupt=-1;unsigned mounts=0,opens=0,closes=0,unmounts=0;
  bool mount(){++mounts;mounted=available;return mounted;}
  uint32_t capacityMiB(){assert(mounted);return 3815;}
  bool openProbe(){assert(mounted);++opens;opened=exists;position=0;return opened;}
  size_t size(){assert(opened);return length;}
  size_t read(uint8_t* dst,size_t count){
    assert(opened&&mounted&&count==512&&position+count<=length);size_t n=reads++==failRead?count-1:count;
    for(size_t i=0;i<n;++i)dst[i]=uint8_t((position+i)*73+19)^uint8_t(int(position+i)==corrupt?1:0);
    position+=n;return n;
  }
  void closeProbe(){++closes;opened=false;}
  void unmount(){++unmounts;mounted=false;}
};
int main(){
  Reader io;auto state=checkCard(io);assert(state.status==CardStatus::Verified&&state.capacityMiB==3815&&io.reads==8&&!io.opened&&!io.mounted&&io.closes==1&&io.unmounts==1);
  io=Reader{};io.available=false;state=checkCard(io);assert(state.status==CardStatus::Unavailable&&!state.capacityMiB&&!io.opens&&!io.reads&&io.unmounts==1);
  io=Reader{};io.exists=false;state=checkCard(io);assert(state.status==CardStatus::Missing&&state.capacityMiB==3815&&!io.reads&&io.closes==1&&!io.mounted);
  for(size_t bad:{size_t(0),size_t(4095),size_t(4097),size_t(65535)}){io=Reader{};io.length=bad;state=checkCard(io);assert(state.status==CardStatus::WrongFile&&!io.reads&&!io.opened&&!io.mounted);}
  for(size_t chunk=0;chunk<8;++chunk){io=Reader{};io.failRead=chunk;state=checkCard(io);assert(state.status==CardStatus::ReadError&&io.reads==chunk+1&&!io.opened&&!io.mounted);}
  for(int bad:{0,511,512,4095}){io=Reader{};io.corrupt=bad;state=checkCard(io);assert(state.status==CardStatus::WrongFile&&io.reads==8&&!io.opened&&!io.mounted);}
  io=Reader{};io.exists=false;assert(checkCard(io).status==CardStatus::Missing);io.exists=true;assert(checkCard(io).status==CardStatus::Verified&&io.mounts==2&&io.unmounts==2);
  const uint8_t known[]={'1','2','3','4','5','6','7','8','9'};assert(~cardCrcUpdate(~0u,known,9)==0xcbf43926u);
  assert(!strcmp(CARD_PROBE_PATH,"/RPGPOKET/teste.bin"));
  puts("PASS: optional read-only4096-byte card check; absent card/file, exact size, eight short-read faults, corrupt content, retry, CRC32 reference, bounded512-byte reads, close/unmount on all paths.");
}
