#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main historicalControllerMain
#include "title_controller.cpp"
#undef main
#include "../firmware/RPG_POKET_2/Persistence.h"
static std::vector<uint8_t> state(){std::vector<uint8_t> b(rpg::PERSISTENT_SAVE_SIZE);rpg::encodePersistent(game,1,0,b.data());return b;}
static void setup(){nvs=Store{};backend.slot=0;assert(journal.load(game)==rpg::Load::Empty);game=rpg::create(0,99);game.city=1;game.crystals=3;game.tutorial=true;game.originStory=true;game.originPage=8;assert(journal.save(game));menu=MenuState{};menu.sessionStarted=true;view=ViewState{};view.page=Page::DungeonEntry;combatFx=CombatFx{};now=100;}
int main(){
 for(bool failed:{false,true})for(bool reboot:{false,true}){
  setup();const auto old=state();auto rng=game.randomState;nvs.fail=failed;tapped(180,290);assert(game.crystals==2&&game.dungeonGenerator==1&&game.dungeonRun==1&&game.randomState==rng);
  const auto entered=state();const auto seed=game.dungeonSeed;const auto seq=journal.seq;
  if(failed){assert(view.page==Page::SaveError);for(auto point:{std::pair<int,int>{120,218},{80,300},{195,300},{180,290}})tapped(point.first,point.second);assert(state()==entered&&journal.seq==seq);nvs.fail=false;
    if(reboot){selectSlot(0);assert(state()==old&&!rpg::inDungeon(game));view.page=Page::DungeonEntry;tapped(180,290);assert(state()==entered&&game.dungeonSeed==seed);}
    else{tapped(100,265);assert(state()==entered&&journal.seq==seq+1);}}
  assert(view.page==Page::Dungeon);auto saved=nvs.blobs;
  // Existing bag, menu, idle clock, and reboot are pauses, not new expeditions.
  tapped(80,300);assert(view.page==Page::Bag&&state()==entered);tapped(40,294);assert(view.page==Page::Dungeon);
  tapped(195,300);assert(view.page==Page::DungeonMenu);tapped(80,290);assert(view.page==Page::Dungeon&&state()==entered);
  lastActivity=now;now+=60001;assert(tickIdleClock(now,false)&&view.page==Page::Clock);tapped(100,100);assert(view.page==Page::Dungeon&&state()==entered&&nvs.blobs==saved);
  selectSlot(0);assert(view.page==Page::Dungeon&&state()==entered&&game.dungeonSeed==seed);
  assert(rpg::startCamp(game,false,false));assert(state()==entered); // No free in-dungeon camp added.
  // A completed exit and a later paid entry advance identity exactly once.
  view.page=Page::DungeonExit;tapped(180,290);assert(view.page==Page::Ruins&&!rpg::inDungeon(game)&&game.crystals==2&&!game.dungeonGenerator&&game.dungeonSeed==seed);
  view.page=Page::DungeonEntry;tapped(180,290);assert(view.page==Page::Dungeon&&game.crystals==1&&game.dungeonRun==2&&game.dungeonSeed!=seed);
 }
 puts("PASS procedural6 actual-controller: entry save failure/retry/reboot, atomic cost/seed, no duplicate charge, bag/menu/idle pause, outside-only camp, exit/new run");
}
