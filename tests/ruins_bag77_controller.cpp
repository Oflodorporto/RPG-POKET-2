#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main previousTitleController
#include "title_controller.cpp"
#undef main
static unsigned cases=0;
static const auto bag=scenicUi::ruinsButton(scenicUi::ruinsBag);
static void tap(scenicUi::Rect r){tapped(r.x+r.w/2,r.y+r.h/2);}
static void same(const rpg::Game& expected){assert(!memcmp(&game,&expected,sizeof(game)));}
static void setup(unsigned slot,unsigned cls){
 nvs=Store{};
 for(unsigned s=0;s<3;++s){NvsBackend b(nvs);b.slot=s;NvsJournal j(b);rpg::Game hero;assert(j.load(hero)==rpg::Load::Empty);hero=rpg::create(s,77+s);hero.tutorial=true;assert(j.save(hero));}
 backend.slot=slot;assert(journal.load(game)==rpg::Load::Ok);game=rpg::create(cls,42);game.tutorial=true;game.city=1;game.p.life=game.p.mana=9;game.p.hp=1;game.p.mp=0;game.scrap=7;game.gazuas=3;game.owned=(1u<<rpg::GEAR_COUNT)-1;assert(journal.save(game));
 view=ViewState{};view.page=Page::Ruins;view.cityPage=1;view.choice=8;view.bagReturn=Page::Inventory;menu=MenuState{};menu.sessionStarted=true;menu.activeSlot=slot;combatFx=CombatFx{};now=100;
}
static void otherSlotsUnchanged(const std::map<std::string,std::vector<uint8_t>>& bytes){
 for(unsigned s=0;s<3;++s)if(s!=backend.slot){NvsBackend b(nvs);b.slot=s;for(const char* key:{b.key(false),b.key(true),b.metaKey(),b.guideKey()}){auto old=bytes.find(key);auto current=nvs.blobs.find(key);assert((old==bytes.end())==(current==nvs.blobs.end()));if(old!=bytes.end())assert(old->second==current->second);}}
}
static void persisted(){auto expected=game;rpg::Game loaded;assert(journal.load(loaded)==rpg::Load::Ok);assert(!memcmp(&loaded,&expected,sizeof(game)));}
static void retry(bool fail,const rpg::Game& expected,Page target){
 same(expected);if(fail){assert(view.page==Page::SaveError);auto bytes=nvs.blobs;tap(panelUi::bagUse);same(expected);assert(nvs.blobs==bytes);nvs.fail=false;tapped(110,269);same(expected);}assert(view.page==target);persisted();
}
int main(){
 // Every pixel maps to at most one current-page action, objective or pager.
 assert(scenicUi::ruinsButtonCount==8&&bag.w==112&&bag.h==44);
 for(unsigned page=0;page<2;++page)for(int y=0;y<320;++y)for(int x=0;x<240;++x){unsigned matches=scenicUi::ruinsMission.contains(x,y)+scenicUi::ruinsPager.contains(x,y);int expected=-1;for(unsigned i=0;i<4;++i)if(scenicUi::ruinsActions[i].contains(x,y)){++matches;expected=scenicUi::ruinsActionIds[page][i];}assert(matches<=1&&scenicUi::ruinsChoice(page,x,y)==expected);}
 for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls){
  for(int x:{bag.x,bag.x+bag.w/2,bag.x+bag.w-1})for(int y:{bag.y,bag.y+bag.h/2,bag.y+bag.h-1}){
   setup(slot,cls);auto original=game;auto bytes=nvs.blobs;nvs.fail=true;tapped(x,y);assert(view.page==Page::TownBag&&view.bagReturn==Page::Ruins&&view.choice==0);same(original);assert(nvs.blobs==bytes);tap(panelUi::bagBack);assert(view.page==Page::Ruins);same(original);assert(nvs.blobs==bytes);++cases;
  }
  for(auto xy:{std::pair<int,int>{bag.x-1,bag.y+20},{bag.x+bag.w,bag.y+20},{bag.x+20,bag.y-1},{bag.x+20,bag.y+bag.h}}){setup(slot,cls);auto original=game;auto bytes=nvs.blobs;tapped(xy.first,xy.second);assert(view.page==Page::Ruins);same(original);assert(nvs.blobs==bytes);++cases;}
  for(bool fail:{false,true})for(unsigned potion:{0u,1u}){
   setup(slot,cls);auto bytes=nvs.blobs;tap(bag);view.choice=potion;auto expected=game;auto& value=potion?expected.p.mp:expected.p.hp;unsigned maximum=potion?expected.p.maxmp:expected.p.maxhp;unsigned heal=std::min(maximum-value,std::max(1u,maximum*(potion?50:30)/100));value+=heal;--(potion?expected.p.mana:expected.p.life);nvs.fail=fail;tap(panelUi::bagUse);retry(fail,expected,Page::TownBag);assert(view.bagReturn==Page::Ruins);tap(panelUi::bagBack);assert(view.page==Page::Ruins);same(expected);otherSlotsUnchanged(bytes);++cases;
  }
  // Full/empty potion failures, opening other items, and reopening never reroll or spend.
  for(unsigned potion:{0u,1u})for(bool empty:{false,true}){setup(slot,cls);tap(bag);view.choice=potion;if(empty)(potion?game.p.mana:game.p.life)=0;else (potion?game.p.mp:game.p.hp)=potion?game.p.maxmp:game.p.maxhp;auto original=game;auto bytes=nvs.blobs;tap(panelUi::bagUse);same(original);assert(nvs.blobs==bytes&&view.page==Page::TownBag);tap(panelUi::bagBack);assert(view.page==Page::Ruins);++cases;}
  for(unsigned item:{2u,3u,4u,5u,7u,8u}){setup(slot,cls);tap(bag);view.choice=item;auto original=game;auto bytes=nvs.blobs;tap(panelUi::bagUse);if(item>=7){assert(view.page==(item==7?Page::Scrap:Page::Lockpicks));tapped(60,290);}assert(view.page==Page::TownBag);tap(panelUi::bagBack);assert(view.page==Page::Ruins);same(original);assert(nvs.blobs==bytes);tap(bag);assert(view.choice==0);++cases;}
  // Equipment save retry must not toggle the item twice; subsequent Back remains Ruins.
  for(bool fail:{false,true})for(unsigned id=1;id<=rpg::GEAR_COUNT;++id){setup(slot,cls);tap(bag);tap(panelUi::bagGear);assert(view.page==Page::BagGear);view.gearIndex=(id-1)/6;view.choice=(id-1)%6;auto bytes=nvs.blobs;auto expected=game;bool valid=!rpg::equipGear(expected,id);nvs.fail=fail;tap(touchUi::gearEquip);if(valid)retry(fail,expected,Page::BagGear);else{same(expected);assert(view.page==Page::BagGear&&nvs.blobs==bytes);nvs.fail=false;}tap(touchUi::gearBack);assert(view.page==Page::TownBag&&view.choice==0);tap(panelUi::bagBack);assert(view.page==Page::Ruins);same(expected);otherSlotsUnchanged(bytes);++cases;}
  setup(slot,cls);auto original=game;auto bytes=nvs.blobs;combatFx.start(Effect::Slash,false,now,{});tap(bag);assert(view.page==Page::Ruins);same(original);assert(nvs.blobs==bytes);combatFx=CombatFx{};menu.journey.start(0,1,now);tap(bag);assert(view.page==Page::Ruins);same(original);++cases;
  // Pending outcomes still block potion use and now explain the visible resolution control.
  for(auto phase:{rpg::Phase::Enemy,rpg::Phase::Won,rpg::Phase::Lost,rpg::Phase::Fled})for(unsigned potion:{0u,1u}){setup(slot,cls);assert(rpg::begin(game,0));game.phase=phase;if(phase==rpg::Phase::Won)game.enemyHp=0;if(phase==rpg::Phase::Lost)game.p.hp=0;view.page=Page::Bag;view.choice=potion;auto original=game;auto bytes=nvs.blobs;tap(panelUi::bagUse);same(original);assert(nvs.blobs==bytes&&view.page==Page::Bag);assert(!strcmp(view.message,phase==rpg::Phase::Enemy?"Aguarde o turno do inimigo":"Volte e toque em Continuar"));++cases;}
  // Held opening input is consumed once; it cannot turn into a potion press.
  setup(slot,cls);original=game;bytes=nvs.blobs;TouchGate held;held.update(true,false,0);held.update(true,false,30);unsigned accepted=0;for(unsigned t=31;t<400;++t)if(held.update(t%7!=0,true,t)){tap(bag);++accepted;}assert(accepted==1&&view.page==Page::TownBag);same(original);assert(nvs.blobs==bytes);tap(panelUi::bagBack);assert(view.page==Page::Ruins);++cases;
 }
 printf("PASS: %u Ruins bag Save29 controller cases, 76800-pixel disjoint geometry, 3 slots/4 classes, 30%% HP/50%% MP, item/equipment return, failed-save retry, phase/animation and held input.\n",cases);
}
