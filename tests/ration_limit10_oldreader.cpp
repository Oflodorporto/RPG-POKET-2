// Compile only against the isolated frozen headers prepared by
// tools/test_ration_limit10_oldreader.py, never the candidate include tree.
#include <Persistence.h>
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
using namespace rpg;
struct OldStore {
 bool ready=true;unsigned writes=0,erases=0;
 std::map<std::string,std::vector<uint8_t>> blobs;
 Read read(const char* key,uint8_t* out,unsigned size){auto it=blobs.find(key);if(it==blobs.end())return Read::Missing;if(it->second.size()!=size)return Read::Corrupt;memcpy(out,it->second.data(),size);return Read::Ok;}
 bool write(const char* key,const uint8_t* data,unsigned size){++writes;blobs[key]={data,data+size};return true;}
 Read presence(const char* key){return blobs.count(key)?Read::Ok:Read::Missing;}
 bool erase(const char* key){++erases;blobs.erase(key);return true;}
};
int main(){
 static_assert(CURRENT_SAVE_FORMAT==30&&PERSISTENT_SAVE_SIZE==160&&SAVE_RESET_EPOCH==1,"Exact old format/epoch");
 unsigned checks=0;
 for(unsigned slot=0;slot<3;++slot)for(unsigned format:{29u,30u}){
  auto g=create(slot,123+slot);g.rations=9;g.p.gold=1000;assert(persistentValid(g));
  auto ten=g;ten.rations=10;assert(!persistentValid(ten));
  std::vector<uint8_t> nineBytes(PERSISTENT_SAVE_SIZE),tenBytes(PERSISTENT_SAVE_SIZE),marker(SLOT_LIFECYCLE_SIZE);
  encodePersistentVersion(g,41,slot,nineBytes.data(),format);encodePersistentVersion(ten,42,slot,tenBytes.data(),format);
  // The new value requires changing only the existing byte and its CRC.
  auto expected=nineBytes;put32(expected.data(),8,42);expected[84]=10;put32(expected.data(),156,crc(expected.data(),156));assert(tenBytes==expected);
  Game restored=create(3,99);const auto untouched=restored;uint32_t seq=99;
  assert(decodePersistent(nineBytes.data(),slot,restored,seq)==Decode::Ok&&restored.rations==9&&seq==41);
  restored=untouched;seq=99;assert(decodePersistent(tenBytes.data(),slot,restored,seq)==Decode::Corrupt&&restored.rations==untouched.rations&&restored.randomState==untouched.randomState&&seq==99);
  OldStore store;PersistentSlotBackend<OldStore> io(store);io.slot=slot;encodeSlotLife(slot,SlotLife::Live,marker.data());store.blobs[io.metaKey()]=marker;
  store.blobs[io.key(false)]=nineBytes;store.blobs[io.key(true)]=tenBytes;auto before=store.blobs;
  PersistentJournal<PersistentSlotBackend<OldStore>> older(io);
  assert(older.load(restored)==Load::Recovered&&restored.rations==9&&older.seq==41);
  assert(store.blobs==before&&!store.writes&&!store.erases);
  store.blobs[io.key(false)]=tenBytes;before=store.blobs;
  PersistentJournal<PersistentSlotBackend<OldStore>> blocked(io);
  assert(blocked.load(restored)==Load::Blocked&&!blocked.save(g));
  assert(store.blobs==before&&!store.writes&&!store.erases);++checks;
 }
 printf("PASS frozen old reader: %u format/slot cases; 9 valid, 10 rejected; mixed journal recovers older9, both10 blocked; zero writes/erases. No downgrade compatibility.\n",checks);
}
