// Exact published dungeon77 headers, real cJSON, production HTTP/SD installer and host SHA-256.
#define RPG_TEST_REAL_SHA256
#include "Save.h"
#include "AssetCatalog.h"
#include "FakeDevice.h"
enum class ArtStatus {Ready,Missing};struct {bool scanning=false,connecting=false;ArtStatus art=ArtStatus::Ready;} menu;
#include "UpdateDevice.h"
#include <fstream>
static std::vector<uint8_t> file(const std::string& p){std::ifstream f(p,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
 assert(argc==2);assert(!strcmp(FW_VERSION,"2026.10.13-dungeon77")&&FW_BUILD==2026101304u&&ART_COUNT==636);
 const std::string root=argv[1];const auto raw=file(root+"/manifest.json"),firmware=file(root+"/firmware.bin"),art=file(root+"/artes.pak");initUpdater();char named[48];updater::artPath(named,sizeof(named),ART_CRC);
 std::map<std::string,std::vector<uint8_t>> safe;safe["/RPGPOKET/artes.pak"]=art;
 for(unsigned slot=0;slot<3;++slot){auto g=rpg::create(slot,8800+slot);g.p.gold=880+slot;std::vector<uint8_t> save(rpg::SAVE_SIZE);rpg::encode(g,42+slot,save.data());safe["/sentinel-save28-slot"+std::to_string(slot)]=save;}
 for(unsigned mode=0;mode<4;++mode){disk=safe;replies.clear();urls.clear();ranges.clear();deleted.clear();flashed.clear();updateWorker={};updateInfo={};ESP.restarted=false;beginCalls=bootCalls=endCalls=abortCalls=0;otaOpen=false;openFiles=0;maxFiles=1;card=true;writeFault=readFault=renameFault=beginFault=endFault=bootFault=false;otaWriteFault=mode==3;
  replies.push_back({200,raw});checkUpdate();assert(updateWorker.state==updater::State::Available&&!updateWorker.artOnly&&updatePlan.build==2026101305u&&!strcmp(updatePlan.version,"2026.10.13-touch77"));assert(updatePlan.art.bytes==art.size()&&updatePlan.firmware.bytes==firmware.size());
  if(mode==0)disk[named]=art;replies.push_back({200,firmware});if(mode!=0){auto body=art;if(mode==2)body[12345]^=1;replies.push_back({200,body});}
  updateInstallTask(nullptr);
  if(mode<2){assert(ESP.restarted&&bootCalls==1&&beginCalls==1&&flashed==firmware&&disk.at(named)==art);}
  else {assert(!ESP.restarted&&!bootCalls&&updateWorker.state==updater::State::Error);if(mode==2)assert(!beginCalls);else assert(beginCalls==1&&abortCalls==1);}
  assert(replies.empty()&&!openFiles&&!otaOpen);assert(urls.size()==(mode==0?2u:3u));for(const auto& p:safe)assert(disk.at(p.first)==p.second);
 }
 printf("PASS: exact published dungeon77 -> touch77, real SHA-256 full firmware/art responses; existing-art reuse, new transfer, bad art and OTA-write failure; no premature boot; three Save28 sentinels and original art preserved.\n");
}
