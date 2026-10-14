#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main priorControllerRegression
#include "title_controller.cpp"
#undef main

static std::vector<uint8_t> saved(const rpg::Game& g){std::vector<uint8_t>b(rpg::PERSISTENT_SAVE_SIZE);rpg::encodePersistent(g,17,0,b.data());return b;}
static void seed(unsigned cls,bool origin,unsigned mask){
 nvs=Store{};backend.slot=0;assert(journal.load(game)==rpg::Load::Empty);
 game=rpg::create(cls,77);game.p.xp=265000;rpg::levelUp(game);game.p.hp=game.p.maxhp;game.p.mp=game.p.maxmp;
 game.p.gold=50000;game.rations=9;game.tutorial=true;game.originStory=origin;game.originPage=origin?8:0;game.dungeonClears=1;game.city=3;game.campaignFlags=7|(mask<<3);
 assert(rpg::persistentValid(game)&&journal.save(game));view=ViewState{};view.page=Page::Dialogue;menu=MenuState{};menu.personIndex=0;combatFx=CombatFx{};now=100;
}
static void openFinale(unsigned expected){auto before=saved(game);auto blobs=nvs.blobs;tapped(120,240);assert(view.page==Page::CampaignTask&&view.campaignChoice==expected);assert(saved(game)==before&&blobs==nvs.blobs);tapped(50,290);assert(view.page==Page::Dialogue&&saved(game)==before);tapped(120,240);assert(view.campaignChoice==expected);}
static void winFromTask(bool fail=false,bool reboot=false){
 unsigned mission=view.campaignChoice;tapped(180,290);assert(view.page==Page::Battle&&game.campaignStage==mission);
 game.enemyHp=0;rpg::finish(game);assert(game.phase==rpg::Phase::Won&&rpg::persistentValid(game)&&journal.save(game));
 if(reboot){selectSlot(0);assert(view.page==Page::CampaignResult&&game.campaignStage==mission);}else {view.page=Page::CampaignResult;view.campaignScene=0;}
 auto before=saved(game);auto blobs=nvs.blobs;unsigned pages=story::finalePages(game,mission);assert(pages>0);
 for(unsigned page=0;page+1<pages;++page){assert(view.campaignScene==page);tapped(100,290);assert(view.page==Page::CampaignResult&&view.campaignScene==page+1&&saved(game)==before&&nvs.blobs==blobs);}
 auto expected=game;assert(rpg::resolveCampaign(expected));nvs.fail=fail;tapped(100,290);assert(saved(game)==saved(expected));
 if(fail){assert(view.page==Page::SaveError&&nvs.blobs==blobs);nvs.fail=false;if(reboot){selectSlot(0);assert(game.campaignStage==mission&&game.phase==rpg::Phase::Won&&view.page==Page::CampaignResult);assert(saved(game)==before);while(view.campaignScene+1<story::finalePages(game,mission))tapped(100,290);tapped(100,290);}else tapped(100,265);}
 assert(view.page==Page::People&&saved(game)==saved(expected));auto won=saved(game);auto resultBlobs=nvs.blobs;tapped(100,290);assert(saved(game)==won&&nvs.blobs==resultBlobs); // A second result tap cannot pay again.
 selectSlot(0);assert(saved(game)==won&&rpg::persistentValid(game));
}
static void readEpilogue(){
 view.page=Page::Campaign;view.objectiveReturn=Page::Village;tapped(100,244);assert(view.page==Page::Epilogue&&view.campaignScene==0);
 const auto before=saved(game);const auto blobs=nvs.blobs;unsigned pages=story::finalePages(game,9);bool letter=false,origin=false;
 for(unsigned p=0;p<pages;++p){assert(view.campaignScene==p);auto section=story::finalePage(game,9,p);letter|=section.npc==story::Npc::Sabela;origin|=!strcmp(section.title,"SUA JORNADA");
  if(p){tapped(50,290);assert(view.campaignScene==p-1);tapped(180,290);assert(view.campaignScene==p);}
  tapped(180,290);assert(saved(game)==before&&nvs.blobs==blobs);}
 assert(view.page==Page::Campaign&&letter&&origin==game.originStory);
 tapped(100,244);assert(view.page==Page::Epilogue&&view.campaignScene==0);tapped(50,290);assert(view.page==Page::Campaign&&saved(game)==before&&blobs==nvs.blobs);
}
int main(){
 // A bought crystal follows the same objective route as the Guardian's key.
 for(unsigned city:{0u,1u,2u,3u}){seed(0,false,0);game.campaignFlags=0;game.dungeonClears=0;game.city=1;game.guardianDefeated=false;game.crystals=0;assert(!rpg::buyCrystal(game));game.city=city;assert(journal.save(game));view.page=Page::Campaign;auto before=saved(game);tapped(100,240);assert(saved(game)==before);if(city==1)assert(view.page==Page::DungeonEntry);else assert(view.page==Page::Map&&menu.destination==1);}
 unsigned paths=0;for(unsigned cls=0;cls<4;++cls)for(bool origin:{false,true})for(unsigned mask=0;mask<16;++mask){
  seed(cls,origin,mask);openFinale(8);winFromTask(mask==0,origin);assert(game.campaignFlags&128);
  view.page=Page::Dialogue;menu.personIndex=0;openFinale(9);winFromTask(mask==15,origin);assert(game.campaignEnding==(mask==15?2:1));
  if(mask!=15){view.page=Page::Dialogue;menu.personIndex=0;openFinale(9);auto before=saved(game);auto blobs=nvs.blobs;tapped(180,290);assert(view.page==Page::CampaignTask&&saved(game)==before&&nvs.blobs==blobs&&!strcmp(message,"Reuna as quatro ajudas para restaurar"));
   for(unsigned city=0;city<4;++city)if(!rpg::contribution(game,city)){game.city=city;game.p.hp=game.p.maxhp;game.p.mp=game.p.maxmp;assert(!rpg::startCampaign(game,city+4));if(!rpg::campaignDonation(city+4)){game.enemyHp=0;rpg::finish(game);assert(rpg::resolveCampaign(game));}assert(rpg::persistentValid(game));}
   game.city=3;assert(journal.save(game));view.page=Page::Dialogue;openFinale(9);winFromTask(false,false);assert(game.campaignEnding==2);}
  readEpilogue();++paths;
 }
 // The first crypt win knows the Book before dungeonResolve; clears, not campaignMemory,
 // selects first discovery vs repeat. Save failures/reboot keep the same discovery.
 for(unsigned clears:{0u,1u,255u})for(bool full:{false,true})for(bool exit:{false,true}){
  if(full&&!clears)continue;
  seed(0,false,full?15:0);game.campaignFlags=full?255:0;game.campaignEnding=full?2:0;game.dungeonClears=clears;game.city=1;game.crystals=1;
  assert(!rpg::enterDungeon(game,0));game.dungeonFlags=3;game.dungeonXY=0x27;game.dungeonLoot=8;game.dungeonEnemies=32;
  assert(rpg::dungeonEnemyAhead(game)==6&&rpg::begin(game,8));game.enemyHp=0;rpg::finish(game);
  assert(rpg::campaignMemory(game)&&rpg::persistentValid(game)&&journal.save(game));selectSlot(0);assert(view.page==Page::DungeonVictory&&game.dungeonClears==clears);
  auto before=saved(game);auto blobs=nvs.blobs;auto expected=game;assert(rpg::dungeonResolve(expected));if(exit)assert(rpg::leaveDungeon(expected));
  nvs.fail=true;tapped(80,exit?290:240);assert(view.page==Page::SaveError&&saved(game)==saved(expected)&&blobs==nvs.blobs);
  nvs.fail=false;selectSlot(0);assert(view.page==Page::DungeonVictory&&saved(game)==before&&game.dungeonClears==clears);
  tapped(80,exit?290:240);assert(saved(game)==saved(expected)&&game.dungeonClears==std::min(clears+1,255u));
  auto gold=game.p.gold,xp=game.p.xp;tapped(80,exit?290:240);assert(game.p.gold==gold&&game.p.xp==xp);selectSlot(0);assert(saved(game)==saved(expected));
 }
 puts("PASS:128 Save30 finale paths; real copied controller, Odran/Heart separation, all masks/classes/origins, temporary requirements, pagination/back/close/reopen, save failure/reboot/no duplicate rewards; first/repeat crypt discovery.");
}
