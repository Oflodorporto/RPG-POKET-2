// The installer is compiled separately. This TU owns the unchanged sketch helpers.
#include "Save.h"
#include "AssetCatalog.h"
#include "FakeDevice.h"
enum class ArtStatus {Ready,Missing};
struct {bool scanning=false,connecting=false;ArtStatus art=ArtStatus::Ready;} menu;
#include "UpdateDevice.h"

int main(){
  assert(updateSpi==nullptr&&updateGuard==nullptr);
  assert(updateWorker.state==updater::State::Idle&&updateWorker.progress==0&&!updateWorker.busy);
  assert(updateWorker.minDbm==-100&&updateWorker.maxDbm==-100);
  assert(!updateWorker.lastWrite.valid&&!updateWorker.lastVerify.valid());
  assert(updatePlan.build==0&&updatePlan.artCrc==0&&updatePlan.assets==0);
  assert(updatePlan.firmware.bytes==0&&updatePlan.art.bytes==0);
  assert(!*updatePlan.version&&!*updatePlan.firmware.sha&&!*updatePlan.art.url);
  assert(updateSdHz==4000000&&updateInitialFree==0&&!updateInitialFreeKnown);
  assert(updateRecovery.attempts==0&&!updateRecovery.recovered&&!updateRecovery.first.valid());
  for(char byte:downloadReason)assert(byte==0);
  PartialUpdate partial;assert(partial.bytes==0&&!partial.uncertain&&!partial.failure.valid());
  {UpdateSpiLock lock;}
  initUpdater();assert(updateSpi&&updateGuard);
  updateReport(updater::State::Checking,"defaults");
  assert(updateWorker.progress==0&&updateWorker.busy&&!strcmp(updateWorker.message,"defaults"));
  updateReport(updater::State::Checking,"explicit",37,false);
  assert(updateWorker.progress==37&&!updateWorker.busy);
  assert(tickUpdater()&&updateInfo.progress==37);
  // The separately linked installer must see and reset the same shared states.
  // A mount failure is deterministic and exercises the task's return path.
  for(bool sleep:{false,true}){
    updateWorker={};updatePlan={};updatePlan.build=FW_BUILD;WiFi.sleep=sleep;
    updateInitialFree=123;updateInitialFreeKnown=true;updateSdHz=8;
    updateRecovery.attempts=9;card=false;sdMounts=0;
    updateInstallTask(nullptr);
    assert(WiFi.sleep==sleep&&!ESP.restarted&&!beginCalls&&!bootCalls);
    assert(!updateInitialFreeKnown&&updateInitialFree==0&&updateSdHz==1000000);
    assert(updateRecovery.attempts==1&&updateWorker.recovery.attempts==1&&sdMounts==2);
    assert(updateWorker.state==updater::State::Error&&!updateWorker.busy);
    assert(updateWorker.lastVerify.issue==storageDiag::Issue::Mount);
    assert(!strcmp(updateWorker.message,"Cartao indisponivel / outro cartao"));
    assert(!openFiles&&!otaOpen);
  }
  puts("PASS: separate-TU initializers, defaults, shared state updates, bounded mount recovery, and both Wi-Fi sleep restoration paths");
}
