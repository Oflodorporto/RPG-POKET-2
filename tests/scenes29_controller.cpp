#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main previousTitleController
#include "title_controller.cpp"
#undef main
static void reset(unsigned slot,unsigned city,unsigned cls,unsigned seed){nvs=Store{};backend.slot=slot;assert(journal.load(game)==rpg::Load::Empty);game=rpg::create(cls,seed);game.tutorial=true;game.originStory=true;game.originPage=8;game.city=city;game.rations=6;game.charts=game.charms=2;game.sleepKit=true;assert(journal.save(game));view=ViewState{};view.trackedLevel=game.p.level;menu=MenuState{};menu.activeSlot=slot;menu.sessionStarted=true;combatFx=CombatFx{};now=100;}
static void click(){tapped(175,290);}
static void checkpoint(){auto expected=game;assert(rpg::persistentValid(game));selectSlot(backend.slot);assert(!memcmp(&game,&expected,sizeof(game)));}
int main(){unsigned trips=0,states=0;
 for(unsigned origin=0;origin<4;++origin)for(unsigned dest=0;dest<4;++dest)if(origin!=dest)for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls)for(unsigned seed=1;seed<5;++seed){
 reset(slot,origin,cls,seed);view.page=Page::TravelConfirm;view.tripDestination=dest;auto before=game;auto bytes=nvs.blobs;tapped(60,290);assert(view.page==Page::Map&&!memcmp(&game,&before,sizeof(game))&&nvs.blobs==bytes);
 view.page=Page::TravelConfirm;nvs.fail=true;click();assert(view.page==Page::SaveError&&game.tripStage==4);auto committed=game;tickTravelProgress(now+9000);assert(!memcmp(&committed,&game,sizeof(game)));nvs.fail=false;tapped(100,269);assert(view.page==Page::TravelRoll&&!memcmp(&committed,&game,sizeof(game)));checkpoint();
 for(unsigned steps=0;rpg::dailyTrip(game)&&steps<80;++steps){++states;checkpoint();
  if(view.page==Page::TravelRoll||view.page==Page::CampRoll){auto saved=game;click();assert(!memcmp(&saved,&game,sizeof(game)));now+=1800;tickTravelProgress(now);assert(menu.rollReady);click();}
  else if(view.page==Page::Travel&&game.daily.phase==rpg::DailyPhase::Leg){now+=Journey::escortDuration;tickTravelProgress(now);}
  else if(view.page==Page::Battle){game.enemyHp=0;rpg::finish(game);savedTransition(currentPage());assert(view.page==Page::Result);}
  else if(view.page==Page::Result){tapped(120,290);}
  else if(view.page==Page::CampRest){auto saved=game;now+=1999;tickTravelProgress(now);assert(view.page==Page::CampRest&&!memcmp(&saved,&game,sizeof(game)));now+=1;tickTravelProgress(now);assert(view.page==Page::Travel&&game.daily.phase==rpg::DailyPhase::Tent);auto paused=game;tickTravelProgress(now+100000);assert(!memcmp(&paused,&game,sizeof(game)));click();}
  else {assert(view.page==Page::Travel&&game.daily.phase==rpg::DailyPhase::Tent);click();}
  assert(view.page!=Page::SaveError);}
 assert(!rpg::dailyTrip(game)&&game.city==dest&&game.rations==6-rpg::dailyTripDays(origin,dest));++trips;
 }
 // Letter escort departure always uses approved preflight; either danger's
 // flee ends the trip and leaves the original mission actionable for retry.
 for(bool night:{false,true})for(bool fail:{false,true}){bool covered=false;for(unsigned seed=1;seed<200&&!covered;++seed){reset(0,1,0,seed);game.p.xp=rpg::dndXp[4];rpg::levelUp(game);game.p.hp=game.p.maxhp;game.p.mp=game.p.maxmp;assert(rpg::offerEvent(game,20733,10));game.eventKind=2;assert(!rpg::acceptEvent(game,int(Page::Village)));assert(journal.save(game));view.page=Page::EventMission;tapped(120,235);assert(view.page==Page::TravelConfirm&&!game.tripStage);tapped(60,290);assert(view.page==Page::EventMission);tapped(120,235);click();assert(view.page==Page::TravelRoll&&game.tripStage==4);menu.rollReady=true;click();
  if(night){if(view.page!=Page::Travel)continue;now+=Journey::escortDuration;tickTravelProgress(now);assert(view.page==Page::CampRoll);menu.rollReady=true;click();if(view.page!=Page::Battle)continue;}else if(view.page!=Page::Battle)continue;
  game.phase=rpg::Phase::Fled;savedTransition(Page::Result);auto oldGold=game.p.gold,oldXP=game.p.xp;nvs.fail=fail;tapped(120,290);if(fail){assert(view.page==Page::SaveError);nvs.fail=false;tapped(120,269);}assert(view.page==Page::EventMission&&game.phase==rpg::Phase::Home&&!game.tripStage&&game.eventStage==2&&!game.eventProgress&&game.city==1&&game.p.gold==oldGold&&game.p.xp==oldXP);tapped(120,235);assert(view.page==Page::TravelConfirm);covered=true;}assert(covered);}
 // A failed escort can recover using the normal charged/D20 camp flow while
 // retaining its mission. Restart and failed-save retry never grant free rest.
 for(unsigned seed=1;seed<30;++seed){reset(0,1,0,seed);game.p.xp=rpg::dndXp[4];rpg::levelUp(game);game.p.hp=1;game.p.mp=0;assert(rpg::offerEvent(game,20733,10));game.eventKind=2;assert(!rpg::acceptEvent(game,int(Page::Village)));assert(journal.save(game));checkpoint();assert(view.page==Page::EventMission);tapped(60,290);assert(view.page==Page::Recovery);tapped(120,168);assert(view.page==Page::CampSetup);menu.campRation=true;menu.campKit=true;auto rations=game.rations;click();assert(view.page==Page::CampRoll&&game.rations==rations-1);checkpoint();menu.rollReady=true;click();if(view.page==Page::Battle){game.enemyHp=0;rpg::finish(game);savedTransition(currentPage());assert(view.page==Page::Result);tapped(120,290);}assert(view.page==Page::CampRest);checkpoint();nvs.fail=true;now+=4500;tickTravelProgress(now);assert(view.page==Page::SaveError);auto cured=game;nvs.fail=false;tapped(120,269);assert(view.page==Page::EventMission&&game.eventStage==2&&!game.eventProgress&&game.rations==rations-1&&game.p.hp==game.p.maxhp&&!memcmp(&game,&cured,sizeof(game)));tapped(120,235);assert(view.page==Page::TravelConfirm);}
 // Notice waits for durable save and final combat animation, never recreates XP.
 reset(0,0,0,99);game.p.xp=2700;rpg::levelUp(game);auto growth=game;nvs.fail=true;savedTransition(Page::Home);assert(view.page==Page::SaveError&&!offerLevelNotice());nvs.fail=false;tapped(120,269);assert(view.levelPending);combatFx.start(Effect::Slash,false,now,{});assert(!offerLevelNotice());combatFx.kind=Effect::None;assert(offerLevelNotice()&&view.page==Page::LevelUp);auto blobs=nvs.blobs;tapped(65,288);assert(view.page==Page::Evolution&&view.levelAttributes);tapped(60,290);assert(view.page==Page::Home&&!view.levelAttributes);assert(blobs==nvs.blobs&&!memcmp(&growth,&game,sizeof(game))&&!offerLevelNotice());
 // Fresh hero guide marker must follow the first durable hero, including retry.
 nvs=Store{};backend.slot=2;assert(journal.load(game)==rpg::Load::Empty);view=ViewState{};view.page=Page::Clothes;menu=MenuState{};nvs.fail=true;click();assert(view.page==Page::SaveError&&view.createHintsPending&&!nvs.blobs.count("h2"));nvs.fail=false;tapped(120,269);assert(view.page==Page::Prologue&&!view.createHintsPending&&nvs.blobs.count("h2"));
 // A visible defeat control is the only active acknowledgement (old area inert).
 reset(0,0,0,7);rpg::begin(game,1);game.p.hp=0;rpg::finish(game);savedTransition(Page::Result);auto lost=game;tapped(120,290);assert(view.page==Page::Result&&!memcmp(&lost,&game,sizeof(game)));tapped(120,199);assert(view.page==Page::Recovery&&game.p.hp==1);
 printf("PASS: %u real Save29 controller trips, %u checkpointed states; per-day animation,2sec rest/manualContinue, retries,reload,level queue,guide-after-save,visible defeat controls.\n",trips,states);
}
