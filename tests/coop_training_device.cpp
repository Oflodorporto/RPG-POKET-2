#define RPG_ENABLE_COOP_TRAINING 1
#include "../firmware/RPG_POKET_2/View.h"
#include "../firmware/RPG_POKET_2/TouchGate.h"
#include <cassert>
#include <cstdio>
rpg::Game game;ViewState view;TouchGate gate;CombatFx combatFx;
struct {bool blocked=false;int active=0;} journal; // No save/write API on purpose.
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
 fake::reset();game=rpg::create(0,91);view.page=Page::Guild;uint8_t before[160],after[160];rpg::encodePersistent(game,9,0,before);auto random=game.randomState;
 gate.armed=true;pendingTouch=true;CHECK(openCoopTraining());CHECK(coopIsOpen()&&view.page==Page::CoopTraining&&!pendingTouch&&!gate.armed);CHECK(!game.guildMember); // Training never buys guild membership.
 sample(true,true,280,10);release();CHECK(!coopClosing&&coopIsOpen()); // old held menu press cannot leave
 sample(true,true,280,10);release();CHECK(coopClosing&&coopIsOpen());now+=351;tickCoopTraining(now);CHECK(!coopIsOpen()&&view.page==Page::Guild&&!gate.armed&&!pendingTouch&&dirty);
 CHECK(!gate.update(true,false,now));now+=31;CHECK(!gate.update(true,false,now));CHECK(gate.update(true,true,now)); // portrait press gate re-arms only after release
 rpg::encodePersistent(game,9,0,after);CHECK(!std::memcmp(before,after,160)&&game.randomState==random&&!fake::nvsWrites&&!fake::erases);
 CHECK(openCoopTraining());coopSession.leave(now);finishCoopExit(now);CHECK(!coopIsOpen());
 updateInfo.busy=true;auto calls=fake::mutations;CHECK(!openCoopTraining()&&fake::mutations==calls);updateInfo.busy=false;
 menu.scanning=true;CHECK(!openCoopTraining());menu.scanning=false;menu.connecting=true;CHECK(!openCoopTraining());menu.connecting=false;
 buffered=false;CHECK(!openCoopTraining());buffered=true;touchReady=false;CHECK(!openCoopTraining());touchReady=true;journal.blocked=true;CHECK(!openCoopTraining());journal.blocked=false;
 CHECK(!fake::nvsWrites&&!fake::erases);std::printf("PASS: %u checks; actual CoopDevice menu-entry/exit/reentry, 350ms leave drain, portrait gate restore; campaign/RNG unchanged; no journal API available; Session=%zu Input=%zu Device=%zu Inbox=%zu\n",checks,sizeof(coopplatform::Session),sizeof(coopplatform::Input),sizeof(coopradio::Device),sizeof(coopradio::queueBytes)+sizeof(coopradio::queueState));
}
