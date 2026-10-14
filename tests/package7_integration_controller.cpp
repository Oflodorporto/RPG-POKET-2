#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main historicalPackage7ControllerMain
#include "title_controller.cpp"
#undef main
#include "SceneCanvas.h"
#include "Fatigue7Trace.h"

static void tap7(scenicUi::Rect r){tapped(r.x+r.w/2,r.y+r.h/2);}
static void setup7(unsigned slot,unsigned cls,unsigned seed=73){
 nvs=Store{};backend.slot=slot;assert(journal.load(game)==rpg::Load::Empty);
 game=rpg::create(cls,seed);game.p.xp=rpg::dndXp[4];rpg::levelUp(game);game.city=1;game.tutorial=true;game.originStory=true;game.originPage=8;game.rations=3;game.sleepKit=true;game.p.hp=1;game.p.mp=0;game.fatigue=70;game.crystals=2;assert(journal.save(game));
 menu=MenuState{};menu.sessionStarted=true;menu.activeSlot=slot;menu.clockValid=true;strcpy(menu.clockTime,"23:18:12");strcpy(menu.clockDate,"14/10/2026");
 view=ViewState{};view.page=Page::Ruins;combatFx=CombatFx{};now=100;enemyAt=0;menu.journey=Journey{};
}
static void readOnly7(const char* image=nullptr){
 auto before=game;auto blobs=nvs.blobs;unsigned seq=journal.seq;Canvas c;
 for(unsigned frame:{0u,6u,12u,18u,23u}){menu.renderNow=frame*120;render(c,game,view,frame);}
 assert(!memcmp(&before,&game,sizeof(game))&&nvs.blobs==blobs&&journal.seq==seq);
 if(image)c.save(std::string("build/package7-focused/")+image+".ppm");
}
static void idleReturn7(){
 Page page=view.page;auto before=game;auto blobs=nvs.blobs;auto food=menu.campRation;auto cityPage=view.cityPage;
 lastActivity=now;now+=60000;assert(tickIdleClock(now,false)&&view.page==Page::Clock);readOnly7();
 tapped(120,120);assert(view.page==page&&view.cityPage==cityPage&&menu.campRation==food);
 assert(!memcmp(&before,&game,sizeof(game))&&nvs.blobs==blobs);
}
static void hitRetry7(unsigned slot){
 assert(game.phase==rpg::Phase::Hero);assert(journal.save(game));view.page=currentPage();
 auto expected=game;auto fresh=game;fresh.fatigue=0;rpg::CombatTrace receipt,neutral;
 auto move=game.p.cls?rpg::Action::Attack:rpg::Action::BurningHands;
 assert(!rpg::act(expected,move,&receipt));assert(!rpg::act(fresh,move,&neutral));
 unsigned penalty=rpg::fatigueCombatPenalty(game);assert(receipt.net==(neutral.net?std::max(1,int(neutral.net)-int(penalty)):0));
 unsigned deductions=0;for(unsigned i=0;i<receipt.count;++i)deductions+=receipt.steps[i].kind==rpg::TraceStep::Fatigue;
 assert(deductions==unsigned(penalty&&neutral.net));assert(expected.randomState==fresh.randomState);
 nvs.fail=true;action(move);assert(view.page==Page::SaveError&&!combatFx.active());
 assert(!memcmp(&game,&expected,sizeof(game))&&sameFatigue7Trace(receipt,view.heroTrace));auto blobs=nvs.blobs;auto seq=journal.seq;
 for(unsigned i=0;i<8;++i){tapped(40,12);tickEnemyTurn(++now);tickTravelProgress(now);}assert(nvs.blobs==blobs&&journal.seq==seq&&!memcmp(&game,&expected,sizeof(game)));
 nvs.fail=false;tapped(120,285);assert(journal.seq==seq+1&&!memcmp(&game,&expected,sizeof(game)));
 now+=20;tapped(40,12);assert(view.traceOpen);readOnly7();
 for(unsigned i=0;i<8;++i){now+=100;tickEnemyTurn(now);tapped(185,259);tapped(44,259);}
 assert(!memcmp(&game,&expected,sizeof(game))&&sameFatigue7Trace(receipt,view.heroTrace));
 tapped(120,295);now+=200;tapped(170,12);selectSlot(slot);
 assert(!memcmp(&game,&expected,sizeof(game))&&!view.heroTrace.valid&&!view.traceOpen);
}
int main(){
 std::vector<uint8_t> art(ART_BYTES);artMemory=art.data();makeFallback();unsigned camps=0,crossings=0;
 for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls)for(bool food:{false,true}){
  setup7(slot,cls);readOnly7(slot==0&&cls==0&&!food?"ruins-page1":nullptr);
  tap7(scenicUi::ruinsPager);assert(view.cityPage==1);readOnly7(slot==0&&cls==0&&!food?"ruins-page2":nullptr);
  tap7(scenicUi::ruinsButton(4));assert(view.page==Page::CampSetup);if(food)tapped(120,220);
  readOnly7(slot==0&&cls==0&&food?"camp-meal":nullptr);idleReturn7();assert(view.page==Page::CampSetup&&view.cityPage==1&&menu.campRation==food);
  auto expected=game;assert(!rpg::startCamp(expected,food,false));nvs.fail=true;tapped(180,290);
  assert(view.page==Page::SaveError&&!memcmp(&game,&expected,sizeof(game)));auto seq=journal.seq;
  for(unsigned i=0;i<8;++i){tickTravelProgress(++now);tapped(180,290);}assert(!memcmp(&game,&expected,sizeof(game))&&journal.seq==seq);
  nvs.fail=false;tapped(120,285);assert(view.page==Page::CampRoll&&journal.seq==seq+1&&!memcmp(&game,&expected,sizeof(game)));
  now=menu.rollStarted+1800;tickTravelProgress(now);tapped(120,285);
  if(game.campStage==2){assert(view.page==Page::Battle);game.enemyHp=0;rpg::finish(game);view.page=Page::Result;tapped(120,290);}
  assert(view.page==Page::CampRest&&game.campStage==3&&game.rations==3-unsigned(food));readOnly7();
  expected=game;assert(rpg::finishCamp(expected));now=menu.campStarted+4499;tickTravelProgress(now);assert(game.campStage==3);
  ++now;tickTravelProgress(now);assert(view.page==Page::Ruins&&view.cityPage==1&&!memcmp(&game,&expected,sizeof(game)));seq=journal.seq;
  now+=9000;tickTravelProgress(now);assert(!memcmp(&game,&expected,sizeof(game))&&journal.seq==seq);idleReturn7();++camps;
 }
 for(unsigned cls=0;cls<4;++cls)for(unsigned start:{66u,67u,85u,86u,96u}){
  setup7(0,cls);game.p.mp=game.p.maxmp;game.fatigue=start;assert(journal.save(game));auto expected=game;
  bool begun=rpg::startDiscovery(expected,false);tap7(scenicUi::ruinsButton(3));assert(begun&&!memcmp(&expected,&game,sizeof(game)));
  // The discovery route can be non-combat; test actual encounter crossings separately.
  setup7(0,cls);game.p.mp=game.p.maxmp;game.fatigue=start;assert(rpg::explore(game));assert(game.fatigue==start+4);hitRetry7(0);++crossings;
 }
 for(unsigned cls=0;cls<4;++cls)for(unsigned start:{69u,70u,88u,89u,99u}){
  setup7(1,cls);game.p.mp=game.p.maxmp;game.fatigue=start;assert(journal.save(game));
  tap7(scenicUi::ruinsButton(1));assert(view.page==Page::DungeonEntry);tapped(180,290);assert(view.page==Page::Dungeon&&game.crystals==1&&game.dungeonGenerator==1);
  // A historical fixed map gives a reproducible one-step crossing. The generated paid entry above remains independently checked.
  setup7(1,cls);game.p.mp=game.p.maxmp;game.fatigue=start;assert(!rpg::enterDungeon(game,0));rpg::dungeonFace(game,1);
  assert(!rpg::dungeonMove(game,1,0,0)&&game.fatigue==start+1);assert(!rpg::dungeonMove(game,1,0,0)&&game.phase==rpg::Phase::Hero&&game.fatigue==start+1);hitRetry7(1);++crossings;
 }
 artMemory=nullptr;printf("PASS package7 integrated: %u Ruins page/camp/idle-return flows and %u crossing/receipt/save-retry/reload flows; all classes, three slots, exact game/RNG parity and one-time charges.\n",camps,crossings);
}
