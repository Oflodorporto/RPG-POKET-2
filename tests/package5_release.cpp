// Untouched published touch77/scenes77/scale77 or current package5 headers; real cJSON/SHA-256.
// SD, HTTP, RTOS and OTA are host adapters, never actual device I/O.
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

#if defined(RPG_TEST_OLD_SCENES77) || defined(RPG_TEST_OLD_SCALE77)
#define RPG_TEST_HISTORICAL_SAVE29
#endif
constexpr uint32_t PACKAGE5_BUILD=2026101308u;
constexpr const char* PACKAGE5_VERSION="2026.10.13-camp77";

static std::vector<uint8_t> readFile(const std::string& path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
static void writeFile(const std::string& path,const std::vector<uint8_t>& bytes){std::ofstream f(path,std::ios::binary);assert(f);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());assert(f);}
#ifndef RPG_TEST_OLD_TOUCH77
// A completed epoch copied from either published Save29 client must only be read, never
// rewritten or used to clear game slots after the package5 upgrade.
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
#endif
static std::string change(const std::string& raw,const std::function<void(cJSON*)>& edit){auto j=cJSON_Parse(raw.c_str());assert(j);edit(j);char* text=cJSON_PrintUnformatted(j);assert(text);std::string result=text;cJSON_free(text);cJSON_Delete(j);return result;}
static void setReads(cJSON* j,std::initializer_list<unsigned> formats){auto reads=cJSON_CreateArray();for(auto n:formats)cJSON_AddItemToArray(reads,cJSON_CreateNumber(n));cJSON_ReplaceItemInObject(j,"reads_save_formats",reads);}
static std::map<std::string,std::vector<uint8_t>> protectedFiles;
static void reset(){disk=protectedFiles;replies.clear();urls.clear();ranges.clear();deleted.clear();flashed.clear();updateWorker={};updateInfo={};updatePlan={};ESP.restarted=false;beginCalls=bootCalls=endCalls=abortCalls=0;otaOpen=false;openFiles=0;maxFiles=1;card=true;writeFault=readFault=renameFault=beginFault=endFault=bootFault=otaWriteFault=false;SD.freeBytes=1ull<<30;WiFi.state=WL_CONNECTED;menu.art=ArtStatus::Ready;}
static void preserved(){for(const auto& p:protectedFiles){assert(disk.at(p.first)==p.second);assert(std::find(deleted.begin(),deleted.end(),p.first)==deleted.end());}assert(!openFiles&&!otaOpen&&replies.empty());
#ifndef RPG_TEST_OLD_TOUCH77
noRepeatedReset();
#endif
}
static void check(const std::string& raw){replies.push_back({200,{raw.begin(),raw.end()}});checkUpdate();}
static void noFlash(){assert(!beginCalls&&!bootCalls&&!ESP.restarted&&flashed.empty());preserved();}

int main(int argc,char** argv){
 assert(argc==3);const std::string root=argv[1],saveRoot=argv[2];const auto bytes=readFile(root+"/manifest.json"),firmware=readFile(root+"/firmware.bin"),art=readFile(root+"/artes.pak");const std::string raw(bytes.begin(),bytes.end());
 assert(art.size()==7470928&&rpg::get32(art.data(),12)==666&&rpg::get32(art.data(),8)==0x9b4bcfc0);
 // The immutable original636 payload, retained alongside the new scenes pack.
 std::vector<uint8_t> oldArt(art.begin(),art.begin()+6661456);rpg::put32(oldArt.data(),4,6661440);rpg::put32(oldArt.data(),8,0x0b84d93f);rpg::put32(oldArt.data(),12,636);assert(rpg::crc(oldArt.data()+16,6661440)==0x0b84d93f);
 protectedFiles["/RPGPOKET/artes.pak"]=oldArt;protectedFiles["/RPGPOKET/artes_0b84d93f.pak"]=oldArt;
 // File sentinels cover installer non-interference; ResetEpoch is exercised separately below.
 protectedFiles["/sentinel-wifi-settings"]={'w','i','f','i'};
 for(unsigned slot=0;slot<3;++slot){auto g=rpg::create(slot,8800+slot);g.p.gold=880+slot;
#ifdef RPG_TEST_OLD_TOUCH77
  std::vector<uint8_t> save(rpg::SAVE_SIZE);rpg::encode(g,42+slot,save.data());
#else
  assert(rpg::persistentValid(g));
#ifdef RPG_TEST_HISTORICAL_SAVE29
  std::vector<uint8_t> save(rpg::PERSISTENT_SAVE_SIZE);rpg::encodePersistent(g,42+slot,slot,save.data());
  writeFile(saveRoot+"/slot"+std::to_string(slot)+".bin",save);
#else
  const auto save=readFile(saveRoot+"/slot"+std::to_string(slot)+".bin");assert(save.size()==rpg::PERSISTENT_SAVE_SIZE);
#endif
  rpg::Game decoded;uint32_t seq=0;assert(rpg::decodePersistent(save.data(),slot,decoded,seq)==rpg::Decode::Ok&&seq==42+slot&&decoded.p.gold==g.p.gold);
  std::vector<uint8_t> encoded(rpg::PERSISTENT_SAVE_SIZE);rpg::encodePersistent(decoded,seq,slot,encoded.data());assert(encoded==save);
#endif
  protectedFiles["/sentinel-save-slot"+std::to_string(slot)]=save;
 }
#ifndef RPG_TEST_OLD_TOUCH77
 static_assert(rpg::SAVE_RESET_EPOCH==1,"Package5 must not reset saves");
#ifdef RPG_TEST_HISTORICAL_SAVE29
 std::vector<uint8_t> marker(rpg::RESET_EPOCH_SIZE);rpg::encodeResetEpoch(rpg::ResetPhase::Done,marker.data());writeFile(saveRoot+"/epoch.bin",marker);
#else
 const auto marker=readFile(saveRoot+"/epoch.bin");assert(marker.size()==rpg::RESET_EPOCH_SIZE);
#endif
 rpg::ResetPhase phase=rpg::ResetPhase::Pending;assert(rpg::decodeResetEpoch(marker.data(),phase)&&phase==rpg::ResetPhase::Done);
 protectedFiles["/sentinel-reset-epoch"]=marker;
#endif
 initUpdater();reset();updater::Plan p;
#ifdef RPG_TEST_OLD_TOUCH77
 static_assert(rpg::CURRENT_SAVE_FORMAT==28,"Must test exact old client");
 assert(!strcmp(FW_VERSION,"2026.10.13-touch77")&&FW_BUILD==2026101305u&&ART_COUNT==636);
 assert(!parseUpdate(raw.c_str(),p));check(raw);assert(updateWorker.state==updater::State::Error);noFlash();
 puts("PASS: unchanged published touch77 rejects honest Save29-only manifest before any download/flash; USB-first required, no legacy compatibility claim.");
#else
 static_assert(rpg::CURRENT_SAVE_FORMAT==29&&rpg::PERSISTENT_SAVE_FORMAT==29,"Save29 contract");
#ifdef RPG_TEST_OLD_SCENES77
 assert(!strcmp(FW_VERSION,"2026.10.13-scenes77")&&FW_BUILD==2026101306u&&ART_COUNT==666);
#elif defined(RPG_TEST_OLD_SCALE77)
 assert(!strcmp(FW_VERSION,"2026.10.13-scale77")&&FW_BUILD==2026101307u&&ART_COUNT==666);
#else
 assert(!strcmp(FW_VERSION,PACKAGE5_VERSION)&&FW_BUILD==PACKAGE5_BUILD&&ART_COUNT==666);
#endif
 assert(parseUpdate(raw.c_str(),p)&&updater::valid(p,target.size,ESP.getPsramSize()));
 assert(p.build==PACKAGE5_BUILD&&!strcmp(p.version,PACKAGE5_VERSION));
 assert(p.writesSaveFormat==29&&p.readsCurrentSaves);check(raw);
#ifdef RPG_TEST_HISTORICAL_SAVE29
 assert(PACKAGE5_BUILD>FW_BUILD);
 assert(updateWorker.state==updater::State::Available&&!updateWorker.artOnly&&updatePlan.build==PACKAGE5_BUILD&&updatePlan.build>FW_BUILD);
#else
 assert(updateWorker.state==updater::State::Current&&updateWorker.artOnly);
#endif
 noFlash();
 for(auto reads:{std::initializer_list<unsigned>{}, {28u}, {30u}}){reset();const auto invalid=change(raw,[&](cJSON* j){setReads(j,reads);});p={};assert(!parseUpdate(invalid.c_str(),p));check(invalid);assert(updateWorker.state==updater::State::Error);noFlash();}
 for(unsigned writes:{28u,65536u}){reset();const auto invalid=change(raw,[&](cJSON* j){cJSON_ReplaceItemInObject(j,"writes_save_format",cJSON_CreateNumber(writes));});check(invalid);assert(updateWorker.state==updater::State::Error);noFlash();}
 reset();check(change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(FW_BUILD));cJSON_ReplaceItemInObject(j,"version",cJSON_CreateString(FW_VERSION));cJSON_ReplaceItemInObject(cJSON_GetObjectItem(j,"art"),"assets",cJSON_CreateNumber(636));}));assert(updateWorker.state==updater::State::Error);noFlash();
 reset();check(change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(FW_BUILD-1));}));assert(updateWorker.state==updater::State::Error);noFlash();
 // Both published Save29 clients see the exact package5 release manifest.
 // For the current client only, a later-build protocol fixture reuses these
 // bytes to retain the original future29/read29-write30 regression coverage.
#ifdef RPG_TEST_HISTORICAL_SAVE29
 const auto future=raw;
 constexpr uint32_t expectedTarget=PACKAGE5_BUILD;
#else
 const auto future=change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(j,"version",cJSON_CreateString("2026.10.13-camp77-next"));cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(FW_BUILD+1));});
 constexpr uint32_t expectedTarget=FW_BUILD+1;
#endif
 const auto future30=change(future,[](cJSON* j){setReads(j,{29,30});cJSON_ReplaceItemInObject(j,"writes_save_format",cJSON_CreateNumber(30));});p={};assert(parseUpdate(future30.c_str(),p)&&updater::valid(p,target.size,ESP.getPsramSize()));
 char named[48];updater::artPath(named,sizeof(named),ART_CRC);
 for(unsigned mode=0;mode<5;++mode){reset();check(future);assert(updateWorker.state==updater::State::Available&&!updateWorker.artOnly&&updatePlan.build==expectedTarget&&updatePlan.build>FW_BUILD);
  if(mode==0)disk[named]=art;
  auto fw=firmware;if(mode==2)fw[100]^=1;replies.push_back({200,fw});
  if(mode!=0){auto a=art;if(mode==3)a[12345]^=1;replies.push_back({200,a});}
  otaWriteFault=mode==4;updateInstallTask(nullptr);
  if(mode<2){assert(ESP.restarted&&bootCalls==1&&beginCalls==1&&flashed==firmware&&disk.at(named)==art);}
  else {assert(!ESP.restarted&&!bootCalls&&updateWorker.state==updater::State::Error);if(mode<4)assert(!beginCalls);else assert(beginCalls==1&&abortCalls==1);}
  assert(urls.size()==(mode==0?2u:3u));preserved();
 }
#ifndef RPG_TEST_HISTORICAL_SAVE29
 reset();menu.art=ArtStatus::Missing;check(raw);assert(updateWorker.state==updater::State::Available&&updateWorker.artOnly);replies.push_back({200,art});updateInstallTask(nullptr);assert(ESP.restarted&&!beginCalls&&!bootCalls&&disk.at(named)==art);preserved();
#endif
 puts("PASS: actual scenes77/scale77 package5 upgrade or explicitly synthetic current-package5 future fixture; completed epoch1 never reset; Save29-only/current29 membership, future29 and read29/write30 plans, downgrade/missing29 rejection; real SHA256 firmware/art/reuse/repair, corruption/OTA-write failure; no premature boot; three Save29 records written by the selected published scenes77/scale77 client round-trip byte-identically, completed epoch marker/Wi-Fi/previous art untouched.");
#endif
 puts("LIMITS: host HTTP/SD/OTA/RTOS adapters; no physical USB/flash/TLS/power-loss or runtime-memory validation. Reset epoch behavior is covered separately.");
}
