#define RPG_TEST_REAL_SHA256
#include "Save.h"
#include "AssetCatalog.h"
#include "FakeDevice.h"
enum class ArtStatus {Ready,Missing};struct {bool scanning=false,connecting=false;ArtStatus art=ArtStatus::Missing;} menu;
#include "UpdateDevice.h"
#include <fstream>
#include <sstream>
static std::vector<uint8_t> bytes(const char* path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
static std::string hash(const std::vector<uint8_t>& b){mbedtls_sha256_context c;mbedtls_sha256_init(&c);assert(!mbedtls_sha256_starts(&c,0));for(size_t off=0;off<b.size();off+=4096)assert(!mbedtls_sha256_update(&c,b.data()+off,std::min<size_t>(4096,b.size()-off)));uint8_t d[32];assert(!mbedtls_sha256_finish(&c,d));char s[65];for(unsigned i=0;i<32;++i)snprintf(s+2*i,3,"%02x",d[i]);return s;}
int main(int argc,char** argv){
 assert(argc==1||argc==3);const auto art=bytes(argc==3?argv[1]:"cartao/RPGPOKET/artes.pak");auto raw=bytes(argc==3?argv[2]:"tests/update_fake/procedural6-readback-manifest.json");
 // Preserve independently pinned current art metadata; use the compiled identity to exercise
 // same-build repair also on an isolated dev build (no firmware hash shortcut).
 std::string manifest(raw.begin(),raw.end());auto replace=[&](const char* from,const std::string& to){auto pos=manifest.find(from);assert(pos!=std::string::npos);manifest.replace(pos,strlen(from),to);};replace("\"version\":\"2026.10.13-scenes77\"",std::string("\"version\":\"")+FW_VERSION+"\"");replace("\"build\":2026101306",std::string("\"build\":")+std::to_string(FW_BUILD));raw.assign(manifest.begin(),manifest.end());
 assert(hash({'a','b','c'})=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 assert(art.size()==7470928&&hash(art)=="14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56");
 initUpdater();
#ifdef RPG_TEST_VERIFY_DETAIL
 // Boot diagnostics must not introduce a null-mutex crash on allocation failure.
 {auto guard=updateGuard;updateGuard=nullptr;storageDiag::Failure f;f.issue=storageDiag::Issue::Open;recordVerifyFailure(f);assert(!updateWorker.lastVerify.valid());updateGuard=guard;}
#endif
 unsigned cases=0;char named[48];updater::artPath(named,sizeof(named),ART_CRC);
 std::map<std::string,std::vector<uint8_t>> protectedFiles;
 for(unsigned slot=0;slot<3;++slot){auto g=rpg::create(slot,1700+slot);g.p.gold=100+slot;std::vector<uint8_t> save(rpg::SAVE_SIZE);rpg::encode(g,slot+1,save.data());protectedFiles["/sentinel-save28-slot"+std::to_string(slot)]=save;}
 protectedFiles["/RPGPOKET/artes.pak"]={9,8,7,6}; // Invalid manual file must not block repair.
 for(unsigned fault=0;fault<13;++fault){
  disk=protectedFiles;deleted.clear();replies.clear();urls.clear();ranges.clear();openFiles=0;maxFiles=1;card=true;writeFault=readFault=renameFault=false;beginCalls=bootCalls=endCalls=abortCalls=0;otaOpen=false;ESP.restarted=false;updateWorker={};updateInfo={};
  readBackFaultPath="/RPGPOKET/artes.part";readBackFaultAt=~size_t(0);readBackBytes=0;readBackFlipAt=~size_t(0);readBackOpenFail=false;reportedSizeDelta=0;flushFaultPath.clear();flushTrim=0;
  replies.push_back({200,raw});checkUpdate();assert(updateWorker.state==updater::State::Available&&updateWorker.artOnly&&updatePlan.build==FW_BUILD);assert(updatePlan.assets==666&&updatePlan.artCrc==0x9b4bcfc0);
  auto payload=art;
  if(fault==1)readBackOpenFail=true;
  if(fault==2)reportedSizeDelta=1;
  if(fault==3){readBackFaultAt=4096;readBackBytes=2048;}
  if(fault==4)readBackFlipAt=12345;
  if(fault==5){flushFaultPath=readBackFaultPath;flushTrim=16;}
  if(fault==6)payload[0]^=1;
  if(fault==7)payload[22222]^=1;
  if(fault==8)payload[4]^=1;
  if(fault==9)payload[8]^=1;
  if(fault==10)payload[12]^=1;
  if(fault==11)updatePlan.art.sha[0]=updatePlan.art.sha[0]=='a'?'b':'a';
  if(fault==12){readBackFaultPath=named;readBackFaultAt=4096;readBackBytes=2048;}
  replies.push_back({200,payload});updateInstallTask(nullptr);
  assert(!beginCalls&&!bootCalls&&!endCalls&&!otaOpen&&!openFiles);assert(urls.size()==2&&urls[1]==updatePlan.art.url&&replies.empty());
  for(auto& entry:protectedFiles)assert(disk.at(entry.first)==entry.second);
  if(!fault){assert(ESP.restarted&&updateWorker.state==updater::State::Restarting&&disk.at(named)==art&&hash(disk.at(named))==updatePlan.art.sha);}
  else {assert(!ESP.restarted&&updateWorker.state==updater::State::Error);
#ifdef RPG_TEST_VERIFY_DETAIL
   assert(!strncmp(updateWorker.message,"Falha conferir SD",17));
   const auto f=updateWorker.lastVerify;using storageDiag::Issue;const Issue expected[]={Issue::None,Issue::Open,Issue::Size,Issue::Read,Issue::Crc,Issue::Size,Issue::Magic,Issue::Crc,Issue::Length,Issue::HeaderCrc,Issue::Catalog,Issue::Sha,Issue::Read};
   assert(f.valid()&&f.issue==expected[fault]&&f.stage==(fault==12?storageDiag::Stage::Installed:storageDiag::Stage::Downloaded)&&f.file==(fault==12?storageDiag::File::VersionedArt:storageDiag::File::ArtPart));
   if(fault==3||fault==12)assert(f.offset==4096&&f.expected==4096&&f.actual==2048);
   if(fault==5)assert(f.expected==art.size()&&f.actual==art.size()-16);
   resetUpdateScreen();assert(updateWorker.lastVerify.issue==f.issue&&updateInfo.lastVerify.offset==f.offset);
   WiFi.state=WL_CONNECTED;for(unsigned i=0;i<3;++i)replies.push_back({500,{}});assert(startNetworkTest());testNetwork();resetUpdateScreen();assert(updateWorker.lastVerify.issue==f.issue);
#else
   assert(!strcmp(updateWorker.message,fault==12?"Instalacao falhou / jogo preservado":"Download incompleto / tente de novo"));
#endif
   assert(!disk.count(named));assert(bool(disk.count("/RPGPOKET/artes.part"))==(fault==1||fault==3||fault==12));}
  ++cases;
 }
 for(unsigned mode=0;mode<3;++mode){
  disk=protectedFiles;replies.clear();urls.clear();ranges.clear();openFiles=0;maxFiles=1;ESP.restarted=false;updateWorker={};updateInfo={};
  readBackFaultPath.clear();readBackOpenFail=false;reportedSizeDelta=0;flushFaultPath.clear();flushTrim=0;
  replies.push_back({200,raw});checkUpdate();assert(updateWorker.artOnly);
  if(mode==2)disk[named]=art;
  else {disk["/RPGPOKET/artes.part"]={art.begin(),art.begin()+4096};disk["/RPGPOKET/artes.part.tag"]={updatePlan.art.sha,updatePlan.art.sha+64};
   if(mode==0){std::vector<uint8_t> tail(art.begin()+4096,art.end());replies.push_back({206,tail,-2,"bytes 4096-"+std::to_string(art.size()-1)+"/"+std::to_string(art.size())});}else replies.push_back({200,art});}
  updateInstallTask(nullptr);assert(ESP.restarted&&disk.at(named)==art&&hash(disk.at(named))==updatePlan.art.sha&&!openFiles&&!beginCalls&&!bootCalls);
  assert(urls.size()==(mode==2?1u:2u));if(mode!=2)assert(ranges.back()=="bytes=4096-");for(auto& p:protectedFiles)assert(disk.at(p.first)==p.second);++cases;
 }
 printf("PASS: %u full current-pack same-build repair paths with real SHA-256: clean transfer/read-back succeeds; open/stat/short-read/read-corruption/flush-truncation/header/catalog/CRC/SHA failures reproduce exact 80%% error; firmware never flashed; three Save28 sentinels/manual original unchanged.\n",cases);
}
