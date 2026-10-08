#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Slots.h"
#include "../firmware/RPG_POKET_2/World.h"
#include "../firmware/RPG_POKET_2/AssetStore.h"
#include "../firmware/RPG_POKET_2/CombatFx.h"
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>
#include <cstdlib>
struct Store {
  bool ready=true;std::map<std::string,std::vector<uint8_t>> blobs;bool tomb[3]={};int fail=-1,operations=0;
  bool step(){return operations++!=fail;}
  bool deleted(unsigned i){return tomb[i];}
  bool mark(unsigned i,bool b){if(!step())return false;tomb[i]=b;return true;}
  bool erase(const char* k){if(!step())return false;blobs.erase(k);return true;}
  rpg::Read read(const char* k,uint8_t* b){if(!step())return rpg::Read::Error;auto it=blobs.find(k);if(it==blobs.end())return rpg::Read::Missing;memcpy(b,it->second.data(),rpg::SAVE_SIZE);return rpg::Read::Ok;}
  bool write(const char* k,const uint8_t* b){if(!step())return false;blobs[k]=std::vector<uint8_t>(b,b+rpg::SAVE_SIZE);return true;}
};
struct Reader {std::vector<uint8_t> bytes;unsigned pos=0;int fail=-1,calls=0;unsigned size(){return bytes.size();}int read(uint8_t* out,unsigned n){if(calls++==fail)return 0;if(pos+n>bytes.size())return 0;memcpy(out,bytes.data()+pos,n);pos+=n;return n;}};
int main(){
  Store base;SlotBackend<Store> io(base);rpg::Game g;
  for(unsigned i=0;i<3;++i){io.slot=i;rpg::Journal<SlotBackend<Store>> j(io);assert(j.load(g)==rpg::Load::Empty);g=rpg::testHero(i,17+i);g.p.gold=100+i;assert(j.save(g));assert(j.save(g));}
  auto slot0a=base.blobs["a"],slot0b=base.blobs["b"],slot2a=base.blobs["a2"],slot2b=base.blobs["b2"];
  // Every interruption in delete cleanup: marker failure preserves, success never revives.
  for(int fail=0;fail<3;++fail){Store s=base;s.fail=fail;s.operations=0;SlotBackend<Store> b(s);b.slot=1;bool deleted=b.remove();s.fail=-1;
    rpg::Journal<SlotBackend<Store>> j(b);auto loaded=j.load(g);assert(loaded==(deleted?rpg::Load::Empty:rpg::Load::Ok));assert(s.blobs["a"]==slot0a&&s.blobs["b"]==slot0b&&s.blobs["a2"]==slot2a&&s.blobs["b2"]==slot2b);
    if(deleted)assert(s.tomb[1]);}
  // Every interrupted recreation either stays empty or contains only the new hero.
  for(int fail=0;fail<9;++fail){Store s=base;SlotBackend<Store> b(s);b.slot=1;assert(b.remove());rpg::Journal<SlotBackend<Store>> j(b);assert(j.load(g)==rpg::Load::Empty);auto fresh=rpg::testHero(3,98);fresh.p.gold=7;s.fail=fail;s.operations=0;bool ok=j.save(fresh);s.fail=-1;
    rpg::Journal<SlotBackend<Store>> resumed(b);auto loaded=resumed.load(g);assert(loaded==rpg::Load::Empty||loaded==rpg::Load::Ok);if(loaded==rpg::Load::Ok)assert(g.p.cls==3&&g.p.gold==7);if(ok)assert(loaded==rpg::Load::Ok);
    assert(s.blobs["a"]==slot0a&&s.blobs["b"]==slot0b&&s.blobs["a2"]==slot2a&&s.blobs["b2"]==slot2b);}
  uint8_t bytes[rpg::SAVE_SIZE];uint32_t seq=0;
  for(unsigned city=0;city<4;++city){g=rpg::testHero(0,32);g.city=city;rpg::encode(g,18,bytes);rpg::Game loaded;assert(rpg::decode(bytes,loaded,seq)==rpg::Decode::Ok&&loaded.city==city);}
  // Existing save5 is upgraded without resetting equipment, mission or player.
  g=rpg::testHero(2,77);g.p.gold=123;assert(!rpg::acceptQuest(g,2));g.questProgress=1;rpg::encode(g,9,bytes);rpg::put16(bytes,4,5);rpg::put16(bytes,6,64);rpg::put32(bytes,60,rpg::crc(bytes,60));rpg::Game old;assert(rpg::decode(bytes,old,seq)==rpg::Decode::Ok&&old.city==0&&old.questProgress==1&&old.p.gold==123&&seq==9);
  bytes[53]|=0x40;rpg::put32(bytes,60,rpg::crc(bytes,60));assert(rpg::decode(bytes,old,seq)==rpg::Decode::Corrupt);
  for(uint8_t a=0;a<4;++a)for(uint8_t b=0;b<4;++b){Journey j;assert(j.start(a,b,UINT32_MAX-500)==(a!=b));if(a==b)continue;assert(j.position().x==places[a].x);int previous=0;
    for(unsigned t=0;t<=3000;t+=10){j.tick(uint32_t(UINT32_MAX-500+t));Point p=j.position();assert(p.x>=0&&p.x<240&&p.y>=40&&p.y<225&&int(j.progress)>=previous);previous=j.progress;}assert(!j.active&&j.position().x==places[b].x&&j.position().y==places[b].y);}
  Journey invalid;assert(!invalid.start(0,4,0));CombatFx fx;fx.start(Effect::Lightning,false,100);assert(!fx.expire(1299)&&fx.frame(1299)==7);assert(fx.expire(1300));
  artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);makeFallback();std::vector<uint8_t> fallback(artMemory,artMemory+ART_BYTES);
  std::ifstream file("outputs/RPG_POKET_2_0_Waveshare/cartao_preparado/RPGPOKET/artes.pak",std::ios::binary);if(!file)file.open("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(file);Reader r;r.bytes=std::vector<uint8_t>(std::istreambuf_iterator<char>(file),{});assert(readArt(r)==ArtStatus::Ready);
  for(unsigned off:{0u,4u,8u,12u,32u,ART_BYTES-1}){Reader bad;bad.bytes=r.bytes;bad.bytes[off]^=1;makeFallback();assert(readArt(bad)==ArtStatus::Invalid);assert(!memcmp(artMemory,fallback.data(),ART_BYTES));}
  Reader truncated;truncated.bytes=r.bytes;truncated.bytes.pop_back();assert(readArt(truncated)==ArtStatus::Invalid);
  for(int fail:{1,10,200}){Reader bad;bad.bytes=r.bytes;bad.fail=fail;makeFallback();assert(readArt(bad)==ArtStatus::Invalid);assert(!memcmp(artMemory,fallback.data(),ART_BYTES));}
  free(artMemory);puts("PASS: 3 isolated slots, delete/recreation interruptions, save5->7, all world routes/wraparound, 1200ms lightning, SD art CRC/read-error fallback.");
}
