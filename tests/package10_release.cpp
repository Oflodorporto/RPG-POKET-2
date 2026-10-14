// Exact historical header closures or current package10; real cJSON and SHA-256.
// HTTP/SD/RTOS/flash and persistence stores are host adapters, not hardware.
#define RPG_TEST_REAL_SHA256
#include "Save.h"
#ifndef RPG_TEST_OLD_TOUCH77
#include "Persistence.h"
#include "ResetEpoch.h"
#endif
#include "AssetCatalog.h"
#include "FakeDevice.h"
enum class ArtStatus {Ready,Missing};
struct {bool scanning=false,connecting=false;ArtStatus art=ArtStatus::Ready;} menu;
#include "UpdateDevice.h"
#include <fstream>
#include <functional>

#if defined(RPG_TEST_OLD_SCENES77) || defined(RPG_TEST_OLD_SCALE77) || defined(RPG_TEST_OLD_CAMP77)
#define RPG_TEST_HISTORICAL_SAVE29
#endif
#if defined(RPG_TEST_OLD_EXPLORE77) || defined(RPG_TEST_OLD_IMMERSION77) || defined(RPG_TEST_OLD_STORY77) || defined(RPG_TEST_OLD_CLOCK77)
#define RPG_TEST_HISTORICAL_SAVE30
#endif
constexpr uint32_t PACKAGE10_BUILD=2026101404u;
constexpr const char* PACKAGE10_VERSION="2026.10.14-pvp77";
using Bytes=std::vector<uint8_t>;
static Bytes readFile(const std::string& path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
static void writeFile(const std::string& path,const Bytes& bytes){std::ofstream f(path,std::ios::binary);assert(f);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());assert(f);}
static std::string fileName(unsigned slot,unsigned mode){return "slot"+std::to_string(slot)+"-mode"+std::to_string(mode);}
static std::map<std::string,Bytes> protectedFiles;
#ifndef RPG_TEST_OLD_TOUCH77
// Any reset write/erase or presence lookup after the historical Done marker fails.
struct CompletedEpochStore {
 unsigned reads=0;
 rpg::Read readEpoch(uint8_t* bytes){++reads;const auto& marker=disk.at("/sentinel-reset-epoch");assert(marker.size()==rpg::RESET_EPOCH_SIZE);memcpy(bytes,marker.data(),marker.size());return rpg::Read::Ok;}
 bool writeEpoch(const uint8_t*){assert(false);return false;}
 rpg::Read newFamilyPresence(){assert(false);return rpg::Read::Error;}
 rpg::Read readLegacyDeleted(uint8_t,bool&){assert(false);return rpg::Read::Error;}
 bool writeLegacyDeleted(uint8_t){assert(false);return false;}
 rpg::Read legacyPresence(const char*){assert(false);return rpg::Read::Error;}
 bool eraseLegacy(const char*){assert(false);return false;}
};
static void noRepeatedReset(){CompletedEpochStore store;assert(rpg::ensureResetEpoch(store)==rpg::ResetResult::Ready&&store.reads==1);}
struct Store {
 bool ready=true;unsigned writes=0,erasures=0;std::map<std::string,Bytes> blobs;
 rpg::Read read(const char* key,uint8_t* bytes,unsigned size){auto it=blobs.find(key);if(it==blobs.end())return rpg::Read::Missing;if(it->second.size()!=size)return rpg::Read::Corrupt;memcpy(bytes,it->second.data(),size);return rpg::Read::Ok;}
 bool write(const char* key,const uint8_t* bytes,unsigned size){++writes;blobs[key]={bytes,bytes+size};return true;}
 rpg::Read presence(const char* key){return blobs.count(key)?rpg::Read::Ok:rpg::Read::Missing;}
 bool erase(const char* key){++erasures;blobs.erase(key);return true;}
};
using Backend=PersistentSlotBackend<Store>;
using Journal=rpg::PersistentJournal<Backend>;
static Bytes packed(const rpg::Game& g,uint32_t seq,unsigned slot){Bytes b(rpg::PERSISTENT_SAVE_SIZE);rpg::encodePersistent(g,seq,slot,b.data());return b;}
// The witness is emitted by the unchanged historical gameplay, covering BOTH
// floors, collision/door state, enemy masks and every spawn/pickup location.
static Bytes geometry(const rpg::Game& g){
 Bytes out;
 if(rpg::inDungeon(g)){
  auto view=g;
  for(unsigned floor=0;floor<2;++floor){rpg::dungeonSetFloor(view,floor);for(int y=0;y<9;++y)for(int x=0;x<9;++x){out.push_back(rpg::dungeonCell(view,x,y));out.push_back(rpg::dungeonWall(view,x,y));out.push_back(1+rpg::dungeonEnemyAt(view,x,y));}}
  for(unsigned i=0;i<rpg::dungeonSpawnCount(g);++i){const auto p=rpg::dungeonSpawn(g,i);out.insert(out.end(),{p.floor,p.x,p.y,p.id});}
  for(unsigned i=0;i<rpg::dungeonPickupCount(g);++i){const auto p=rpg::dungeonPickup(g,i);out.insert(out.end(),{p.floor,p.x,p.y,p.id});}
 }
 auto next=g;unsigned offset=out.size();out.resize(offset+4);rpg::put32(out.data(),offset,rpg::random(next));return out;
}
static Store loadStore(const std::string& root,unsigned mode){
 Store s;
 for(unsigned slot=0;slot<3;++slot){Backend b(s);b.slot=slot;s.blobs[b.key(false)]=readFile(root+"/"+fileName(slot,mode)+".bin");Bytes life(rpg::SLOT_LIFECYCLE_SIZE);rpg::encodeSlotLife(slot,rpg::SlotLife::Live,life.data());s.blobs[b.metaKey()]=life;s.blobs[b.guideKey()]={2,3,4,5};}
 s.blobs["epoch"]=readFile(root+"/epoch.bin");s.blobs["wifi"]={'w','i','f','i'};return s;
}
static void sameOtherFiles(const Store& before,const Store& after,const char* changed){
 for(const auto& pair:before.blobs)if(pair.first!=changed)assert(after.blobs.at(pair.first)==pair.second);
 for(const auto& pair:after.blobs)if(pair.first!=changed)assert(before.blobs.at(pair.first)==pair.second);
}
#ifdef RPG_TEST_HISTORICAL_SAVE29
static void historicalFixtures(const std::string& root){
 static_assert(rpg::CURRENT_SAVE_FORMAT==29&&rpg::PERSISTENT_SAVE_FORMAT==29&&rpg::PERSISTENT_SAVE_SIZE==160,"Exact historical Save29 required");
 for(unsigned mode=0;mode<3;++mode)for(unsigned slot=0;slot<3;++slot){
  auto g=rpg::create(slot,8800+slot);g.p.gold=880+slot;g.race=slot;g.shirt=2+slot;g.trousers=7-slot;g.fatigue=30+slot;g.rations=5;g.city=1;g.crystals=3;
  if(mode){assert(!rpg::enterDungeon(g));g.dungeonLoot=8;g.dungeonEnemies=1;rpg::dungeonSetFloor(g,mode-1);g.dungeonXY=mode==1?0x73:0x71;rpg::dungeonFace(g,slot);assert(rpg::inDungeon(g)&&rpg::dungeonFloor(g)==mode-1);}
  assert(rpg::persistentValid(g));const auto save=packed(g,42+slot,slot);assert(rpg::get16(save.data(),4)==29);
  writeFile(root+"/"+fileName(slot,mode)+".bin",save);writeFile(root+"/"+fileName(slot,mode)+".geometry",geometry(g));
  protectedFiles["/sentinel-"+fileName(slot,mode)]=save;
 }
 Bytes marker(rpg::RESET_EPOCH_SIZE);rpg::encodeResetEpoch(rpg::ResetPhase::Done,marker.data());writeFile(root+"/epoch.bin",marker);protectedFiles["/sentinel-reset-epoch"]=marker;
}
static void rollback(const std::string& root){
 unsigned probes=0;
 for(unsigned mode=0;mode<3;++mode){const auto baseline=loadStore(root,mode);
  for(unsigned slot=0;slot<3;++slot)for(unsigned placement=0;placement<3;++placement){
   auto s=baseline;Backend b(s);b.slot=slot;Journal stale(b);rpg::Game old;assert(stale.load(old)==rpg::Load::Ok);const auto untouched=packed(old,77,slot);
   const auto migrated=readFile(root+"/migrated/"+fileName(slot,mode)+".bin");assert(migrated.size()==160&&rpg::get16(migrated.data(),4)==30&&rpg::get32(migrated.data(),156)==rpg::crc(migrated.data(),156));
   uint32_t sequence=0x11223344;assert(rpg::decodePersistent(migrated.data(),slot,old,sequence)==rpg::Decode::Unsupported&&sequence==0x11223344&&packed(old,77,slot)==untouched);
   if(placement==0){s.blobs[b.key(true)]=migrated;}
   if(placement==1){s.blobs[b.key(true)]=s.blobs[b.key(false)];s.blobs[b.key(false)]=migrated;}
   if(placement==2){s.blobs[b.key(false)]=s.blobs[b.key(true)]=migrated;}
   const auto before=s.blobs;assert(!stale.save(old));Journal reboot(b);assert(reboot.load(old)==rpg::Load::Blocked&&reboot.blocked&&reboot.active==-1&&!reboot.save(old));assert(packed(old,77,slot)==untouched&&s.blobs==before&&!s.writes&&!s.erasures);
   for(unsigned other=0;other<3;++other)if(other!=slot){Backend ob(s);ob.slot=other;Journal j(ob);rpg::Game h;assert(j.load(h)==rpg::Load::Ok);}
   ++probes;
  }
 }
 for(unsigned slot=0;slot<3;++slot){auto s=loadStore(root,0);Backend b(s);b.slot=slot;const auto procedural=readFile(root+"/migrated/slot"+std::to_string(slot)+"-procedural.bin");assert(rpg::get16(procedural.data(),4)==30&&procedural[143]==1&&rpg::get32(procedural.data(),148));s.blobs[b.key(true)]=procedural;const auto before=s.blobs;rpg::Game g;uint32_t seq=0;assert(rpg::decodePersistent(procedural.data(),slot,g,seq)==rpg::Decode::Unsupported);Journal j(b);assert(j.load(g)==rpg::Load::Blocked&&!j.save(g)&&s.blobs==before&&!s.writes&&!s.erasures);++probes;}
 protectedFiles["/sentinel-reset-epoch"]=readFile(root+"/epoch.bin");disk=protectedFiles;noRepeatedReset();
 printf("PASS: %u actual migrated/procedural Save30 rollback probes through untouched historical Save29 decoder/journal, either/both sides, all three slots, stale writer, no writes/erase/reset.\n",probes);
}
#else
static void migrateFixtures(const std::string& root){
 static_assert(rpg::CURRENT_SAVE_FORMAT==30&&rpg::PERSISTENT_SAVE_FORMAT==30&&rpg::PERSISTENT_SAVE_SIZE==160,"Real production Save30 required");
 for(unsigned mode=0;mode<3;++mode){auto s=loadStore(root,mode);const auto original=s;
  for(unsigned slot=0;slot<3;++slot){Backend b(s);b.slot=slot;Journal j(b);rpg::Game loaded;const auto before=s;
   assert(j.load(loaded)==rpg::Load::Ok&&j.seq==42+slot&&s.blobs==before.blobs&&s.writes==before.writes&&s.erasures==before.erasures);
   assert(!loaded.dungeonGenerator&&!loaded.dungeonSeed&&!loaded.dungeonRun);
   if(mode)assert(rpg::inDungeon(loaded)&&rpg::dungeonFloor(loaded)==mode-1);
   const auto historical=readFile(root+"/"+fileName(slot,mode)+".bin");Bytes oldFormat(160);rpg::encodePersistentVersion(loaded,j.seq,slot,oldFormat.data(),29);assert(oldFormat==historical);assert(geometry(loaded)==readFile(root+"/"+fileName(slot,mode)+".geometry"));
   // A normal save is the first write; loading and the OTA installer never migrate.
   assert(j.save(loaded)&&j.seq==43+slot&&s.writes==before.writes+1&&!s.erasures);sameOtherFiles(before,s,b.key(true));assert(s.blobs.at(b.key(false))==historical);
   const auto migrated=s.blobs.at(b.key(true));assert(rpg::get16(migrated.data(),4)==30&&migrated==packed(loaded,43+slot,slot));
   Journal reboot(b);rpg::Game restored;assert(reboot.load(restored)==rpg::Load::Ok&&reboot.seq==43+slot&&packed(restored,reboot.seq,slot)==migrated&&geometry(restored)==geometry(loaded));
   rpg::encodePersistentVersion(restored,42+slot,slot,oldFormat.data(),29);assert(oldFormat==historical);writeFile(root+"/migrated/"+fileName(slot,mode)+".bin",migrated);
   // Unopened slots stay on their exact historical bytes until their own save.
   for(unsigned other=slot+1;other<3;++other){Backend ob(s);ob.slot=other;assert(s.blobs.at(ob.key(false))==original.blobs.at(ob.key(false))&&!s.blobs.count(ob.key(true)));}
   protectedFiles["/sentinel-"+fileName(slot,mode)]=historical;
  }
 }
 // These are actual generated runs and the real Save30 encoder, not a version
 // byte patched into a Save29 blob or just an optimistic future manifest.
 for(unsigned slot=0;slot<3;++slot){auto s=loadStore(root,0);Backend b(s);b.slot=slot;Journal j(b);rpg::Game g;assert(j.load(g)==rpg::Load::Ok);const auto before=s;const auto rng=g.randomState;const auto crystals=g.crystals;assert(!rpg::enterDungeon(g)&&g.dungeonGenerator==1&&g.dungeonSeed&&g.dungeonRun==1&&g.randomState==rng&&g.crystals+1==crystals&&rpg::persistentValid(g));assert(j.save(g));sameOtherFiles(before,s,b.key(true));const auto bytes=s.blobs.at(b.key(true));assert(rpg::get16(bytes.data(),4)==30&&bytes[143]==1&&rpg::get32(bytes.data(),148)==g.dungeonSeed&&bytes[153]==1&&!bytes[154]&&!bytes[155]);Journal reboot(b);rpg::Game h;assert(reboot.load(h)==rpg::Load::Ok&&packed(h,j.seq,slot)==bytes);writeFile(root+"/migrated/slot"+std::to_string(slot)+"-procedural.bin",bytes);}
 protectedFiles["/sentinel-reset-epoch"]=readFile(root+"/epoch.bin");
 puts("PASS: actual historical Save29 fixtures, all three slots at home and both active legacy floors; byte-canonical fields/geometry/next RNG preserved; load is read-only; next ordinary journal save writes30 to the other side; genuine generated Save30 identity persists.");
}
#endif

#ifdef RPG_TEST_HISTORICAL_SAVE30
static void historical30Fixtures(const std::string& root){
 static_assert(rpg::CURRENT_SAVE_FORMAT==30&&rpg::PERSISTENT_SAVE_FORMAT==30&&rpg::PERSISTENT_SAVE_SIZE==160,"Exact historical Save30 required");
 for(unsigned mode=0;mode<5;++mode)for(unsigned slot=0;slot<3;++slot){
  auto g=rpg::create(slot,8800+slot);g.p.gold=880+slot;g.race=slot;g.shirt=2+slot;g.trousers=7-slot;g.fatigue=71+slot;g.rations=5;g.city=1;g.crystals=3;
  if(mode){assert(!rpg::enterDungeon(g,mode<3?0:1));g.dungeonLoot=8;g.dungeonEnemies=1;rpg::dungeonSetFloor(g,(mode-1)%2);g.dungeonXY=(mode-1)%2?0x71:0x73;rpg::dungeonFace(g,slot);assert(rpg::inDungeon(g));}
  if(mode>=3)assert(g.dungeonGenerator==1&&g.dungeonSeed&&g.dungeonRun==1);
  else assert(!g.dungeonGenerator&&!g.dungeonSeed&&!g.dungeonRun);
  assert(rpg::persistentValid(g));const auto save=packed(g,42+slot,slot);assert(rpg::get16(save.data(),4)==30);
  writeFile(root+"/"+fileName(slot,mode)+".bin",save);writeFile(root+"/"+fileName(slot,mode)+".geometry",geometry(g));protectedFiles["/sentinel-"+fileName(slot,mode)]=save;
 }
 Bytes marker(rpg::RESET_EPOCH_SIZE);rpg::encodeResetEpoch(rpg::ResetPhase::Done,marker.data());writeFile(root+"/epoch.bin",marker);protectedFiles["/sentinel-reset-epoch"]=marker;
}

static void rollback30(const std::string& root){
#ifdef RPG_TEST_OLD_EXPLORE77
 assert(!strcmp(FW_VERSION,"2026.10.13-explore77")&&FW_BUILD==2026101309u);
#elif defined(RPG_TEST_OLD_CLOCK77)
 assert(!strcmp(FW_VERSION,"2026.10.14-clock77")&&FW_BUILD==2026101403u);
#elif defined(RPG_TEST_OLD_STORY77)
 assert(!strcmp(FW_VERSION,"2026.10.14-story77")&&FW_BUILD==2026101402u);
#else
 assert(!strcmp(FW_VERSION,"2026.10.14-immersion77")&&FW_BUILD==2026101401u);
#endif
 static_assert(rpg::CURRENT_SAVE_FORMAT==30&&rpg::PERSISTENT_SAVE_FORMAT==30,"Exact historical Save30 reader required");
 unsigned probes=0;
 for(unsigned mode=0;mode<5;++mode)for(unsigned slot=0;slot<3;++slot)for(unsigned placement=0;placement<3;++placement){
  auto s=loadStore(root,mode);Backend b(s);b.slot=slot;
  const auto original=s.blobs.at(b.key(false));const auto fromPackage10=readFile(root+"/preserved/"+fileName(slot,mode)+".bin");
  assert(rpg::get16(fromPackage10.data(),4)==30&&rpg::get32(fromPackage10.data(),8)==43+slot);
  if(placement==0)s.blobs[b.key(true)]=fromPackage10;
  if(placement==1){s.blobs[b.key(true)]=original;s.blobs[b.key(false)]=fromPackage10;}
  if(placement==2)s.blobs[b.key(false)]=s.blobs[b.key(true)]=fromPackage10;
  const auto before=s;Journal j(b);rpg::Game g;assert(j.load(g)==rpg::Load::Ok&&j.seq==43+slot&&!j.blocked&&s.blobs==before.blobs&&!s.writes&&!s.erasures);
  assert(packed(g,j.seq,slot)==fromPackage10&&packed(g,42+slot,slot)==original&&geometry(g)==readFile(root+"/"+fileName(slot,mode)+".geometry"));
  if(mode>=3)assert(g.dungeonGenerator==1&&g.dungeonSeed==rpg::get32(fromPackage10.data(),148)&&g.dungeonRun==1);
  assert(g.dungeonLoot==(mode?8:0)&&g.dungeonEnemies==(mode?1:0));
  for(unsigned other=0;other<3;++other)if(other!=slot){Backend ob(s);ob.slot=other;Journal oj(ob);rpg::Game h;assert(oj.load(h)==rpg::Load::Ok&&packed(h,42+other,other)==readFile(root+"/"+fileName(other,mode)+".bin"));}
  assert(s.blobs==before.blobs&&!s.writes&&!s.erasures);
  // Read the new record again through the untouched old journal after reboot.
  // No historical writer is invoked: this is a read-only codec compatibility
  // probe, never a supported OTA firmware-downgrade path.
  Journal reboot(b);rpg::Game restored;assert(reboot.load(restored)==rpg::Load::Ok&&reboot.seq==43+slot&&packed(restored,reboot.seq,slot)==fromPackage10&&geometry(restored)==geometry(g));
  assert(s.blobs==before.blobs&&!s.writes&&!s.erasures);
  ++probes;
 }
 protectedFiles["/sentinel-reset-epoch"]=readFile(root+"/epoch.bin");disk=protectedFiles;noRepeatedReset();
 printf("PASS: %u exact %s Save30 reader/journal read-back probes of newly package10-saved records; three slots/home/legacy/procedural floors, either/both journal sides, seed/run/masks/positions/next RNG unchanged; read-only codec/journal loads with no writes/erase or repeated epoch. This proves save-codec compatibility, not permission to OTA-downgrade firmware.\n",probes,FW_VERSION);
}

#endif
#ifndef RPG_TEST_HISTORICAL_SAVE29
static void preserve30Fixtures(const std::string& root){
 static_assert(rpg::CURRENT_SAVE_FORMAT==30&&rpg::PERSISTENT_SAVE_FORMAT==30&&rpg::PERSISTENT_SAVE_SIZE==160,"Real production Save30 required");
 for(unsigned mode=0;mode<5;++mode){auto s=loadStore(root,mode);const auto original=s;
  for(unsigned slot=0;slot<3;++slot){Backend b(s);b.slot=slot;Journal j(b);rpg::Game loaded;const auto before=s;
   assert(j.load(loaded)==rpg::Load::Ok&&j.seq==42+slot&&s.blobs==before.blobs&&s.writes==before.writes&&s.erasures==before.erasures);
   const auto historical=readFile(root+"/"+fileName(slot,mode)+".bin");assert(packed(loaded,j.seq,slot)==historical&&geometry(loaded)==readFile(root+"/"+fileName(slot,mode)+".geometry"));
   assert(loaded.dungeonLoot==(mode?8:0)&&loaded.dungeonEnemies==(mode?1:0));
   if(mode)assert(rpg::inDungeon(loaded)&&rpg::dungeonFloor(loaded)==(mode-1)%2&&loaded.dungeonXY==((mode-1)%2?0x71:0x73));
   if(mode>=3)assert(loaded.dungeonGenerator==1&&loaded.dungeonSeed==rpg::get32(historical.data(),148)&&loaded.dungeonRun==1);
   else assert(!loaded.dungeonGenerator&&!loaded.dungeonSeed&&!loaded.dungeonRun);
   assert(j.save(loaded)&&j.seq==43+slot&&s.writes==before.writes+1&&!s.erasures);sameOtherFiles(before,s,b.key(true));assert(s.blobs.at(b.key(false))==historical);
   const auto saved=s.blobs.at(b.key(true));assert(saved==packed(loaded,43+slot,slot));auto canonical=saved;rpg::put32(canonical.data(),8,42+slot);rpg::put32(canonical.data(),156,rpg::crc(canonical.data(),156));assert(canonical==historical);
   Journal reboot(b);rpg::Game restored;assert(reboot.load(restored)==rpg::Load::Ok&&reboot.seq==43+slot&&packed(restored,reboot.seq,slot)==saved&&geometry(restored)==geometry(loaded));
   writeFile(root+"/preserved/"+fileName(slot,mode)+".bin",saved);
   for(unsigned other=slot+1;other<3;++other){Backend ob(s);ob.slot=other;assert(s.blobs.at(ob.key(false))==original.blobs.at(ob.key(false))&&!s.blobs.count(ob.key(true)));}
   protectedFiles["/sentinel-"+fileName(slot,mode)]=historical;
  }
 }
 protectedFiles["/sentinel-reset-epoch"]=readFile(root+"/epoch.bin");
 puts("PASS: exact historical Save30 -> package10 Save30, all three slots/home/legacy and generated floors; byte-canonical seed/run/geometry/masks/positions/next RNG preserved; read-only load, ordinary journal save changes sequence and CRC only; no new reset.");
}
#endif

#endif
static std::string change(const std::string& raw,const std::function<void(cJSON*)>& edit){auto j=cJSON_Parse(raw.c_str());assert(j);edit(j);char* text=cJSON_PrintUnformatted(j);assert(text);std::string result=text;cJSON_free(text);cJSON_Delete(j);return result;}
static void setReads(cJSON* j,std::initializer_list<unsigned> formats){auto reads=cJSON_CreateArray();for(auto n:formats)cJSON_AddItemToArray(reads,cJSON_CreateNumber(n));cJSON_ReplaceItemInObject(j,"reads_save_formats",reads);}
static void reset(){disk=protectedFiles;replies.clear();urls.clear();ranges.clear();deleted.clear();flashed.clear();updateWorker={};updateInfo={};updatePlan={};ESP.restarted=false;beginCalls=bootCalls=endCalls=abortCalls=0;otaOpen=false;openFiles=0;maxFiles=1;card=true;writeFault=readFault=renameFault=beginFault=endFault=bootFault=otaWriteFault=false;SD.freeBytes=1ull<<30;WiFi.state=WL_CONNECTED;menu.art=ArtStatus::Ready;}
static void preserved(){for(const auto& p:protectedFiles){assert(disk.at(p.first)==p.second);assert(std::find(deleted.begin(),deleted.end(),p.first)==deleted.end());}assert(!openFiles&&!otaOpen&&replies.empty());
#ifndef RPG_TEST_OLD_TOUCH77
noRepeatedReset();
#endif
}
static void check(const std::string& raw){replies.push_back({200,{raw.begin(),raw.end()}});checkUpdate();}
static void noFlash(){assert(!beginCalls&&!bootCalls&&!ESP.restarted&&flashed.empty());preserved();}
int main(int argc,char** argv){
 assert(argc==4);const std::string root=argv[1],saveRoot=argv[2],mode=argv[3];
#ifdef RPG_TEST_HISTORICAL_SAVE29
 if(mode=="rollback"){rollback(saveRoot);return 0;}
#endif
#ifdef RPG_TEST_HISTORICAL_SAVE30
 if(mode=="rollback30"){rollback30(saveRoot);return 0;}
#endif
 assert(mode=="upgrade"||mode=="upgrade30");const auto bytes=readFile(root+"/manifest.json"),firmware=readFile(root+"/firmware.bin"),art=readFile(root+"/artes.pak");const std::string raw(bytes.begin(),bytes.end());
 assert(art.size()==7470928&&rpg::get32(art.data(),12)==666&&rpg::get32(art.data(),8)==0x9b4bcfc0);
 Bytes oldArt(art.begin(),art.begin()+6661456);rpg::put32(oldArt.data(),4,6661440);rpg::put32(oldArt.data(),8,0x0b84d93f);rpg::put32(oldArt.data(),12,636);assert(rpg::crc(oldArt.data()+16,6661440)==0x0b84d93f);
 protectedFiles["/RPGPOKET/artes.pak"]=oldArt;protectedFiles["/RPGPOKET/artes_0b84d93f.pak"]=oldArt;protectedFiles["/sentinel-wifi-settings"]={'w','i','f','i'};
#ifdef RPG_TEST_OLD_TOUCH77
 for(unsigned slot=0;slot<3;++slot){auto g=rpg::create(slot,8800+slot);Bytes save(rpg::SAVE_SIZE);rpg::encode(g,42+slot,save.data());protectedFiles["/sentinel-slot"+std::to_string(slot)]=save;}
#else
 static_assert(rpg::SAVE_RESET_EPOCH==1,"No new reset epoch");
#ifdef RPG_TEST_HISTORICAL_SAVE29
 historicalFixtures(saveRoot);
#elif defined(RPG_TEST_HISTORICAL_SAVE30)
 historical30Fixtures(saveRoot);
#else
 if(mode=="upgrade30")preserve30Fixtures(saveRoot);else migrateFixtures(saveRoot);
#endif
 rpg::ResetPhase phase=rpg::ResetPhase::Pending;assert(rpg::decodeResetEpoch(protectedFiles.at("/sentinel-reset-epoch").data(),phase)&&phase==rpg::ResetPhase::Done);
#endif
 initUpdater();reset();updater::Plan p;
#ifdef RPG_TEST_OLD_TOUCH77
 static_assert(rpg::CURRENT_SAVE_FORMAT==28,"Exact touch77 required");assert(!strcmp(FW_VERSION,"2026.10.13-touch77")&&FW_BUILD==2026101305u&&ART_COUNT==636);
 assert(!parseUpdate(raw.c_str(),p));check(raw);assert(updateWorker.state==updater::State::Error&&urls.size()==1);noFlash();puts("PASS: unchanged touch77 rejects honest read29,30/write30 before payload download/flash; legacy USB boundary retained.");
#else
#ifdef RPG_TEST_OLD_SCENES77
 assert(!strcmp(FW_VERSION,"2026.10.13-scenes77")&&FW_BUILD==2026101306u);
#elif defined(RPG_TEST_OLD_SCALE77)
 assert(!strcmp(FW_VERSION,"2026.10.13-scale77")&&FW_BUILD==2026101307u);
#elif defined(RPG_TEST_OLD_CAMP77)
 assert(!strcmp(FW_VERSION,"2026.10.13-camp77")&&FW_BUILD==2026101308u);
#elif defined(RPG_TEST_OLD_EXPLORE77)
 assert(!strcmp(FW_VERSION,"2026.10.13-explore77")&&FW_BUILD==2026101309u);
#elif defined(RPG_TEST_OLD_CLOCK77)
 assert(!strcmp(FW_VERSION,"2026.10.14-clock77")&&FW_BUILD==2026101403u);
#elif defined(RPG_TEST_OLD_STORY77)
 assert(!strcmp(FW_VERSION,"2026.10.14-story77")&&FW_BUILD==2026101402u);
#elif defined(RPG_TEST_OLD_IMMERSION77)
 assert(!strcmp(FW_VERSION,"2026.10.14-immersion77")&&FW_BUILD==2026101401u);
#else
 assert(!strcmp(FW_VERSION,PACKAGE10_VERSION)&&FW_BUILD==PACKAGE10_BUILD);
#endif
 assert(ART_COUNT==666&&parseUpdate(raw.c_str(),p)&&updater::valid(p,target.size,ESP.getPsramSize()));assert(p.build==PACKAGE10_BUILD&&!strcmp(p.version,PACKAGE10_VERSION)&&p.writesSaveFormat==30&&p.readsCurrentSaves);check(raw);
#if defined(RPG_TEST_HISTORICAL_SAVE29) || defined(RPG_TEST_HISTORICAL_SAVE30)
 assert(updateWorker.state==updater::State::Available&&!updateWorker.artOnly&&PACKAGE10_BUILD>FW_BUILD);
#else
 assert(updateWorker.state==updater::State::Current&&updateWorker.artOnly);
#endif
 noFlash();
 for(auto reads:{std::initializer_list<unsigned>{}, {28u}, {31u},
#ifdef RPG_TEST_HISTORICAL_SAVE29
 {30u}
#else
 {29u}
#endif
 }){reset();const auto invalid=change(raw,[&](cJSON* j){setReads(j,reads);});p={};assert(!parseUpdate(invalid.c_str(),p));check(invalid);assert(updateWorker.state==updater::State::Error);noFlash();}
 for(unsigned writes:{28u,65536u}){reset();check(change(raw,[&](cJSON* j){cJSON_ReplaceItemInObject(j,"writes_save_format",cJSON_CreateNumber(writes));}));assert(updateWorker.state==updater::State::Error);noFlash();}
#ifndef RPG_TEST_HISTORICAL_SAVE29
 // Current Save30 also rejects an advertised writer downgrade, even when
 // its read list contains the current format.
 reset();check(change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(j,"writes_save_format",cJSON_CreateNumber(29));}));assert(updateWorker.state==updater::State::Error);noFlash();
 // Even a forged later build cannot make a Save29-only rollback readable.
 reset();check(change(raw,[](cJSON* j){setReads(j,{29});cJSON_ReplaceItemInObject(j,"writes_save_format",cJSON_CreateNumber(29));cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(FW_BUILD+1));}));assert(updateWorker.state==updater::State::Error);noFlash();
#endif
 reset();check(change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(FW_BUILD-1));}));assert(updateWorker.state==updater::State::Error);noFlash();
 reset();check(change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(FW_BUILD));cJSON_ReplaceItemInObject(j,"version",cJSON_CreateString(FW_VERSION));cJSON_ReplaceItemInObject(cJSON_GetObjectItem(j,"art"),"assets",cJSON_CreateNumber(636));}));assert(updateWorker.state==updater::State::Error);noFlash();
#if defined(RPG_TEST_HISTORICAL_SAVE29) || defined(RPG_TEST_HISTORICAL_SAVE30)
 const auto install=raw; // Exact release manifest, not a forged future format.
 constexpr uint32_t expectedBuild=PACKAGE10_BUILD;
#else
 // Explicit protocol-only future-build fixture; real candidate repairs below.
 const auto install=change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(j,"version",cJSON_CreateString("2026.10.14-pvp77-next"));cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(FW_BUILD+1));});
 constexpr uint32_t expectedBuild=FW_BUILD+1;
#endif
 char named[48];updater::artPath(named,sizeof(named),ART_CRC);
 for(unsigned fault=0;fault<5;++fault){reset();check(install);assert(updateWorker.state==updater::State::Available&&!updateWorker.artOnly&&updatePlan.build==expectedBuild&&updatePlan.writesSaveFormat==30);
  if(fault==0)disk[named]=art;auto fw=firmware;if(fault==2)fw[100]^=1;replies.push_back({200,fw});if(fault!=0){auto a=art;if(fault==3)a[12345]^=1;replies.push_back({200,a});}otaWriteFault=fault==4;updateInstallTask(nullptr);
  if(fault<2)assert(ESP.restarted&&bootCalls==1&&beginCalls==1&&flashed==firmware&&disk.at(named)==art);
  else {assert(!ESP.restarted&&!bootCalls&&updateWorker.state==updater::State::Error);if(fault<4)assert(!beginCalls);else assert(beginCalls==1&&abortCalls==1);}
  assert(urls.size()==(fault==0?2u:3u));preserved();
 }
#if !defined(RPG_TEST_HISTORICAL_SAVE29) && !defined(RPG_TEST_HISTORICAL_SAVE30)
 for(bool corrupt:{false,true}){reset();menu.art=ArtStatus::Missing;if(corrupt){disk[named]=art;disk[named][12345]^=1;}check(raw);assert(updateWorker.state==updater::State::Available&&updateWorker.artOnly);replies.push_back({200,art});updateInstallTask(nullptr);assert(ESP.restarted&&!beginCalls&&!bootCalls&&flashed.empty()&&disk.at(named)==art&&urls.size()==2);preserved();}
#endif
 puts("PASS: exact historical Save29/30 -> package10 read29,30/write30 install, or explicit current-client future-build fixture; genuine SHA256, art reuse/download/current repair, corruption/write failure without premature boot; all save bytes/epoch1/Wi-Fi/old art preserved by installer.");
#endif
 puts("LIMITS: host HTTP/SD/OTA/RTOS/persistence adapters; no physical USB/flash/TLS/power-loss/runtime-memory validation. Synthetic firmware mode does not qualify an ESP32 build.");
}
