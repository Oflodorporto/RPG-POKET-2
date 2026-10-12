#define main isolatedControllerRegression
#include "title_controller.cpp"
#undef main
#include <array>

// Computer-only fixture: real controller/rules/journal, simulated NVS and time.
static unsigned checkpoints=0, failures=0, battles=0, rounds=0;
static unsigned letters=0, eventMask=0;
static void bytes(const rpg::Game& g,uint8_t* b){if(!rpg::valid(g))fprintf(stderr,"INVALID phase=%u cls=%u level=%u city=%u event=%u/%u progress=%u trip=%u dungeon=%u enemy=%u camp=%u valid:event=%d dungeon=%d camp=%d discovery=%d beat=%u hp=%u enemyHP=%u\n",unsigned(g.phase),g.p.cls,g.p.level,g.city,g.eventKind,g.eventStage,g.eventProgress,g.tripStage,g.dungeonFlags,g.enemyId,g.campStage,rpg::eventValid(g),rpg::dungeonValid(g),rpg::campValid(g),rpg::discoveryValid(g),g.enemyBeat,g.p.hp,g.enemyHp);assert(rpg::valid(g));rpg::encode(g,1,b);}
static void commitAndRestart(){
 uint8_t expected[rpg::SAVE_SIZE],actual[rpg::SAVE_SIZE];bytes(game,expected);auto expectedFatigue=game.fatigue;
 auto protectedBlobs=nvs.blobs;
 bool fail=(checkpoints%17)==0;nvs.fail=fail;
 savedTransition(currentPage());
 if(fail){assert(view.page==Page::SaveError&&nvs.blobs==protectedBlobs);nvs.fail=false;tapped(120,269);++failures;}
 if(view.page==Page::SaveError)fprintf(stderr,"SAVEERR checkpoint=%u fail=%d blocked=%d active=%d phase=%u event=%u/%u progress=%u trip=%u fx=%u\n",checkpoints,fail,journal.blocked,journal.active,unsigned(game.phase),game.eventKind,game.eventStage,game.eventProgress,game.tripStage,unsigned(combatFx.kind));assert(view.page!=Page::SaveError);bytes(game,actual);assert(!memcmp(expected,actual,sizeof(expected)));
 auto committed=nvs.blobs;combatFx.kind=Effect::None;bootTitle();assert(view.page==Page::Title&&nvs.blobs==committed);
 continueTitle();bytes(game,actual);assert(!memcmp(expected,actual,sizeof(expected))&&game.fatigue==expectedFatigue&&nvs.blobs==committed);++checkpoints;
 now+=913; // Deliberately crosses uint32 millis rollover in the same session.
}
static void battle(){
 ++battles;unsigned turns=0;
 while(game.phase==rpg::Phase::Hero||game.phase==rpg::Phase::Enemy){
  assert(++turns<1000);
  if(game.phase==rpg::Phase::Hero){
   rpg::Action a=game.p.hp<game.p.maxhp/2&&game.p.life?rpg::Action::Life:rpg::Action::Attack;
   assert(!rpg::act(game,a));
  }else assert(rpg::enemy(game));
  ++rounds;commitAndRestart();
 }
}
static void dailyLetter(unsigned day){
 assert(rpg::rest(game));assert(rpg::offerEvent(game,day,21));eventMask|=1u<<game.eventKind;++letters;commitAndRestart();
 auto campaign=game.campaignFlags,ending=game.campaignEnding,clears=game.dungeonClears,wins=game.ruinsWins,contract=game.questProgress;
 assert(!rpg::acceptEvent(game,49));commitAndRestart();unsigned steps=0;
 while(!rpg::eventReady(game)&&game.phase!=rpg::Phase::Lost&&game.phase!=rpg::Phase::Fled){
  assert(++steps<50);
  if(game.phase==rpg::Phase::Hero||game.phase==rpg::Phase::Enemy){battle();if(game.phase==rpg::Phase::Won&&game.eventKind){assert(rpg::resumeEventBattle(game));commitAndRestart();}else if(!game.eventKind)break;continue;}
  unsigned kind=game.eventKind,choice=0;
  if(kind==6){if(!game.eventProgress){for(choice=0;choice<3;++choice)if(!(game.eventFlags&(1u<<choice)))break;}else if(game.eventProgress==1)choice=game.eventTier%3;}
  assert(!rpg::eventAct(game,choice));commitAndRestart();
  if(kind==2){assert(rpg::acceptTrip(game));commitAndRestart();if(game.tripStage==2){battle();if(game.phase==rpg::Phase::Won){assert(rpg::resumeEventBattle(game));commitAndRestart();}}if(game.tripStage==3){assert(menu.journey.tick(menu.journey.started+7000));assert(rpg::arriveTrip(game));commitAndRestart();}}
  // Full dungeon traversal already has its own BFS regression; here exercise its exit/restart boundary.
  if(kind==3){assert(rpg::abandonEvent(game));break;}
 }
 assert(rpg::finishEvent(game));commitAndRestart();auto gold=game.p.gold,xp=game.p.xp;
 assert(!rpg::finishEvent(game)&&game.p.gold==gold&&game.p.xp==xp);
 assert(game.campaignFlags==campaign&&game.campaignEnding==ending&&game.dungeonClears==clears&&game.ruinsWins==wins&&game.questProgress==contract);
}
int main(){
 now=0xffff0000u;
 for(unsigned cls=0;cls<4;++cls){
  nvs=Store{};menu=MenuState{};journal.blocked=false;
  std::array<rpg::Game,3> slots;
  for(unsigned s=0;s<3;++s){backend.slot=s;rpg::Game empty;journal.load(empty);game=rpg::create(cls,71+s);game.p.xp=rpg::dndXp[(s==0?1:s==1?10:20)-1];rpg::levelUp(game);rpg::rest(game);game.tutorial=true;game.originStory=true;game.originPage=8;game.guildMember=true;assert(journal.save(game));slots[s]=game;}
  for(unsigned cycle=0;cycle<120;++cycle){
   unsigned s=cycle%3;menu.activeSlot=s;backend.slot=s;selectSlot(s);
   uint8_t old[rpg::SAVE_SIZE],loaded[rpg::SAVE_SIZE];bytes(slots[s],old);bytes(game,loaded);assert(!memcmp(old,loaded,sizeof(old))&&game.fatigue==slots[s].fatigue);
   // Actual menu/guide navigation is read-only, and returns to the same menu.
   combatFx.kind=Effect::None;view.page=Page::Menu;auto before=nvs.blobs;
   tapped(65,290);assert(view.page==Page::Guide);tapped(175,240);tapped(120,290);assert(view.page==Page::Menu&&nvs.blobs==before);
   if(cycle%4==0){assert(rpg::rest(game));commitAndRestart();}assert(rpg::startDiscovery(game,cycle%2));commitAndRestart();
   if(game.chestLock){assert(!rpg::attemptLock(game,false));commitAndRestart();}
   if(game.discovery&&game.phase==rpg::Phase::Home){
    if(!game.chestLock||game.chestLock==2){rpg::collectDiscovery(game);commitAndRestart();}
    if(game.phase==rpg::Phase::Home){rpg::clearDiscovery(game);commitAndRestart();}
   }
   if(game.phase!=rpg::Phase::Home){battle();assert(rpg::home(game));commitAndRestart();}
   if(cycle%2==0&&(game.p.hp<game.p.maxhp||game.p.mp<game.p.maxmp||rpg::powersSpent(game))){assert(!rpg::startCamp(game,game.rations!=0,false));commitAndRestart();assert(rpg::acceptCamp(game));commitAndRestart();if(game.campStage==2){battle();assert(rpg::resolveCamp(game));commitAndRestart();}if(game.campStage==3){assert(rpg::finishCamp(game));commitAndRestart();assert(!rpg::finishCamp(game));}}
   if(cycle%3==0){assert(!rpg::prepareTrip(game,(game.city+1)%4));commitAndRestart();assert(rpg::acceptTrip(game));commitAndRestart();if(game.tripStage==2){battle();assert(rpg::home(game));commitAndRestart();}if(game.tripStage==3){assert(menu.journey.active);assert(!menu.journey.tick(menu.journey.started+2999));assert(menu.journey.tick(menu.journey.started+3000));assert(rpg::arriveTrip(game));commitAndRestart();assert(!rpg::arriveTrip(game));}}
   if(cycle%5==0)dailyLetter(21000+cycle);
   slots[s]=game;
   // Loading all other slots cannot overwrite the active or any committed hero.
   auto allBlobs=nvs.blobs;for(unsigned i=0;i<3;++i){backend.slot=i;rpg::FatigueJournal<NvsBackend> probe(backend);rpg::Game h;assert(probe.load(h)==rpg::Load::Ok);bytes(h,loaded);bytes(slots[i],old);assert(!memcmp(old,loaded,sizeof(old))&&h.fatigue==slots[i].fatigue);}backend.slot=s;assert(nvs.blobs==allBlobs);
  }
 }
 // Timer rollover uses the real journey state machine.
 for(bool escort:{false,true}){Journey j;assert(j.start(0,3,0xffffff00u,escort));unsigned duration=escort?7000:3000;for(unsigned elapsed=0;elapsed<duration;++elapsed){assert(!j.tick(uint32_t(0xffffff00u+elapsed)));assert(j.progress<=1000);auto p=j.position();assert(p.x>=0&&p.x<240&&p.y>=0&&p.y<320);}assert(j.tick(uint32_t(0xffffff00u+duration))&&!j.tick(10000));}
 assert(eventMask==255);
 printf("PASS: long sessions, 480 mixed cycles / 12 heroes / 3 isolated slots; %u battles, %u real combat turns, %u committed restarts, %u write failures/retries, %u letters / all 8 types; journey rollover\n",battles,rounds,checkpoints,failures,letters);
}
