#define main previousOtaTests
#include "update_adapter.cpp"
#undef main
#include <climits>
int main(){
 initUpdater();firmware.resize(10000);for(unsigned i=0;i<firmware.size();++i)firmware[i]=i*7;
 art.resize(10000,0x35);memcpy(art.data(),"PKA1",4);rpg::put32(art.data(),4,art.size()-16);rpg::put32(art.data(),8,rpg::crc(art.data()+16,art.size()-16));rpg::put32(art.data(),12,1);oldArt={9,8,7,6};plan();unsigned cases=0;
 for(bool artFile:{false,true})for(bool append:{false,true})for(size_t wrote:{size_t(0),size_t(1),size_t(2048),size_t(4095)})for(int error:{0,ENOSPC,EIO}){
  reset();updateInitialFreeKnown=false;shortWritePath=artFile?"/RPGPOKET/artes.part":"/RPGPOKET/firmware.part";shortWriteAt=append?4096:0;shortWriteBytes=wrote;shortWriteErrno=error;
  const auto& bytes=artFile?art:firmware;const auto& package=artFile?updatePlan.art:updatePlan.firmware;
  if(artFile){disk["/RPGPOKET/firmware.part"]=firmware;tag("/RPGPOKET/firmware.part",updatePlan.firmware);}
  if(append){disk[shortWritePath]={bytes.begin(),bytes.begin()+4096};tag(shortWritePath.c_str(),package);std::string range="bytes 4096-9999/10000";replies.push_back({206,{bytes.begin()+4096,bytes.end()},-2,range});}
  else replies.push_back({200,bytes});
  assert(!installUpdate(false)&&!ESP.restarted&&!bootCalls&&!beginCalls);preserved();
  const auto f=updateWorker.lastWrite;assert(f.valid&&f.file==(artFile?updater::DownloadFile::Art:updater::DownloadFile::Firmware));
  assert(f.offset==(append?4096u:0u)&&f.total==bytes.size()&&f.requested==4096&&f.written==wrote&&f.error==error&&f.append==append);
  assert(f.freeKnown&&f.initialFree==SD.freeBytes);assert(f.progress==(artFile?25u:0u)+(append?4096u:0u)*(artFile?55u:25u)/bytes.size());
  assert(!disk.count(shortWritePath)&&!disk.count(shortWritePath+".tag")); // existing safe discard, never append a torn block
  assert(strncmp(updateWorker.message,"SD: gravacao falhou",sizeof("SD: gravacao falhou")-1)==0);
  tickUpdater();assert(updateInfo.lastWrite.valid&&updateInfo.lastWrite.written==wrote);
  resetUpdateScreen();assert(updateInfo.state==updater::State::Idle&&updateInfo.lastWrite.valid&&updateInfo.lastWrite.offset==f.offset&&updateInfo.lastWrite.error==error);
  assert(!beginCalls&&!bootCalls&&!ESP.restarted);preserved();++cases;
 }
 // Running the real network-test entry points cannot overwrite the stored
 // SD failure. Its three probes remain read-only, then reopening recalls it.
 {const auto f=updateWorker.lastWrite;assert(f.valid);assert(startNetworkTest());for(unsigned i=0;i<3;++i)replies.push_back({500,{}});testNetwork();tickUpdater();assert(updateInfo.tested&&updateInfo.netTrials==3&&updateInfo.netSuccess==0&&updateInfo.lastWrite.valid&&updateInfo.lastWrite.offset==f.offset&&updateInfo.lastWrite.written==f.written);resetUpdateScreen();assert(updateInfo.lastWrite.valid&&updateInfo.lastWrite.error==f.error);}
 // Recovery path remains unchanged: a later clean retry succeeds and never
 // promotes the torn file left by the injected short write.
 shortWritePath.clear();reset();cached();assert(installUpdate(false)&&ESP.restarted&&flashed==firmware);preserved();
 // Diagnostics do not create false short-write evidence for network failures.
 reset();for(unsigned i=0;i<5;++i)replies.push_back({500,{}});assert(!installUpdate(false)&&!updateWorker.lastWrite.valid);preserved();
 // Direct download callers without an installer preflight report unknown free space.
 reset();updateInitialFreeKnown=false;shortWritePath="/RPGPOKET/firmware.part";shortWriteAt=0;shortWriteBytes=1;shortWriteErrno=0;replies.push_back({200,firmware});
 assert(!downloadUpdate(updatePlan.firmware,shortWritePath.c_str(),0,25)&&!updateWorker.lastWrite.freeKnown);preserved();shortWritePath.clear();
 // New preparation must not retain the prior file label while the task starts.
 reset();updateInfo.state=updater::State::Available;updateWorker.file=updater::DownloadFile::Art;assert(startUpdate(true));assert(updateWorker.file==updater::DownloadFile::None&&updateWorker.state==updater::State::Downloading);
 printf("PASS: %u SD short-write diagnostic cases; firmware/art, fresh/resume, exact offset/requested/written/errno/preflight space, RAM recall, no boot/save changes; clean retry/network errors unchanged.\n",cases);
}
