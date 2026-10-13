#define RPG_TEST_REAL_SHA256
#include "Save.h"
#include "AssetCatalog.h"
#include "FakeDevice.h"
enum class ArtStatus {Ready,Missing};struct {bool scanning=false,connecting=false;ArtStatus art=ArtStatus::Missing;} menu;
#include "UpdateDevice.h"
#include <fstream>
static std::string hash(const std::vector<uint8_t>& b){mbedtls_sha256_context c;mbedtls_sha256_init(&c);mbedtls_sha256_starts(&c,0);mbedtls_sha256_update(&c,b.data(),b.size());uint8_t d[32];mbedtls_sha256_finish(&c,d);mbedtls_sha256_free(&c);char s[65];for(unsigned i=0;i<32;++i)snprintf(s+i*2,3,"%02x",d[i]);return s;}
int main(){
 std::ifstream input("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(input);std::vector<uint8_t> art{std::istreambuf_iterator<char>(input),{}},fw(10000,0x57);
 static_assert(sizeof(updater::Info)<512,"bounded RAM diagnostics");initUpdater();updatePlan={};updatePlan.artCrc=ART_CRC;updatePlan.assets=ART_COUNT;updatePlan.art.bytes=art.size();updatePlan.firmware.bytes=fw.size();strcpy(updatePlan.art.sha,hash(art).c_str());strcpy(updatePlan.firmware.sha,hash(fw).c_str());
 char final[48],backup[48];updater::artPath(final,sizeof(final),ART_CRC);updater::artBackupPath(backup,sizeof(backup),ART_CRC);
 const char* ap="/RPGPOKET/artes.part";const char* fp="/RPGPOKET/firmware.part";
 std::map<std::string,std::vector<uint8_t>> saves;for(unsigned slot=0;slot<3;++slot){auto g=rpg::create(slot,700+slot);std::vector<uint8_t> b(rpg::SAVE_SIZE);rpg::encode(g,50+slot,b.data());saves["/sentinel"+std::to_string(slot)]=b;}
 auto reset=[&](){disk=saves;disk["/RPGPOKET/artes_00000001.pak"]={1,2,3};disk["/RPGPOKET/artes.pak"]={5,6,7};disk[ap]=art;disk[std::string(ap)+".tag"]={updatePlan.art.sha,updatePlan.art.sha+64};disk[fp]=fw;disk[std::string(fp)+".tag"]={updatePlan.firmware.sha,updatePlan.firmware.sha+64};
  lookupErrno=EIO;hashFinishFaultOnOta=copyFlipOnOta=false;renameFailTo.clear();lookupFaultPath.clear();openMetaFaultPath.clear();readBackFaultPath.clear();readBackFaultAt=readBackFlipAt=~size_t(0);readBackBytes=0;readBackErrno=0;readBackFailures=-1;recoverOnSlow=false;readBackOpenFail=false;reportedSizeDelta=0;flushFaultPath.clear();flushTrim=0;renameFailFrom.clear();card=true;SD.freeBytes=1ull<<30;sdMounts=0;openFiles=0;maxFiles=1;readFault=writeFault=renameFault=beginFault=otaWriteFault=endFault=bootFault=false;beginCalls=bootCalls=abortCalls=endCalls=0;otaOpen=false;ESP.restarted=false;updateWorker={};updateInfo={};urls.clear();replies.clear();deleted.clear();};
 auto protectedOK=[&](){for(auto& p:saves)assert(disk.at(p.first)==p.second);assert(disk.at("/RPGPOKET/artes_00000001.pak")==std::vector<uint8_t>({1,2,3}));assert(disk.at("/RPGPOKET/artes.pak")==std::vector<uint8_t>({5,6,7}));assert(!openFiles&&!otaOpen);};
 unsigned cases=0;
 struct Lookup {bool present;int error;bool exists(const char*){errno=error;return present;}};
 for(auto pair:{std::pair<Lookup,ArtPresence>{{false,0},ArtPresence::Uncertain},{{false,ENOENT},ArtPresence::Absent},{{false,EIO},ArtPresence::Uncertain},{{true,0},ArtPresence::Present},{{true,EIO},ArtPresence::Uncertain}}){assert(artPresence(pair.first,"fixed")==pair.second);++cases;}

 for(unsigned mode=0;mode<16;++mode){reset();
  if(mode==1||mode==2){readBackFaultPath=ap;readBackFaultAt=4534272;readBackErrno=EIO;recoverOnSlow=mode==1;}
  if(mode==3){readBackFaultPath=std::string(ap)+".tag";readBackFaultAt=0;readBackErrno=EIO;}
  if(mode==4)disk[ap][12345]^=1;
  if(mode==5){readBackFaultPath=final;readBackFaultAt=4812800;readBackErrno=EIO;}
  if(mode==6){disk[final]={9,9,9};disk[backup]={8,8,8};}
  if(mode==7){disk[final]={9,9,9};renameFailFrom=ap;}
  if(mode==8)SD.freeBytes=1048576; // Complete cached files need only the safety reserve.
  if(mode==9)SD.freeBytes=1048575;
  if(mode==10)openMetaFaultPath=ap;
  if(mode==11)openMetaFaultPath=std::string(ap)+".tag";
  if(mode==12){disk[final]=art;lookupFaultPath=final;}
  if(mode==13){disk[final]={9,9,9};lookupFaultPath=backup;}
  if(mode==14){readBackFaultPath=std::string(ap)+".tag";readBackOpenFail=true;readBackErrno=0;}
  if(mode==15){disk[final]=art;lookupFaultPath=final;lookupErrno=0;}
  bool ok=installUpdate(false);assert(ok==(mode==0||mode==1||mode==8));assert(urls.empty()&&replies.empty());protectedOK();
  if(ok){assert(ESP.restarted&&bootCalls==1&&flashed==fw&&disk.at(final)==art);if(mode==1){assert(sdMounts==2&&lastSdHz==1000000&&updateWorker.recovery.attempts==1&&updateWorker.recovery.first.offset==4534272&&updateWorker.recovery.recovered);}}
  else {assert(!ESP.restarted&&!bootCalls);if(mode==2||mode==3||mode==5){assert(updateWorker.canResume&&updateWorker.stagingRetained&&disk.at(ap)==art&&disk.at(fp)==fw&&updateWorker.recovery.attempts==1&&sdMounts==2);assert(updateWorker.recovery.first.valid());}
   if(mode==10||mode==11){assert(disk.at(ap)==art&&disk.at(fp)==fw&&updateWorker.canResume&&updateWorker.lastVerify.issue==storageDiag::Issue::Open&&updateWorker.lastVerify.code==EIO);}
   if(mode==14)assert(disk.at(ap)==art&&disk.at(fp)==fw&&updateWorker.canResume&&updateWorker.lastVerify.code==0);
   if(mode==15)assert(disk.at(final)==art&&disk.at(ap)==art&&updateWorker.canResume&&updateWorker.lastVerify.code==0);
   if(mode==12)assert(disk.at(final)==art&&disk.at(ap)==art&&!disk.count(backup));
   if(mode==13)assert(disk.at(final)==std::vector<uint8_t>({9,9,9})&&disk.at(ap)==art&&!disk.count(backup));
   if(mode==4)assert(!disk.count(ap)&&!disk.count(std::string(ap)+".tag")&&disk.at(fp)==fw);
   if(mode==6)assert(disk.at(final)==std::vector<uint8_t>({9,9,9})&&disk.at(backup)==std::vector<uint8_t>({8,8,8})&&disk.at(ap)==art);
   if(mode==7)assert(disk.at(final)==std::vector<uint8_t>({9,9,9})&&!disk.count(backup)&&disk.at(ap)==art);
   if(mode==9)assert(disk.at(ap)==art&&disk.at(fp)==fw);
  }
  if(mode==2||mode==3||mode==5){readBackFaultPath.clear();assert(installUpdate(false)&&ESP.restarted&&urls.empty()&&disk.at(final)==art);protectedOK();}
  ++cases;
 }
 // A bad prior same-name artifact is retained as backup after verified replacement.
 reset();disk[final]={9,9,9};assert(installUpdate(true)&&disk.at(final)==art&&disk.at(backup)==std::vector<uint8_t>({9,9,9}));protectedOK();++cases;
 // Models power loss after first rename: no final yet; stage and backup survive.
 reset();disk[backup]={9,9,9};assert(installUpdate(true)&&disk.at(final)==art&&disk.at(backup)==std::vector<uint8_t>({9,9,9}));protectedOK();++cases;
 // Publication failure after second rename restores the original name without deleting bytes.
 reset();disk[final]={9,9,9};readBackFaultPath=final;readBackFaultAt=4096;readBackErrno=EIO;assert(!installUpdate(true)&&disk.at(final)==std::vector<uint8_t>({9,9,9})&&disk.at(ap)==art&&!disk.count(backup));protectedOK();++cases;
 // Rollback itself may fail. Neither version is deleted, and backup is a full valid pack.
 reset();disk[final]=art;readBackFaultPath=final;readBackFlipAt=12345;renameFailTo=ap;
 assert(!installUpdate(true)&&disk.at(final)==art&&disk.at(backup)==art&&!disk.count(ap)&&!bootCalls);protectedOK();++cases;
 // Hash API failure or mutation only during the final copy must abort the OTA handle.
 for(bool api:{false,true}){reset();hashFinishFaultOnOta=api;copyFlipOnOta=!api;assert(!installUpdate(false)&&!ESP.restarted&&!bootCalls&&beginCalls==1&&abortCalls==1&&!endCalls&&disk.at(fp)==fw&&updateWorker.canResume);assert(updateWorker.lastVerify.stage==storageDiag::Stage::Installing&&updateWorker.lastVerify.issue==(api?storageDiag::Issue::HashApi:storageDiag::Issue::Sha));protectedOK();++cases;}
 printf("PASS: %u actual installer recovery cases, real full-pack SHA/CRC: bounded 1MHz restart, physical EIO offsets, complete stage no HTTP, tag uncertainty, selective corruption discard, safe promotion/backup/rollback/interruption, low-space resume and three Save28 sentinels.\n",cases);
}
