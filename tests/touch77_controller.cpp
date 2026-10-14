#define main priorTitleRegression
#include "title_controller.cpp"
#undef main
static unsigned cases;
static void setup(unsigned slot,bool modern,unsigned cls,rpg::Phase phase=rpg::Phase::Home){
 nvs=Store{};backend.slot=slot;assert(journal.load(game)==rpg::Load::Empty);game=modern?rpg::create(cls,42):rpg::testHero(cls,42);game.tutorial=true;game.p.life=game.p.mana=9;game.p.hp=game.p.mp=1;game.scrap=7;game.gazuas=3;game.owned=(1u<<rpg::GEAR_COUNT)-1;
 if(phase!=rpg::Phase::Home){assert(rpg::begin(game,0));game.phase=phase;if(phase==rpg::Phase::Won)game.enemyHp=0;}assert(journal.save(game));view=ViewState{};view.page=phase==rpg::Phase::Home?Page::TownBag:Page::Bag;view.bagReturn=Page::Village;menu=MenuState{};menu.sessionStarted=true;combatFx=CombatFx{};now=100;
}
static void same(const rpg::Game& a){assert(!memcmp(&a,&game,sizeof(game)));}
static void tap(scenicUi::Rect r){tapped(r.x+r.w/2,r.y+r.h/2);}
static std::vector<std::pair<int,int>> corners(scenicUi::Rect r){return {{r.x,r.y},{r.x+r.w-1,r.y},{r.x,r.y+r.h-1},{r.x+r.w-1,r.y+r.h-1}};}
int main(){
 for(unsigned slot=0;slot<3;++slot)for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls){
  for(auto phase:{rpg::Phase::Home,rpg::Phase::Hero,rpg::Phase::Enemy,rpg::Phase::Won}){
   setup(slot,modern,cls,phase);auto original=game;auto blobs=nvs.blobs;
   for(unsigned i=0;i<9;++i)for(auto xy:corners(panelUi::bagSlot(i))){tapped(xy.first,xy.second);assert(view.choice==i);same(original);assert(nvs.blobs==blobs);++cases;}
   // Slot borders select only; gaps never activate Usar or adjacent navigation.
   for(unsigned i=0;i<9;++i){auto r=panelUi::bagSlot(i);view.choice=0;tapped(r.x-1,r.y+20);assert(view.choice==0);same(original);++cases;}
   for(unsigned i:{7u,8u}){setup(slot,modern,cls,phase);original=game;blobs=nvs.blobs;view.choice=i;tap(panelUi::bagUse);assert(view.page==(i==8?Page::Lockpicks:phase==rpg::Phase::Home?Page::Scrap:Page::Bag));same(original);assert(nvs.blobs==blobs);++cases;}
   for(unsigned i:{0u,1u})for(bool fail:{false,true}){
    setup(slot,modern,cls,phase);auto expected=game;view.choice=i;bool mana=i==1;bool changes=phase==rpg::Phase::Home||phase==rpg::Phase::Hero;
    if(phase==rpg::Phase::Hero)assert(!rpg::act(expected,mana?rpg::Action::Mana:rpg::Action::Life));
    else if(phase==rpg::Phase::Home){auto& value=mana?expected.p.mp:expected.p.hp;auto& amount=mana?expected.p.mana:expected.p.life;value+=rpg::recovery(value,mana?expected.p.maxmp:expected.p.maxhp,mana);--amount;}
    nvs.fail=fail;tap(panelUi::bagUse);same(expected);
    if(changes){assert(view.page==(fail?Page::SaveError:phase==rpg::Phase::Hero?Page::Battle:Page::TownBag));if(fail){auto blobs=nvs.blobs;tap(panelUi::bagUse);same(expected);assert(nvs.blobs==blobs);nvs.fail=false;tapped(110,269);same(expected);}rpg::Game loaded;assert(journal.load(loaded)==rpg::Load::Ok);assert(loaded.p.life==expected.p.life&&loaded.p.mana==expected.p.mana&&loaded.phase==expected.phase&&loaded.randomState==expected.randomState);}
    ++cases;
   }
   setup(slot,modern,cls,phase);combatFx.start(Effect::Slash,false,now,{CombatFeedback::Damage,2,false,false});original=game;view.choice=0;tap(panelUi::bagUse);same(original);tap(panelUi::bagGear);assert(view.page==(phase==rpg::Phase::Home?Page::TownBag:Page::Bag));++cases;
  }
  // Event restrictions affect explicit Ver/Usar, never the selection itself.
  for(unsigned i:{6u,7u,8u}){setup(slot,modern,cls);game.eventStage=2;view.choice=i;auto before=game;tap(panelUi::bagUse);assert(view.page==Page::TownBag);same(before);++cases;}
  setup(slot,modern,cls);auto before=game;auto blobs=nvs.blobs;tap(panelUi::bagGear);assert(view.page==Page::BagGear);
  for(unsigned p=0;p<3;++p){assert(view.gearIndex==p);for(unsigned i=0;i<6;++i){auto r=scenicUi::Rect{10+int(i%3)*76,74+int(i/3)*64,68,58};for(auto xy:corners(r)){tapped(xy.first,xy.second);assert(view.choice==i);assert(rpg::gearOwnedAt(game.owned,view.gearIndex*6+view.choice)==p*6+i+1);same(before);assert(nvs.blobs==blobs);++cases;}}tap(touchUi::gearNext);assert(view.choice==0);}assert(view.gearIndex==0);tap(touchUi::gearPrevious);assert(view.gearIndex==2);tap(touchUi::gearBack);assert(view.page==Page::TownBag);same(before);
  for(unsigned id=1;id<=rpg::GEAR_COUNT;++id)for(auto phase:{rpg::Phase::Home,rpg::Phase::Hero,rpg::Phase::Won}){setup(slot,modern,cls,phase);view.page=Page::BagGear;view.gearIndex=(id-1)/6;view.choice=(id-1)%6;auto expected=game;if(phase==rpg::Phase::Home)rpg::equipGear(expected,id);tap(touchUi::gearEquip);same(expected);++cases;}
  // Equipment save retry persists the result, not a second equip action.
  setup(slot,modern,cls);view.page=Page::BagGear;unsigned valid=1;for(;valid<=rpg::GEAR_COUNT;++valid){auto trial=game;if(!rpg::equipGear(trial,valid)&&memcmp(&trial,&game,sizeof(game)))break;}assert(valid<=rpg::GEAR_COUNT);view.gearIndex=(valid-1)/6;view.choice=(valid-1)%6;nvs.fail=true;tap(touchUi::gearEquip);assert(view.page==Page::SaveError);before=game;nvs.fail=false;tapped(110,269);same(before);assert(view.page==Page::BagGear);++cases;
  const Page destinations[]={Page::People,Page::Market,Page::TownBag,Page::Character,Page::Explore,Page::Guild,Page::CampSetup,Page::Map,Page::Menu};
  for(unsigned page=0;page<2;++page)for(unsigned i=0;i<5;++i)for(auto xy:corners(i==4?carvalhoUi::menu:carvalhoUi::actions[i])){setup(slot,modern,cls);view.page=Page::Village;view.cityPage=page;before=game;blobs=nvs.blobs;tapped(xy.first,xy.second);assert(view.page==destinations[i==4?8:carvalhoUi::actionIds[page][i]]);same(before);assert(nvs.blobs==blobs);++cases;}
  for(auto xy:corners(carvalhoUi::mission)){setup(slot,modern,cls);view.page=Page::Village;tapped(xy.first,xy.second);assert(view.page==Page::Campaign);++cases;}
  setup(slot,modern,cls);view.page=Page::Village;for(auto xy:{std::pair<int,int>{5,176},{118,196},{6,216},{6,264},{234,272}}){tapped(xy.first,xy.second);assert(view.page==Page::Village);++cases;}
 }
 // Every enlarged paging/back button uses the same inclusive/exclusive rectangle.
 for(auto r:{touchUi::gearPrevious,touchUi::gearNext,touchUi::gearBack})for(auto xy:corners(r)){setup(0,true,0);view.page=Page::BagGear;tapped(xy.first,xy.second);assert(view.page==(r.y==276?Page::TownBag:Page::BagGear));if(r.y!=276)assert(view.gearIndex==(r.x==10?2u:1u));++cases;}
 for(auto r:{touchUi::gearEquip,touchUi::gearSell})for(auto xy:corners(r)){setup(0,true,0,rpg::Phase::Hero);view.page=Page::BagGear;auto before=game;auto blobs=nvs.blobs;tapped(xy.first,xy.second);same(before);assert(view.page==Page::BagGear&&nvs.blobs==blobs);++cases;}
 // Real dungeon entry/return and potion paths, including terminal result and storage error.
 for(auto phase:{rpg::Phase::Home,rpg::Phase::Hero,rpg::Phase::Won})for(unsigned potion:{0u,1u})for(bool fail:{false,true}){
  setup(0,true,0);game.city=1;game.crystals=1;assert(!rpg::enterDungeon(game,0));if(phase!=rpg::Phase::Home){game.dungeonXY=0x72;assert(rpg::begin(game,rpg::dungeonSpawn(game,0).id));game.phase=phase;if(phase==rpg::Phase::Won)game.enemyHp=0;}assert(journal.save(game));view.page=Page::Dungeon;auto before=game;tapped(80,300);assert(view.page==Page::Bag);tap(panelUi::bagGear);assert(view.page==Page::BagGear);tap(touchUi::gearBack);assert(view.page==Page::Bag);same(before);tap(panelUi::bagBack);assert(view.page==Page::Dungeon);tapped(80,300);view.choice=potion;
  auto expected=game;bool mana=potion==1;if(phase==rpg::Phase::Hero)assert(!rpg::act(expected,mana?rpg::Action::Mana:rpg::Action::Life));else if(phase==rpg::Phase::Home){auto& val=mana?expected.p.mp:expected.p.hp;val+=rpg::recovery(val,mana?expected.p.maxmp:expected.p.maxhp,mana);--(mana?expected.p.mana:expected.p.life);}
  nvs.fail=fail;tap(panelUi::bagUse);same(expected);if(fail&&phase!=rpg::Phase::Won){assert(view.page==Page::SaveError);tap(panelUi::bagUse);same(expected);nvs.fail=false;tapped(110,269);same(expected);}assert(view.page==(phase==rpg::Phase::Hero?Page::Dungeon:Page::Bag));++cases;
 }
 for(auto phase:{rpg::Phase::Home,rpg::Phase::Hero})for(unsigned i:{0u,1u})for(bool empty:{false,true}){
  setup(0,true,0,phase);view.choice=i;if(empty)(i?game.p.mana:game.p.life)=0;else (i?game.p.mp:game.p.hp)=i?game.p.maxmp:game.p.maxhp;auto before=game;auto blobs=nvs.blobs;tap(panelUi::bagUse);same(before);assert(nvs.blobs==blobs);++cases;
 }
 setup(0,true,0);journal.blocked=true;view.page=Page::Blocked;auto blocked=game;auto blockedBlobs=nvs.blobs;tap(panelUi::bagUse);assert(view.page==Page::Blocked);tap(panelUi::bagGear);same(blocked);assert(nvs.blobs==blockedBlobs&&view.page==Page::Menu);++cases;
 // Held touch cannot consume multiple potions, including invalid I2C samples.
 setup(0,true,0);game.p.maxhp=300;game.p.hp=1;assert(journal.save(game));TouchGate held;held.update(true,false,0);held.update(true,false,30);unsigned consumed=0;
 for(unsigned t=31;t<400;++t){if(held.update(t%7!=0,true,t)){tap(panelUi::bagUse);++consumed;}}assert(consumed==1&&game.p.life==8);held.update(true,false,400);held.update(true,false,430);assert(held.update(true,true,431));tap(panelUi::bagUse);assert(game.p.life==7);++cases;
 printf("PASS: %u touch selection/action, all equipment/page, city boundary, phase/animation, three-slot legacy/new, save retry and held-input cases.\n",cases);
}
