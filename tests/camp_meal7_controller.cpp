#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main unusedCampMealTitleMain
#include "title_controller.cpp"
#undef main
#include "SceneCanvas.h"

std::vector<uint8_t> encoded(const rpg::Game& g){std::vector<uint8_t> b(rpg::PERSISTENT_SAVE_SIZE);rpg::encodePersistent(g,1,0,b.data());return b;}
void readOnlyFrames(){auto before=encoded(game);auto saved=nvs.blobs;auto rng=game.randomState;Canvas c;for(unsigned frame=0;frame<48;++frame)render(c,game,view,frame);assert(encoded(game)==before&&nvs.blobs==saved&&game.randomState==rng);}
int main(){
 std::vector<uint8_t> art(ART_BYTES);artMemory=art.data();makeFallback();
 unsigned camps=0;
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race)for(bool food:{false,true})for(bool kit:{false,true}){
  nvs=Store{};backend.slot=0;assert(journal.load(game)==rpg::Load::Empty);game=rpg::create(cls,73);game.race=race;game.tutorial=true;game.originStory=true;game.originPage=8;game.p.hp=1;game.p.mp=0;game.fatigue=65;game.rations=3;game.sleepKit=true;assert(journal.save(game));view=ViewState{};menu=MenuState{};menu.art=ArtStatus::Fallback;menu.sessionStarted=true;combatFx=CombatFx{};openCamp();
  auto initial=encoded(game);auto durable=nvs.blobs;
  for(unsigned repeat=0;repeat<4;++repeat){tapped(120,220);readOnlyFrames();assert(encoded(game)==initial&&nvs.blobs==durable);}
  assert(!menu.campRation);if(food)tapped(120,220);if(kit)tapped(120,255);readOnlyFrames();assert(encoded(game)==initial&&nvs.blobs==durable);
  // Back/cancel cannot commit food, kit, seed, roll or a save record.
  tapped(60,290);assert(!game.campStage&&encoded(game)==initial&&nvs.blobs==durable);openCamp();if(food)tapped(120,220);if(kit)tapped(120,255);
  now+=2000;tapped(180,290);assert(view.page==Page::CampRoll&&game.campStage==1&&game.rations==3-unsigned(food));auto rolled=encoded(game);auto roll=game.campRoll;auto rng=game.randomState;readOnlyFrames();
  for(unsigned i=0;i<3;++i)tapped(120,285);
  assert(encoded(game)==rolled&&game.campRoll==roll&&game.randomState==rng);
  now=menu.rollStarted+1800;tickTravelProgress(now);tapped(120,285);
  if(game.campStage==2){assert(view.page==Page::Battle);game.enemyHp=0;rpg::finish(game);view.page=Page::Result;tapped(120,290);}
  assert(view.page==Page::CampRest&&game.campStage==3);auto committed=encoded(game);readOnlyFrames();assert(game.rations==3-unsigned(food));
  // Resume keeps the committed option even though volatile menu choices reset.
  selectSlot(0);assert(view.page==Page::CampRest&&encoded(game)==committed);menu.campRation=!food;readOnlyFrames();
  now=menu.campStarted+4499;tickTravelProgress(now);assert(game.campStage==3&&encoded(game)==committed);
  now+=1;tickTravelProgress(now);assert(!game.campStage&&game.rations==3-unsigned(food));auto complete=encoded(game);now+=9000;tickTravelProgress(now);assert(encoded(game)==complete);++camps;
 }
 // A selected preview cannot conjure food when stock is zero.
 game=rpg::create(0,42);game.p.hp=1;game.rations=0;view=ViewState{};menu=MenuState{};openCamp();auto original=encoded(game);tapped(120,220);assert(!menu.campRation&&encoded(game)==original);readOnlyFrames();
 artMemory=nullptr;printf("PASS: %u camp meal controller paths, cosmetic previews/cancel, committed one-time ration, early taps, post-ambush meal, resume, unchanged RNG/journal, exact rest timer and no double recovery.\n",camps);
}
