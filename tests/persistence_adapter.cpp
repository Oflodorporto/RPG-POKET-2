#include "TestHero.h"
#include "../firmware/RPG_POKET_2/NvsRecord.h"
#include "../firmware/RPG_POKET_2/Slots.h"
#include <map>
#include <string>
#include <vector>
#include <cassert>
#include <cstdio>
using namespace rpg;
struct Prefs {std::map<std::string,std::vector<uint8_t>> data;std::string ioFault;bool isKey(const char* k){return data.count(k);}size_t getBytesLength(const char* k){return data[k].size();}size_t getBytes(const char* k,void* out,size_t n){if(ioFault==k)return 0;assert(n==data[k].size());memcpy(out,data[k].data(),n);return n;}};
struct Store {Prefs prefs;bool ready=true;bool tomb[3]{};bool fail=false;Read read(const char* k,uint8_t* out){return readNvsRecord(prefs,ready,k,out);}bool write(const char* k,const uint8_t* b){if(fail)return false;prefs.data[k]={b,b+SAVE_SIZE};return true;}bool deleted(unsigned i){return tomb[i];}bool mark(unsigned i,bool v){tomb[i]=v;return true;}bool erase(const char* k){prefs.data.erase(k);return true;}};
std::vector<uint8_t> blob(const Game& g,unsigned seq){std::vector<uint8_t> b(SAVE_SIZE);encode(g,seq,b.data());return b;}
int main(){
 Store s;SlotBackend<Store> io(s);Game loaded;
 for(unsigned slot=0;slot<3;++slot){io.slot=slot;auto original=testHero(slot,42+slot);original.p.gold=1000+slot;original.rations=slot+1;Journal<SlotBackend<Store>> j(io);assert(j.load(loaded)==Load::Empty&&j.save(original));auto key=io.key(false);auto saved=s.prefs.data[key];
  for(size_t n:{0u,1u,63u,65u,95u,97u,127u}){s.prefs.data[io.key(true)]=std::vector<uint8_t>(n,0x55);Journal<SlotBackend<Store>> recovery(io);assert(recovery.load(loaded)==Load::Recovered&&blob(loaded,1)==saved);assert(s.prefs.data[key]==saved);}
  s.prefs.data[io.key(true)]=std::vector<uint8_t>(256,0x55);Journal<SlotBackend<Store>> oversized(io);assert(oversized.load(loaded)==Load::Blocked&&!oversized.save(original)&&s.prefs.data[key]==saved);
  auto bad=saved;put16(bad.data(),6,96);s.prefs.data[io.key(true)]=bad;Journal<SlotBackend<Store>> header(io);assert(header.load(loaded)==Load::Recovered&&blob(loaded,1)==saved);
  bad=saved;put16(bad.data(),4,CURRENT_SAVE_FORMAT+1);s.prefs.data[io.key(true)]=bad;Journal<SlotBackend<Store>> future(io);assert(future.load(loaded)==Load::Blocked&&!future.save(original)&&s.prefs.data[key]==saved);
  s.prefs.data[io.key(true)]=saved;s.prefs.ioFault=io.key(true);Journal<SlotBackend<Store>> transport(io);assert(transport.load(loaded)==Load::Blocked&&!transport.save(original));s.prefs.ioFault="";
  bad=saved;put16(bad.data(),6,96);put32(bad.data(),124,crc(bad.data(),124));s.prefs.data[io.key(true)]=bad;Journal<SlotBackend<Store>> wrongSize(io);assert(wrongSize.load(loaded)==Load::Recovered);auto copies=s.prefs.data;s.fail=true;assert(!wrongSize.save(original)&&s.prefs.data==copies);s.fail=false;
 }
 // Both corrupt copies remain protected; explicit deletion is the sole opt-in replacement path.
 io.slot=2;s.prefs.data[io.key(false)]={1};s.prefs.data[io.key(true)]={2};auto before=s.prefs.data;Journal<SlotBackend<Store>> blocked(io);assert(blocked.load(loaded)==Load::Blocked&&!blocked.save(testHero(0,1))&&s.prefs.data==before);
 // Corrupted input, including CRC-valid field mutations: safe decode and byte-stable accepted states.
 auto g=testHero(0,7);auto clean=blob(g,21);for(unsigned off=0;off<SAVE_SIZE;++off)for(unsigned value=0;value<256;value+=17){auto b=clean;b[off]=value;if(off>=8&&off<124)put32(b.data(),124,crc(b.data(),124));uint32_t seq=0;Game h;auto d=decode(b.data(),h,seq);if(d==Decode::Ok){assert(valid(h));auto next=blob(h,seq);Game again;assert(decode(next.data(),again,seq)==Decode::Ok&&blob(again,seq)==next);}}
 puts("PASS: actual NVS reader/3 slots; malformed lengths/headers recover intact copy; future versions and I/O faults remain protected; both corrupt copies and failed writes unchanged;2048 CRC-aware decode mutations");
}
