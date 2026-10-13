// Untouched published touch77 or current Save29 headers, real cJSON and SHA-256.
// SD, HTTP, RTOS and OTA are host adapters, never actual device I/O.
#define RPG_TEST_REAL_SHA256
#include "Save.h"
#ifndef RPG_TEST_OLD_TOUCH77
#include "Persistence.h"
#endif
#include "AssetCatalog.h"
#include "FakeDevice.h"
enum class ArtStatus {Ready,Missing};
struct {bool scanning=false,connecting=false;ArtStatus art=ArtStatus::Ready;} menu;
#include "UpdateDevice.h"
#include <fstream>
#include <functional>

static std::vector<uint8_t> readFile(const std::string& path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
static std::string change(const std::string& raw,const std::function<void(cJSON*)>& edit){auto j=cJSON_Parse(raw.c_str());assert(j);edit(j);char* text=cJSON_PrintUnformatted(j);assert(text);std::string result=text;cJSON_free(text);cJSON_Delete(j);return result;}
static void setReads(cJSON* j,std::initializer_list<unsigned> formats){auto reads=cJSON_CreateArray();for(auto n:formats)cJSON_AddItemToArray(reads,cJSON_CreateNumber(n));cJSON_ReplaceItemInObject(j,"reads_save_formats",reads);}
static std::map<std::string,std::vector<uint8_t>> protectedFiles;
static void reset(){disk=protectedFiles;replies.clear();urls.clear();ranges.clear();deleted.clear();flashed.clear();updateWorker={};updateInfo={};updatePlan={};ESP.restarted=false;beginCalls=bootCalls=endCalls=abortCalls=0;otaOpen=false;openFiles=0;maxFiles=1;card=true;writeFault=readFault=renameFault=beginFault=endFault=bootFault=otaWriteFault=false;SD.freeBytes=1ull<<30;WiFi.state=WL_CONNECTED;menu.art=ArtStatus::Ready;}
static void preserved(){for(const auto& p:protectedFiles){assert(disk.at(p.first)==p.second);assert(std::find(deleted.begin(),deleted.end(),p.first)==deleted.end());}assert(!openFiles&&!otaOpen&&replies.empty());}
static void check(const std::string& raw){replies.push_back({200,{raw.begin(),raw.end()}});checkUpdate();}
static void noFlash(){assert(!beginCalls&&!bootCalls&&!ESP.restarted&&flashed.empty());preserved();}

int main(int argc,char** argv){
 assert(argc==2);const std::string root=argv[1];const auto bytes=readFile(root+"/manifest.json"),firmware=readFile(root+"/firmware.bin"),art=readFile(root+"/artes.pak");const std::string raw(bytes.begin(),bytes.end());
 assert(art.size()==7470928&&rpg::get32(art.data(),12)==666&&rpg::get32(art.data(),8)==0x9b4bcfc0);
 // The immutable original636 payload, retained alongside the new scenes pack.
 std::vector<uint8_t> oldArt(art.begin(),art.begin()+6661456);rpg::put32(oldArt.data(),4,6661440);rpg::put32(oldArt.data(),8,0x0b84d93f);rpg::put32(oldArt.data(),12,636);assert(rpg::crc(oldArt.data()+16,6661440)==0x0b84d93f);
 protectedFiles["/RPGPOKET/artes.pak"]=oldArt;protectedFiles["/RPGPOKET/artes_0b84d93f.pak"]=oldArt;
 // These are preservation sentinels, not an emulation of NVS or its reset.
 protectedFiles["/sentinel-wifi-settings"]={'w','i','f','i'};
 for(unsigned slot=0;slot<3;++slot){auto g=rpg::create(slot,8800+slot);g.p.gold=880+slot;
#ifdef RPG_TEST_OLD_TOUCH77
  std::vector<uint8_t> save(rpg::SAVE_SIZE);rpg::encode(g,42+slot,save.data());
#else
  assert(rpg::persistentValid(g));std::vector<uint8_t> save(rpg::PERSISTENT_SAVE_SIZE);rpg::encodePersistent(g,42+slot,slot,save.data());rpg::Game decoded;uint32_t seq=0;assert(rpg::decodePersistent(save.data(),slot,decoded,seq)==rpg::Decode::Ok&&seq==42+slot&&decoded.p.gold==g.p.gold);
#endif
  protectedFiles["/sentinel-save-slot"+std::to_string(slot)]=save;
 }
 initUpdater();reset();updater::Plan p;
#ifdef RPG_TEST_OLD_TOUCH77
 static_assert(rpg::CURRENT_SAVE_FORMAT==28,"Must test exact old client");
 assert(!strcmp(FW_VERSION,"2026.10.13-touch77")&&FW_BUILD==2026101305u&&ART_COUNT==636);
 assert(!parseUpdate(raw.c_str(),p));check(raw);assert(updateWorker.state==updater::State::Error);noFlash();
 puts("PASS: unchanged published touch77 rejects honest Save29-only manifest before any download/flash; USB-first required, no legacy compatibility claim.");
#else
 static_assert(rpg::CURRENT_SAVE_FORMAT==29&&rpg::PERSISTENT_SAVE_FORMAT==29,"Save29 contract");
 assert(!strcmp(FW_VERSION,"2026.10.13-scenes77")&&FW_BUILD==2026101306u&&ART_COUNT==666);
 assert(parseUpdate(raw.c_str(),p)&&updater::valid(p,target.size,ESP.getPsramSize()));
 assert(p.writesSaveFormat==29&&p.readsCurrentSaves);check(raw);assert(updateWorker.state==updater::State::Current&&updateWorker.artOnly);noFlash();
 for(auto reads:{std::initializer_list<unsigned>{}, {28u}, {30u}}){reset();const auto invalid=change(raw,[&](cJSON* j){setReads(j,reads);});p={};assert(!parseUpdate(invalid.c_str(),p));check(invalid);assert(updateWorker.state==updater::State::Error);noFlash();}
 for(unsigned writes:{28u,65536u}){reset();const auto invalid=change(raw,[&](cJSON* j){cJSON_ReplaceItemInObject(j,"writes_save_format",cJSON_CreateNumber(writes));});check(invalid);assert(updateWorker.state==updater::State::Error);noFlash();}
 reset();check(change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(cJSON_GetObjectItem(j,"art"),"assets",cJSON_CreateNumber(636));}));assert(updateWorker.state==updater::State::Error);noFlash();
 reset();check(change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(FW_BUILD-1));}));assert(updateWorker.state==updater::State::Error);noFlash();
 // A future Save29 OTA plan uses these exact real firmware/art response bytes.
 // It is a host protocol fixture, not a compiled or published future release.
 const auto future=change(raw,[](cJSON* j){cJSON_ReplaceItemInObject(j,"version",cJSON_CreateString("2026.10.13-scenes77-next"));cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(FW_BUILD+1));});
 const auto future30=change(future,[](cJSON* j){setReads(j,{29,30});cJSON_ReplaceItemInObject(j,"writes_save_format",cJSON_CreateNumber(30));});p={};assert(parseUpdate(future30.c_str(),p)&&updater::valid(p,target.size,ESP.getPsramSize()));
 char named[48];updater::artPath(named,sizeof(named),ART_CRC);
 for(unsigned mode=0;mode<5;++mode){reset();check(future);assert(updateWorker.state==updater::State::Available&&!updateWorker.artOnly&&updatePlan.build==FW_BUILD+1);
  if(mode==0)disk[named]=art;
  auto fw=firmware;if(mode==2)fw[100]^=1;replies.push_back({200,fw});
  if(mode!=0){auto a=art;if(mode==3)a[12345]^=1;replies.push_back({200,a});}
  otaWriteFault=mode==4;updateInstallTask(nullptr);
  if(mode<2){assert(ESP.restarted&&bootCalls==1&&beginCalls==1&&flashed==firmware&&disk.at(named)==art);}
  else {assert(!ESP.restarted&&!bootCalls&&updateWorker.state==updater::State::Error);if(mode<4)assert(!beginCalls);else assert(beginCalls==1&&abortCalls==1);}
  assert(urls.size()==(mode==0?2u:3u));preserved();
 }
 reset();menu.art=ArtStatus::Missing;check(raw);assert(updateWorker.state==updater::State::Available&&updateWorker.artOnly);replies.push_back({200,art});updateInstallTask(nullptr);assert(ESP.restarted&&!beginCalls&&!bootCalls&&disk.at(named)==art);preserved();
 puts("PASS: Save29-only/current29 membership, future29 and read29/write30 plans, downgrade/missing29 rejection; real SHA256 firmware/art/reuse/repair, corruption/OTA-write failure; no premature boot; three Save29 sentinels and previous art untouched.");
#endif
 puts("LIMITS: host HTTP/SD/OTA/RTOS adapters; no physical USB/flash/TLS/power-loss or runtime-memory validation. Reset epoch behavior is covered separately.");
}
