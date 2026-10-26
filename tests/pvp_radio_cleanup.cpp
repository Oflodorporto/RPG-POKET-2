#include "pvp_save30_fixture.h"
static unsigned checks=0;
#define CHECK(value) do { ++checks; assert(value); } while(0)
static void fresh(bool connected=true,bool enabled=true){
 CHECK(!clubRadioState.lease);
#if RPG_ENABLE_COOP_TRAINING
 CHECK(coopradio::owner==coopradio::Owner::None);
#endif
 clubFake::reset();clubFake::mode=enabled?WIFI_MODE_STA:WIFI_MODE_NULL;clubFake::channel=11;clubFake::connected=connected;
 clubQueue=nullptr;clubRadioReceiver=nullptr;arena=ClubUi{};view.page=Page::Guild;
 game=testHero(1,41);game.p.level=5;game.guildMember=true;game.p.gold=100;
}
static void unchanged(const std::vector<uint8_t>& before,uint32_t random,unsigned writes){
 CHECK(packed(game)==before&&game.randomState==random&&nvs.primaryWrites==writes);
}
int main(){
 for(int failure=1;failure<=10;++failure){
  fresh();auto before=packed(game);auto random=game.randomState;auto writes=nvs.primaryWrites;
  clubFake::openFail=failure;
  CHECK(!openClub());CHECK(!arena.opened&&!arena.cleanupPending&&!clubRadioState.lease);
  CHECK(!clubRadioReceiver&&!clubFake::radio&&clubQueue==nullptr);
  CHECK(clubFake::mode==WIFI_MODE_STA&&clubFake::channel==11&&clubFake::storage==WIFI_STORAGE_FLASH);
  CHECK(clubFake::connected&&clubFake::autoReconnect);
#if RPG_ENABLE_COOP_TRAINING
  CHECK(coopradio::owner==coopradio::Owner::None);
#endif
  unchanged(before,random,writes);
 }
 fresh();auto before=packed(game);auto random=game.randomState;auto writes=nvs.primaryWrites;
 CHECK(openClub()&&arena.opened&&clubRadioReceiver&&clubFake::radio);
#if RPG_ENABLE_COOP_TRAINING
 CHECK(coopradio::owner==coopradio::Owner::Pvp);
#endif
 CHECK(closeClub()&&!arena.opened&&!clubRadioState.lease&&!clubRadioReceiver&&!clubFake::radio);
 unchanged(before,random,writes);
 for(int failure=1;failure<=7;++failure){
  const bool enabled=failure!=5;fresh(true,enabled);before=packed(game);random=game.randomState;writes=nvs.primaryWrites;
  if(failure==7)clubFake::autoReconnect=false;
  CHECK(openClub());view.page=Page::Club;clubFake::closeFail=failure;
  clubTap(20,285);CHECK(arena.cleanupPending&&!arena.opened&&view.page==Page::Club&&clubRadioState.lease);
#if RPG_ENABLE_COOP_TRAINING
  CHECK(coopradio::owner==coopradio::Owner::Pvp);
#endif
  CHECK(!openClub());auto unregisterCalls=clubFake::unregisterCalls;
  clubTap(20,100);CHECK(arena.cleanupPending&&view.page==Page::Club);
  clubFake::closeFail=0;clubTap(20,285);
  CHECK(view.page==Page::Guild&&!arena.cleanupPending&&!clubRadioState.lease&&!clubRadioReceiver&&!clubFake::radio);
  if(failure!=1)CHECK(clubFake::unregisterCalls==unregisterCalls);
  unchanged(before,random,writes);
 }
 fresh();before=packed(game);random=game.randomState;writes=nvs.primaryWrites;
 clubFake::openFail=6;clubFake::closeFail=2;
 CHECK(!openClub()&&arena.cleanupPending&&clubRadioState.lease&&!arena.opened&&clubFake::radio);
 CHECK(view.page==Page::Club&&!clubRadioReceiver&&!openClub());
 clubFake::openFail=clubFake::closeFail=0;clubTap(20,285);
 CHECK(view.page==Page::Guild&&!clubRadioState.lease&&!clubFake::radio);
 unchanged(before,random,writes);
 std::printf("PASS: %u checks; ten PvP open faults, seven close faults, failed-open rollback retry, owner/UI fencing, Save30/RNG unchanged\n",checks);
}
