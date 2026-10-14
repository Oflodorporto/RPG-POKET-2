// Real controller snapshot, real TouchGate and production Save30 journal.
// RPG_TEST_SAVE29 selects ControllerStore29.h (the production 29/30 codec).
#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main originalBulkTitleTests
#include "title_controller.cpp"
#undef main
#include <cstdio>
using namespace rpg;
static unsigned cases=0,accepted=0;
static std::vector<uint8_t> bytes(const Game& g){std::vector<uint8_t> b(PERSISTENT_SAVE_SIZE);encodePersistent(g,1,0,b.data());return b;}
static void same(const Game& a,const Game& b){assert(bytes(a)==bytes(b));}
static void sample(bool valid,bool down,int x,int y,unsigned time){now=time;if(gate.update(valid,down,now)){++accepted;tapped(x,y);}}
static void press(int x,int y){
 const unsigned start=now+1;sample(true,false,x,y,start);sample(true,false,x,y,start+TouchGate::releaseMs);
 const unsigned before=accepted;sample(true,true,x,y,start+31);
 // A held contact including invalid reads and brief releases cannot repeat.
 for(unsigned t=32;t<90;++t)sample(t%7!=0,t%13!=0,x,y,start+t);
 assert(accepted==before+1);
}
static void rect(scenicUi::Rect r){press(r.x+r.w/2,r.y+r.h/2);}
static void minus(){press(60,194);}static void plus(){press(175,194);}static void maximum(){press(120,236);}
static void cancel(){press(60,292);}static void confirm(){press(175,292);}static void receipt(){press(120,292);}
static Game hero(unsigned city,unsigned cls){auto g=create(cls,75+cls);g.p.xp=dndXp[19];levelUp(g);g.tutorial=true;g.city=city;g.p.gold=12000;g.p.life=g.p.mana=2;g.rations=g.charts=g.charms=g.gazuas=g.crystals=0;g.sleepKit=false;g.owned=0;g.fatigue=38;assert(persistentValid(g));return g;}
static void setup(unsigned slot,unsigned city,unsigned cls,bool migration=false){
 nvs=Store{};for(unsigned s=0;s<3;++s){NvsBackend b(nvs);b.slot=s;NvsJournal j(b);Game g;assert(j.load(g)==Load::Empty);g=hero(s,s);g.p.gold+=s;assert(j.save(g));}
 backend.slot=slot;assert(journal.load(game)==Load::Ok);game=hero(city,cls);assert(journal.save(game));
 if(migration){auto& b=nvs.blobs[backend.key(journal.active)];encodePersistentVersion(game,journal.seq,slot,b.data(),29);assert(journal.load(game)==Load::Ok);}
 view=ViewState{};menu=MenuState{};menu.activeSlot=slot;menu.sessionStarted=true;combatFx=CombatFx{};pendingTouch=false;gate=TouchGate{};now=100;afterSave=Page::Home;
}
static Game durable(unsigned slot){NvsBackend b(nvs);b.slot=slot;NvsJournal j(b);Game g;auto loaded=j.load(g);assert(loaded==Load::Ok||loaded==Load::Recovered);return g;}
static void unchangedOthers(const std::map<std::string,std::vector<uint8_t>>& before){
 for(unsigned s=0;s<3;++s)if(s!=backend.slot){NvsBackend b(nvs);b.slot=s;for(const char* key:{b.key(false),b.key(true),b.metaKey(),b.guideKey()}){auto old=before.find(key);auto current=nvs.blobs.find(key);assert((old==before.end())==(current==nvs.blobs.end()));if(old!=before.end())assert(old->second==current->second);}}
}
enum class Route {Life,Mana,Good,Crystal,PicksCity,PicksBag,KitCity,KitCamp,Gear};
struct PurchasePath {unsigned item;Page page,back;};
static PurchasePath open(Route route,unsigned gear=0){
 switch(route){
 case Route::Life:case Route::Mana:view.page=Page::Shop;press(120,route==Route::Life?192:238);return {route==Route::Life?0u:1u,Page::Buy,Page::Shop};
 case Route::Good:view.page=Page::CityGoods;press(60,238);return {2u+localGood(game.city),Page::GoodsBuy,Page::CityGoods};
 case Route::Crystal:view.page=Page::CityGoods;press(175,238);return {8,Page::CrystalBuy,Page::CityGoods};
 case Route::PicksCity:view.page=Page::CityGoods;press(120,196);assert(view.page==Page::Lockpicks);confirm();return {6,Page::Buy,Page::Lockpicks};
 case Route::PicksBag:view.page=Page::TownBag;view.choice=8;rect(panelUi::bagUse);assert(view.page==Page::Lockpicks&&view.picksReturn==Page::TownBag);confirm();return {6,Page::Buy,Page::Lockpicks};
 case Route::KitCity:view.page=Page::CityGoods;confirm();return {7,Page::CampKit,Page::CityGoods};
 case Route::KitCamp:view.page=Page::CampSetup;press(120,257);return {7,Page::CampKit,Page::CampSetup};
 case Route::Gear:view.page=Page::GearShop;view.gearIndex=gear;press(120,200);return {19u+cityGearOffer(game,gear),Page::GearBuy,Page::GearShop};
 }
 assert(false);return {};
}
static void checkOpen(const PurchasePath& p){assert(view.page==p.page&&view.purchaseReturn==p.back&&view.purchase.open&&view.purchase.item==p.item);}
static void purchasePaths(){
 for(unsigned slot=0;slot<3;++slot)for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls)for(auto route:{Route::Life,Route::Mana,Route::Good,Route::Crystal,Route::PicksCity,Route::PicksBag,Route::KitCity,Route::KitCamp,Route::Gear}){
  if(route==Route::Crystal&&city!=1)continue;
  setup(slot,city,cls);auto before=game;auto blobs=nvs.blobs;auto p=open(route);checkOpen(p);same(game,before);assert(nvs.blobs==blobs&&view.purchase.quantity==1);
  const unsigned limit=purchaseMaximum(game,p.item);minus();assert(view.purchase.quantity==1);plus();assert(view.purchase.quantity==std::min(2u,limit));maximum();assert(view.purchase.quantity==limit);plus();assert(view.purchase.quantity==limit);minus();assert(view.purchase.quantity==std::max(1u,limit-1));same(game,before);assert(nvs.blobs==blobs);
  cancel();assert(view.page==p.back&&!view.purchase.open);same(game,before);assert(nvs.blobs==blobs);++cases;
  // Reopening resets the selection; a held confirm yields exactly one batch,
  // remains on its receipt and cannot fall through to the source's next action.
  p=open(route);checkOpen(p);assert(view.purchase.quantity==1);maximum();auto expected=game;auto quote=view.purchase;assert(!confirmPurchase(expected,quote));confirm();assert(view.page==Page::PurchaseReceipt&&!view.purchase.open);same(game,expected);same(durable(slot),expected);unchangedOthers(blobs);
  auto seq=journal.seq;const unsigned acceptedBefore=accepted;for(unsigned t=0;t<500;++t)sample(t%11!=0,true,175,292,now+1);assert(accepted==acceptedBefore&&view.page==Page::PurchaseReceipt&&journal.seq==seq);same(game,expected);
  receipt();assert(view.page==p.back);same(game,expected);assert(journal.seq==seq);if(route==Route::KitCamp)assert(menu.campKit);if(route==Route::PicksBag||route==Route::PicksCity){cancel();assert(view.page==(route==Route::PicksBag?Page::TownBag:Page::CityGoods));}++cases;
 }
 // Each actual city/class equipment offer reaches its unique-item selector.
 for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls){setup(0,city,cls);unsigned count=cityGearCount(game);
  for(unsigned i=0;i<count;++i){setup(0,city,cls);auto p=open(Route::Gear,i);checkOpen(p);assert(purchaseMaximum(game,p.item)==1);maximum();plus();assert(view.purchase.quantity==1);auto expected=game;auto q=view.purchase;assert(!confirmPurchase(expected,q));confirm();assert(view.page==Page::PurchaseReceipt);same(game,expected);same(durable(0),expected);receipt();assert(view.page==Page::GearShop);++cases;}
 }
}
static void purchaseFailures(){
 for(unsigned slot=0;slot<3;++slot)for(bool migration:{false,true})for(auto route:{Route::Life,Route::Mana,Route::Good,Route::Crystal,Route::PicksCity,Route::KitCity,Route::KitCamp,Route::Gear}){
  setup(slot,1,slot,migration);auto before=game;auto blobs=nvs.blobs;auto p=open(route);checkOpen(p);maximum();auto expected=game;auto q=view.purchase;assert(!confirmPurchase(expected,q));nvs.fail=true;confirm();assert(view.page==Page::SaveError&&afterSave==Page::PurchaseReceipt&&!view.purchase.open);same(game,expected);same(durable(slot),before);assert(nvs.blobs==blobs);
  auto failedBytes=nvs.blobs;maximum();plus();confirm();cancel();assert(view.page==Page::SaveError);same(game,expected);assert(nvs.blobs==failedBytes);
  nvs.fail=false;press(110,269);assert(view.page==Page::PurchaseReceipt);same(game,expected);same(durable(slot),expected);unchangedOthers(blobs);assert(get16(nvs.blobs[backend.key(journal.active)].data(),4)==30);receipt();assert(view.page==p.back);++cases;
  // A reboot before retry restores the entire old transaction, never just its
  // gold or inventory half. Quotes are discarded by the real title path.
  setup(slot,1,slot,migration);before=game;open(route);maximum();nvs.fail=true;confirm();assert(view.page==Page::SaveError);nvs.fail=false;bootTitle();assert(view.page==Page::Title&&!view.purchase.open&&!view.sale.open);selectSlot(slot);same(game,before);++cases;
 }
 // Revalidation catches stale inventory/city/price and drained gold atomically.
 for(unsigned change=0;change<5;++change){setup(0,0,0);open(Route::Life);maximum();
  if(change==0)++game.p.life;else if(change==1)game.city=1;else if(change==2)game.p.gold=0;else if(change==3)++view.purchase.unit;else view.purchase.quantity=0;
  auto before=game;auto blobs=nvs.blobs;confirm();assert(view.page==Page::Buy&&view.purchase.open);same(game,before);assert(nvs.blobs==blobs);cancel();assert(!view.purchase.open&&view.page==Page::Shop);++cases;
 }
 // Reachable empty/full selectors on goods, crystals, picks and kit stay inert.
 for(unsigned id:{2u,6u,7u,8u})for(bool full:{false,true}){
  setup(0,id==8?1:0,0);if(full){if(id==2)game.rations=10;if(id==6)game.gazuas=9;if(id==7)game.sleepKit=true;if(id==8)game.crystals=9;}else game.p.gold=0;
  assert(journal.save(game));auto before=game;auto blobs=nvs.blobs;openPurchase(id,id==7?Page::CampKit:id==8?Page::CrystalBuy:Page::GoodsBuy,Page::CityGoods);assert(view.purchase.open&&!view.purchase.quantity&&purchaseMaximum(game,id)==0);
  minus();plus();maximum();assert(!view.purchase.quantity);confirm();same(game,before);assert(nvs.blobs==blobs&&view.page!=Page::PurchaseReceipt);cancel();assert(view.page==Page::CityGoods&&!view.purchase.open);++cases;
 }
 // Existing one-item shop guards still reject full, unaffordable and ineligible gear.
 setup(0,0,0);game.p.life=99;auto before=game;open(Route::Life);assert(view.page==Page::Shop&&!view.purchase.open);same(game,before);
 setup(0,0,0);game.p.gold=0;before=game;open(Route::Mana);assert(view.page==Page::Shop&&!view.purchase.open);same(game,before);
 setup(0,3,0);game.p.level=1;before=game;open(Route::Gear,1);assert(view.page==Page::GearShop&&!view.purchase.open);same(game,before);
 // Switching slots or leaving to title invalidates both volatile quote types.
 setup(0,0,0);open(Route::Life);assert(!quoteSale(game,view.sale,0));selectSlot(1);assert(!view.purchase.open&&!view.sale.open);open(Route::Life);assert(!quoteSale(game,view.sale,0));openTitle();assert(!view.purchase.open&&!view.sale.open);++cases;
}
static void escortKitRecovery(){
 for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls)for(unsigned tier=1;tier<4;++tier)for(bool migration:{false,true})for(bool fail:{false,true}){
  setup(slot,tier,cls);game.eventStage=2;game.eventKind=2;game.eventTier=tier;game.eventLevel=tier==1?5:tier==2?10:18;game.eventDay=20733;game.eventOriginPage=9;game.p.hp=1;game.p.mp=0;
  assert(persistentValid(game)&&escortCampAllowed(game)&&journal.save(game));
  if(migration){auto& b=nvs.blobs[backend.key(journal.active)];encodePersistentVersion(game,journal.seq,slot,b.data(),29);assert(journal.load(game)==Load::Ok);}
  auto before=game;auto blobs=nvs.blobs;view.page=Page::EventMission;cancel();assert(view.page==Page::Recovery);press(120,168);assert(view.page==Page::CampSetup);press(120,257);assert(view.page==Page::CampKit&&view.purchase.item==7&&view.purchaseReturn==Page::CampSetup);maximum();plus();assert(view.purchase.quantity==1);cancel();assert(view.page==Page::CampSetup);same(game,before);assert(nvs.blobs==blobs);
  press(120,257);nvs.fail=fail;confirm();auto expected=before;expected.sleepKit=true;expected.p.gold-=80;same(game,expected);
  if(fail){assert(view.page==Page::SaveError);same(durable(slot),before);confirm();same(game,expected);nvs.fail=false;press(110,269);}
  assert(view.page==Page::PurchaseReceipt&&!view.purchase.open&&menu.campKit);same(durable(slot),expected);unchangedOthers(blobs);receipt();assert(view.page==Page::CampSetup);cancel();assert(view.page==Page::EventMission);selectSlot(slot);same(game,expected);assert(view.page==Page::EventMission);++cases;
 }
}
static void openListSale(unsigned id){view.page=Page::Market;cancel();assert(view.page==Page::TradeList);for(view.saleIndex=0;view.saleIndex<tradeCount(game)&&tradeOffer(game,view.saleIndex)!=id;++view.saleIndex){}assert(view.saleIndex<tradeCount(game));maximum();assert(view.page==Page::TradeConfirm&&view.sale.item==id&&view.saleReturn==Page::TradeList);}
static void salePaths(){
 for(unsigned slot=0;slot<3;++slot)for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls){
  setup(slot,city,cls);game.p.life=10;game.p.gold=999999-3*tradeUnit(game,0);assert(journal.save(game));auto before=game;auto blobs=nvs.blobs;openListSale(0);assert(view.sale.quantity==1&&saleMaximum(game,0)==3);minus();assert(view.sale.quantity==1);plus();assert(view.sale.quantity==2);maximum();plus();assert(view.sale.quantity==3);cancel();assert(view.page==Page::TradeList&&!view.sale.open);same(game,before);assert(nvs.blobs==blobs);
  openListSale(0);maximum();auto expected=game;auto q=view.sale;assert(!confirmSale(expected,q));confirm();assert(view.page==Page::TradeReceipt&&!view.sale.open);same(game,expected);same(durable(slot),expected);assert(game.p.gold==999999&&game.p.life==7);receipt();assert(view.page==Page::TradeList);unchangedOthers(blobs);++cases;
  // Scrap shortcut starts at the affordable maximum and returns to Scrap.
  setup(slot,city,cls);game.scrap=9;game.p.gold=999995;assert(journal.save(game));before=game;view.page=Page::Scrap;confirm();assert(view.page==Page::TradeConfirm&&view.sale.quantity==2&&view.saleReturn==Page::Scrap);cancel();assert(view.page==Page::Scrap);same(game,before);confirm();confirm();assert(view.page==Page::TradeReceipt&&game.scrap==7&&game.p.gold==999999);receipt();assert(view.page==Page::Scrap);++cases;
  // Gear selling shares the selector but preserves its bag-specific return.
  setup(slot,city,cls);const unsigned gear=cls*3+1;game.owned=1u<<(gear-1);assert(journal.save(game));view.page=Page::BagGear;view.choice=view.gearIndex=0;rect(touchUi::gearSell);assert(view.page==Page::GearSell&&view.sale.item==19+gear&&view.sale.quantity==1&&view.saleReturn==Page::BagGear);plus();maximum();assert(view.sale.quantity==1);cancel();assert(view.page==Page::BagGear&&game.owned);rect(touchUi::gearSell);confirm();assert(view.page==Page::TradeReceipt&&!game.owned);receipt();assert(view.page==Page::BagGear);same(durable(slot),game);++cases;
  setup(slot,city,cls);game.owned=1u<<(gear-1);game.equipped[0]=gear;view.page=Page::BagGear;view.choice=view.gearIndex=0;before=game;blobs=nvs.blobs;rect(touchUi::gearSell);assert(view.page==Page::BagGear&&!view.sale.open);same(game,before);assert(nvs.blobs==blobs);++cases;
 }
 for(unsigned slot=0;slot<3;++slot)for(bool migration:{false,true}){
  setup(slot,0,slot,migration);game.p.life=9;assert(journal.save(game));if(migration){auto& b=nvs.blobs[backend.key(journal.active)];encodePersistentVersion(game,journal.seq,slot,b.data(),29);assert(journal.load(game)==Load::Ok);}auto before=game;auto blobs=nvs.blobs;openListSale(0);maximum();auto expected=game;auto q=view.sale;assert(!confirmSale(expected,q));nvs.fail=true;confirm();assert(view.page==Page::SaveError&&!view.sale.open);same(game,expected);same(durable(slot),before);assert(nvs.blobs==blobs);confirm();same(game,expected);nvs.fail=false;press(110,269);assert(view.page==Page::TradeReceipt);same(durable(slot),expected);unchangedOthers(blobs);receipt();assert(view.page==Page::TradeList);++cases;
 }
 setup(0,0,0);game.crystals=9;view.page=Page::Market;cancel();for(unsigned i=0;i<tradeCount(game);++i)assert(tradeOffer(game,i)!=8);
 setup(0,0,0);openListSale(0);++game.p.life;auto before=game;auto blobs=nvs.blobs;confirm();assert(view.page==Page::TradeConfirm&&view.sale.open);same(game,before);assert(nvs.blobs==blobs);++cases;
}
int main(){purchasePaths();purchaseFailures();escortKitRecovery();salePaths();std::printf("PASS: %u bulk-commerce real-controller scenarios, %u TouchGate-accepted presses; every purchase route, +/-/Max/cancel/receipt, caps/protection/stale quotes, held samples, Save30/29 retry/reboot and 3-slot isolation.\n",cases,accepted);}
