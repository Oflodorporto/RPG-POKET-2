// Exercise the same controller bodies checked against the actual sketch.
#define main existingControllerRegression
#include "title_controller.cpp"
#undef main
int main(){
 firstHints.load(0);assert(!firstHints.bits); // Existing characters opt out, even at level 1.
 game=rpg::create(0,42);game.tutorial=game.originStory=true;game.originPage=8;
 journal.load(game);game=rpg::create(0,42);game.tutorial=game.originStory=true;game.originPage=8;assert(journal.save(game));
 menu.sessionStarted=true;view.page=Page::Home;assert(!offerFirstHint());
 // Missing, malformed or unknown sidecar metadata never blocks or edits a hero.
 for(uint32_t raw:{0u,1u,0xffffffffu,firstExperience::pack(0x4000)}){nvs.hints[0]=raw;firstHints.load(0);assert(!firstHints.bits);}
 firstHints.start(0);assert(firstHints.pending(0));
 uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(game,1,before);auto blobs=nvs.blobs;
 const Page pages[]={Page::Home,Page::Menu,Page::Explore,Page::Map,Page::Battle,Page::TownBag,Page::CampSetup,Page::Campaign,Page::Character,Page::Dungeon};
 for(unsigned i=0;i<firstExperience::count;++i){
  view.page=pages[i];if(i==4){assert(rpg::begin(game,0));assert(journal.save(game));blobs=nvs.blobs;rpg::encode(game,1,before);}
  assert(offerFirstHint()&&view.page==Page::FirstTip&&view.hintIndex==i&&view.hintReturn==pages[i]);
  tapped(120,140);assert(view.page==Page::FirstTip); // Scene touch does not choose an action.
  firstHints.load(0);assert(firstHints.pending(i)); // Restart before acknowledgement keeps pending hint.
  nvs.fail=true;tapped(175,290);assert(view.page==Page::FirstTip&&view.hintError&&firstHints.pending(i));
  nvs.fail=false;tapped(175,290);assert(view.page==pages[i]&&!firstHints.pending(i));
  firstHints.load(0);assert(!firstHints.pending(i)&&!offerFirstHint());
  rpg::encode(game,1,after);assert(!memcmp(before,after,sizeof(before))&&blobs==nvs.blobs);
 }
 assert((firstHints.bits&firstExperience::all)==firstExperience::all);
 // Three independent slots, skip persists, stale flags reset only on explicit new creation.
 firstHints.start(1);assert(firstHints.pending(0));assert(firstHints.acknowledge(0,true));firstHints.load(1);assert(!firstHints.pending(1));
 firstHints.start(2);firstHints.load(0);assert(!firstHints.pending(2));firstHints.load(2);assert(firstHints.pending(2));
 firstHints.start(1);assert(firstHints.pending(0));
 game=rpg::create(0,42);game.tutorial=true;game.originStory=true;game.originPage=8;view.page=Page::Battle;game.phase=rpg::Phase::Enemy;assert(!offerFirstHint());
 game.phase=rpg::Phase::Home;view.page=Page::Home;game.originPage=7;assert(!offerFirstHint());game.originPage=8;combatFx.start(Effect::Slash,false,now);assert(!offerFirstHint());combatFx.kind=Effect::None;
 menu.sessionStarted=false;assert(!offerFirstHint());menu.sessionStarted=true;
 // Creation -> saved origin page -> optional skip -> welcome -> no hints, with original narrative replay intact.
 nvs.blobs.clear();nvs.tomb[0]=false;backend.slot=0;journal.load(game);view.page=Page::Clothes;view.choice=2;
 tapped(175,290);assert(view.page==Page::Prologue&&game.p.level==1&&game.originPage==0);
 tapped(175,290);assert(game.originPage==1);selectSlot(0);assert(view.page==Page::Prologue&&menu.storyIndex==1);
 tapped(120,259);assert(view.page==Page::Help&&game.originPage==8&&!game.tutorial);
 tapped(65,290);assert(view.page==Page::Village&&game.tutorial&&!firstHints.pending(0));selectSlot(0);assert(view.page==Page::Home&&!offerFirstHint());
 for(unsigned i=0;i<firstExperience::count;++i){unsigned lines=story::wrapStory(firstExperience::tips[i].text,33,[](unsigned,const char* s){assert(strlen(s)<=33);});assert(lines<=9);}
 puts("PASS: first-experience controller, all 10 contexts, skip/restart, three slots, failed hint writes, unchanged Save28/gameplay and creation checkpoints");
}
