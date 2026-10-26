#define RPG_ENABLE_COOP_TRAINING 1
#include "../firmware/RPG_POKET_2/View.h"
#include "../firmware/RPG_POKET_2/TouchGate.h"
#include "../firmware/RPG_POKET_2/Coop/CoopRadioDevice.h"
#include "../firmware/RPG_POKET_2/Coop/CoopInput.h"
#include <cassert>
#include <cstdio>
rpg::Game game;ViewState view;TouchGate gate;CombatFx combatFx;
struct {bool blocked=false;int active=0;} journal;
bool buffered=true,touchReady=true,lcdReady=true,pendingTouch=false,dirty=false;
uint32_t now=100,lastActivity=0;
uint32_t millis(){return now;}
uint32_t esp_random(){return 0x12345678u+now;}
void say(const char* s){view.message=s;}
#include "../firmware/RPG_POKET_2/CoopDevice.h"
static unsigned checks=0;
#define CHECK(x) do{++checks;assert(x);}while(0)
static void sample(bool valid,bool touching,int x,int y){auto p=coopplatform::toPortrait(x,y);captureCoopTouch(valid,touching,p.x,p.y,now);tickCoopTraining(now);}
static void release(){sample(true,false,0,0);now+=31;sample(true,false,0,0);}

int main(){
 for(int fault=1;fault<=7;++fault){
  if(fault==7)now=0xfffffff0u;
  fake::reset();if(fault!=7){fake::mode=WIFI_MODE_STA;fake::connected=true;}
  game=rpg::create(0,91);view.page=Page::Guild;
  uint8_t before[160],after[160];rpg::encodePersistent(game,9,0,before);
  CHECK(openCoopTraining());coopSession.leave(now);fake::closeFail=fault;finishCoopExit(now);
  CHECK(coopIsOpen()&&view.page==Page::CoopTraining);
  CHECK(coopruntime::view(true).cleanupPending);
  CHECK(coopview::hitTest(coopruntime::view(true),280,10)==coopview::Control::None);
  CHECK(coopradio::owner==coopradio::Owner::Training&&!coopradio::acquire(coopradio::Owner::Pvp));
  CHECK(coopradio::callbackCounters.generation()==0&&!pendingTouch&&!gate.armed);
  auto sent=fake::sendCount;now+=1000;tickCoopTraining(now);CHECK(fake::sendCount==sent&&coopIsOpen());
  // A lobby action cannot create a room while the driver cleanup is pending.
  release();sample(true,true,80,200);release();CHECK(coopIsOpen()&&coopSession.state()!=coopplatform::SessionState::Hosting);
  fake::closeFail=0;
  release();sample(true,true,160,200);release();
  CHECK(!coopIsOpen()&&view.page==Page::Guild&&coopradio::owner==coopradio::Owner::None);
  CHECK(fake::mode==(fault==7?WIFI_MODE_NULL:WIFI_MODE_STA));
  CHECK(fake::connected==(fault!=7)&&fake::autoReconnect&&fake::persistent);
  rpg::encodePersistent(game,9,0,after);CHECK(!std::memcmp(before,after,160)&&!fake::nvsWrites&&!fake::erases);
  auto mutations=fake::mutations;finishCoopExit(now);CHECK(fake::mutations==mutations);
 }
 // A callback-registration failure can itself fail rollback; show the same UI.
 fake::reset();game=rpg::create(0,91);view.page=Page::Guild;fake::fail=6;fake::closeFail=3;
 CHECK(openCoopTraining()&&coopIsOpen()&&view.page==Page::CoopTraining);
 CHECK(coopradio::owner==coopradio::Owner::Training);
 fake::fail=fake::closeFail=0;release();sample(true,true,160,200);release();
 CHECK(!coopIsOpen()&&view.page==Page::Guild&&coopradio::owner==coopradio::Owner::None);
 std::printf("PASS cleanup: %u checks; seven close faults, retained ownership/UI, retry, failed-open rollback, idempotence, unchanged Save30/RNG\n",checks);
}
