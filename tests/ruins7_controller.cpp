#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main historicalControllerMain
#include "title_controller.cpp"
#undef main
static unsigned cases=0;
static void tap(scenicUi::Rect r){tapped(r.x+r.w/2,r.y+r.h/2);}
static void act(unsigned id){view.cityPage=scenicUi::ruinsActionPage(id);tap(scenicUi::ruinsButton(id));}
static void setup(unsigned slot=0,unsigned cls=0){nvs=Store{};backend.slot=slot;assert(journal.load(game)==rpg::Load::Empty);game=rpg::create(cls,42);game.city=1;game.tutorial=true;game.p.xp=rpg::dndXp[7];rpg::levelUp(game);game.crystals=2;game.rations=3;game.p.gold=1000;assert(journal.save(game));menu=MenuState{};menu.sessionStarted=true;menu.activeSlot=slot;view=ViewState{};view.page=Page::Ruins;combatFx=CombatFx{};now=100;}
static void unchanged(const rpg::Game& original,const std::map<std::string,std::vector<uint8_t>>& bytes){assert(!memcmp(&game,&original,sizeof(game))&&nvs.blobs==bytes);}
int main(){
 // Stable eight action IDs appear once across pages, with no overlap or invisible actions.
 unsigned seen=0;for(unsigned p=0;p<2;++p)for(unsigned i=0;i<4;++i){unsigned id=scenicUi::ruinsActionIds[p][i];assert(id<8&&!(seen&(1<<id)));seen|=1<<id;assert(scenicUi::ruinsActionPage(id)==p);auto r=scenicUi::ruinsButton(id);assert(r.w==112&&r.h==44);}
 assert(seen==255);for(unsigned page=0;page<2;++page)for(int y=0;y<320;++y)for(int x=0;x<240;++x){unsigned hits=scenicUi::ruinsMission.contains(x,y)+scenicUi::ruinsPager.contains(x,y);int expected=-1;for(unsigned i=0;i<4;++i)if(scenicUi::ruinsActions[i].contains(x,y)){++hits;expected=scenicUi::ruinsActionIds[page][i];}assert(hits<=1&&scenicUi::ruinsChoice(page,x,y)==expected);}
 for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls){
  // Every corner of all read-only actions works, preserving NVS even if saving is unavailable.
  const Page targets[]={Page::People,Page::DungeonEntry,Page::Ruins,Page::Ruins,Page::CampSetup,Page::Map,Page::Market,Page::TownBag};
  for(unsigned id:{0u,1u,4u,5u,6u,7u}){auto r=scenicUi::ruinsButton(id);for(int x:{r.x,r.x+r.w-1})for(int y:{r.y,r.y+r.h-1}){setup(slot,cls);view.cityPage=scenicUi::ruinsActionPage(id);auto original=game;auto bytes=nvs.blobs;nvs.fail=true;tapped(x,y);assert(view.page==targets[id]);unchanged(original,bytes);++cases;}
   setup(slot,cls);auto original=game;auto bytes=nvs.blobs;act(id);if(id==0){assert(menu.storyReturn==int(Page::Ruins));tapped(80,100);assert(view.page==Page::Dialogue);tapped(80,290);assert(view.page==Page::People);tapped(80,290);}else if(id==1)tapped(60,290);else if(id==4)tapped(60,290);else if(id==5)tap(scenicUi::mapButtons[0]);else if(id==6)tapped(175,290);else tap(panelUi::bagBack);
   assert(view.page==Page::Ruins&&view.cityPage==(id==5?0:scenicUi::ruinsActionPage(id)));unchanged(original,bytes);++cases;
  }
  for(unsigned page=0;page<2;++page){setup(slot,cls);view.cityPage=page;auto original=game;auto bytes=nvs.blobs;tap(scenicUi::ruinsPager);assert(view.cityPage==1-page&&view.page==Page::Ruins);tap(scenicUi::ruinsPager);assert(view.cityPage==page);unchanged(original,bytes);tap(scenicUi::ruinsMission);assert(view.page==Page::Campaign&&view.objectiveReturn==Page::Ruins);tapped(175,290);assert(view.page==Page::Ruins&&view.cityPage==page);unchanged(original,bytes);++cases;}
  // Guardian gate stays at three wins. Success remains exact production-rule behavior.
  for(unsigned wins=0;wins<4;++wins)for(bool defeated:{false,true}){if(defeated&&wins<3)continue;setup(slot,cls);game.ruinsWins=wins;game.guardianDefeated=defeated;assert(journal.save(game));auto expected=game;auto bytes=nvs.blobs;if(wins>=3)assert(rpg::explore(expected,true));act(2);assert(!memcmp(&game,&expected,sizeof(game)));if(wins<3){assert(view.page==Page::Ruins&&!strcmp(view.message,"Venca 3 encontros primeiro"));unchanged(expected,bytes);}else assert(view.page==currentPage()&&game.enemyId==3);++cases;}
  // Exploring delegates to the old daytime/nighttime rule, with no extra RNG/costs.
  for(bool night:{false,true}){setup(slot,cls);menu.clockValid=menu.dayCycle=true;menu.worldPeriod=night?worldClock::Period::Night:worldClock::Period::Day;auto expected=game;bool begun=rpg::startDiscovery(expected,night);act(3);assert(!memcmp(&game,&expected,sizeof(game)));assert(!begun||view.page==currentPage());++cases;}
  // Dungeon entry still pays only on confirmation, exactly once through save retry.
  for(bool fail:{false,true}){setup(slot,cls);auto expected=game;act(1);assert(!memcmp(&game,&expected,sizeof(game)));assert(!rpg::enterDungeon(expected));nvs.fail=fail;tapped(175,290);assert(!memcmp(&game,&expected,sizeof(game)));if(fail){assert(view.page==Page::SaveError);nvs.fail=false;tapped(110,269);assert(!memcmp(&game,&expected,sizeof(game)));}assert(view.page==Page::Dungeon&&game.crystals==1);++cases;}
  setup(slot,cls);game.crystals=0;auto original=game;auto bytes=nvs.blobs;act(1);tapped(175,290);assert(view.page==Page::DungeonEntry&&!strcmp(view.message,"Precisa de um cristal"));unchanged(original,bytes);++cases;
  // Market's existing route still exposes the 300-gold crystal, and can return to page 2.
  setup(slot,cls);act(6);tapped(175,245);assert(view.page==Page::CityGoods);tapped(175,235);assert(view.page==Page::CrystalBuy);auto before=game;tapped(175,290);assert(game.p.gold==before.p.gold-300&&game.crystals==before.crystals+1&&view.page==Page::PurchaseReceipt);tapped(120,290);assert(view.page==Page::CityGoods);tapped(60,290);assert(view.page==Page::Market);tapped(175,290);assert(view.page==Page::Ruins&&view.cityPage==1);++cases;
  // In-flight combat/road animation and held touches cannot page through controls.
  setup(slot,cls);original=game;bytes=nvs.blobs;combatFx.start(Effect::Slash,false,now,{});tap(scenicUi::ruinsPager);assert(view.cityPage==0);for(unsigned id=0;id<8;++id){act(id);assert(view.page==Page::Ruins);unchanged(original,bytes);}combatFx=CombatFx{};view.cityPage=0;menu.journey.start(0,1,now);tap(scenicUi::ruinsPager);assert(view.cityPage==0);unchanged(original,bytes);++cases;
 }
 setup();auto original=game;auto bytes=nvs.blobs;TouchGate hold;hold.update(true,false,0);hold.update(true,false,30);unsigned flips=0;for(unsigned t=31;t<1000;++t)if(hold.update(t%7!=0,true,t)){tap(scenicUi::ruinsPager);++flips;}assert(flips==1&&view.cityPage==1);unchanged(original,bytes);view.cityPage=255;assert(scenicUi::ruinsChoice(255,10,180)==0);tap(scenicUi::ruinsPager);assert(view.cityPage==1);selectSlot(0);assert(view.cityPage==0);++cases;
 printf("PASS: %u Ruins controller scenarios, 153600 exhaustive page-hit pixels, eight routes+objective, live gate/costs, Save30 retries, 3 slots/4 classes, returns and held/blocked inputs.\n",cases);
}
