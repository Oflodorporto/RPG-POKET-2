// Package12 integration: verbatim production controller, production Save30
// journal and real CoopDevice bridge, linked to a separate CoopRuntime.cpp.
#define RPG_ENABLE_COOP_TRAINING 1
#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main originalPackage12TitleTests
#include "title_controller.cpp"
#undef main
#include "../firmware/RPG_POKET_2/Coop/CoopRadioDevice.h"
#include "../firmware/RPG_POKET_2/Coop/CoopInput.h"
#include "../firmware/RPG_POKET_2/CoopDevice.h"
#include <array>
#include <climits>
#include <cstdio>
using namespace rpg;
static unsigned scenarios=0,presses=0;
using SaveBytes=std::array<uint8_t,PERSISTENT_SAVE_SIZE>;
static SaveBytes packed(const Game& g){SaveBytes out{};encodePersistent(g,47,0,out.data());assert(get16(out.data(),4)==30);return out;}
static void same(const Game& a,const Game& b){assert(packed(a)==packed(b));}
static void regionalSelections(){
 // Independent table oracle avoids reproducing the GCC/UBSan pointer-selection
 // bug in the reference. n%2 and n%4 make four residues exhaustive for unsigned
 // indices; representatives additionally cover type boundaries and UINT_MAX.
 const std::array<unsigned,24> indices={0u,1u,2u,3u,4u,5u,6u,7u,31u,32u,33u,127u,128u,129u,65535u,65536u,UINT_MAX-7u,UINT_MAX-6u,UINT_MAX-5u,UINT_MAX-4u,UINT_MAX-3u,UINT_MAX-2u,UINT_MAX-1u,UINT_MAX};
 constexpr uint8_t tables[2][2][4]={{{6,25,6,26},{26,6,25,26}},{{7,27,7,28},{28,7,27,28}}};
 Game g=create(2,0x6a09e667u);g.p.life=7;g.p.mana=6;g.p.xp=12345;g.p.gold=67890;g.rations=10;
 unsigned long long checked=0;
 for(unsigned region=0;region<=UINT8_MAX;++region)for(unsigned level=0;level<=UINT8_MAX;++level)for(bool progression:{false,true})for(bool night:{false,true})for(unsigned n:indices){
  g.city=region;g.p.level=level;g.dndProgression=progression;g.p.cls=level%4;g.race=(level/4)%4;
  std::array<unsigned char,sizeof(Game)> before{};std::memcpy(before.data(),&g,sizeof(g));const auto rng=g.randomState;
  const uint8_t expected=region==0?(progression&&level<3?(n%2?0:1):(n%2?4:1)):region==1?(n%2?5:2):tables[region==2?0:1][night?1:0][n%4];
  assert(regionalEnemyAt(g,region,n,night)==expected&&regionalEnemy(g,n,night)==expected);
  assert(regionalEnemyAt(g,region,n%4,night)==expected);
  assert(std::memcmp(before.data(),&g,sizeof(g))==0&&g.randomState==rng);++checked;
 }
 assert(checked==6291456);
 std::printf("PASS: %llu regional selections; all region/level bytes, progression/day/night, class/race representatives, modulo-4 and UINT_MAX; independent tables, whole Game/RNG unchanged.\n",checked);
}
static void portrait(bool valid,bool down,int x,int y){if(gate.update(valid,down,now)){++presses;tapped(x,y);}}
static void press(int x,int y){
 ++now;portrait(true,false,x,y);now+=TouchGate::releaseMs;portrait(true,false,x,y);++now;
 unsigned before=presses;portrait(true,true,x,y);assert(presses==before+1);
 // Neither a held contact nor an invalid reading can repeat a commerce action.
 for(unsigned i=0;i<40;++i){++now;portrait(i%7!=0,true,x,y);}assert(presses==before+1);
}
static void minus(){press(60,194);}static void plus(){press(175,194);}static void maximum(){press(120,236);}
static void cancel(){press(60,292);}static void confirm(){press(175,292);}static void receipt(){press(120,292);}
static Game hero(unsigned city,unsigned cls){
 auto g=create(cls,711+city*4+cls);while(g.p.level<5){g.p.xp=xpNeeded(g);levelUp(g);}
 g.tutorial=true;g.city=city;g.p.gold=12000;g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.p.life=7;g.p.mana=6;g.rations=8;
 assert(persistentValid(g));return g;
}
static void setup(unsigned slot,unsigned city,unsigned cls){
 assert(!coopIsOpen());coopruntime::close();fake::reset();assert(coopradio::owner==coopradio::Owner::None);
 nvs=Store{};
 for(unsigned s=0;s<3;++s){NvsBackend b(nvs);b.slot=s;NvsJournal j(b);Game g;assert(j.load(g)==Load::Empty);g=hero(s,s);g.p.gold+=s;assert(j.save(g));assert(nvs.writeGuide(s,0x100u+s));}
 backend.slot=slot;assert(journal.load(game)==Load::Ok);game=hero(city,cls);assert(journal.save(game));
 view=ViewState{};menu=MenuState{};menu.activeSlot=slot;menu.sessionStarted=true;combatFx=CombatFx{};
 gate=TouchGate{};pendingTouch=false;now=100;afterSave=Page::Home;buffered=touchReady=lcdReady=true;arena.opened=false;updateInfo=updater::Info{};
}
static void durableEquals(const Game& expected){NvsBackend b(nvs);b.slot=backend.slot;NvsJournal j(b);Game restored;assert(j.load(restored)==Load::Ok);same(restored,expected);}
static void otherSlotsUnchanged(const std::map<std::string,std::vector<uint8_t>>& before){
 for(unsigned s=0;s<3;++s)if(s!=backend.slot){NvsBackend b(nvs);b.slot=s;for(const char* key:{b.key(false),b.key(true),b.metaKey(),b.guideKey()}){auto a=before.find(key);auto z=nvs.blobs.find(key);assert((a==before.end())==(z==nvs.blobs.end()));if(a!=before.end())assert(a->second==z->second);}}
}
static void commerce(){
 for(unsigned slot=0;slot<3;++slot)for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls){
  for(unsigned item=0;item<2;++item){
   setup(slot,city,cls);view.page=Page::Shop;auto before=game;auto blobs=nvs.blobs;
   press(120,item?238:192);assert(view.page==Page::Buy&&view.purchase.item==item&&view.purchase.quantity==1);
   minus();assert(view.purchase.quantity==1);plus();assert(view.purchase.quantity==2);maximum();assert(view.purchase.quantity==purchaseMaximum(game,item));plus();assert(view.purchase.quantity==purchaseMaximum(game,item));
   cancel();assert(view.page==Page::Shop&&!view.purchase.open);same(game,before);assert(nvs.blobs==blobs);
   press(120,item?238:192);assert(view.purchase.quantity==1);plus();auto expected=game;auto quote=view.purchase;assert(!confirmPurchase(expected,quote));confirm();
   assert(view.page==Page::PurchaseReceipt&&!view.purchase.open);same(game,expected);durableEquals(expected);otherSlotsUnchanged(blobs);
   auto seq=journal.seq;for(unsigned i=0;i<100;++i){++now;portrait(true,true,175,292);}assert(journal.seq==seq&&view.page==Page::PurchaseReceipt);receipt();assert(view.page==Page::Shop);++scenarios;
  }
  // Sell through the visible market/list/quantity/receipt controller path.
  setup(slot,city,cls);view.page=Page::Market;auto before=game;auto blobs=nvs.blobs;cancel();assert(view.page==Page::TradeList);
  for(view.saleIndex=0;view.saleIndex<tradeCount(game)&&tradeOffer(game,view.saleIndex)!=0;++view.saleIndex){}assert(view.saleIndex<tradeCount(game));
  maximum();assert(view.page==Page::TradeConfirm&&view.sale.quantity==1);minus();plus();assert(view.sale.quantity==2);maximum();assert(view.sale.quantity==7);cancel();assert(view.page==Page::TradeList);same(game,before);assert(nvs.blobs==blobs);
  maximum();plus();plus();assert(view.sale.quantity==3);auto expected=game;auto quote=view.sale;assert(!confirmSale(expected,quote));confirm();assert(view.page==Page::TradeReceipt&&!view.sale.open);same(game,expected);durableEquals(expected);otherSlotsUnchanged(blobs);receipt();assert(view.page==Page::TradeList);++scenarios;
 }
 for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls){
  // The merged package10 ration cap is still 10 while coop is compiled ON.
  setup(slot,0,cls);view.page=Page::CityGoods;auto blobs=nvs.blobs;press(60,238);assert(view.page==Page::GoodsBuy&&view.purchase.item==2);
  plus();maximum();plus();assert(view.purchase.quantity==2&&purchaseMaximum(game,2)==2);auto expected=game;auto quote=view.purchase;assert(!confirmPurchase(expected,quote));confirm();assert(view.page==Page::PurchaseReceipt&&game.rations==10);same(game,expected);durableEquals(expected);otherSlotsUnchanged(blobs);receipt();
  blobs=nvs.blobs;auto full=game;press(60,238);assert(view.purchase.open&&!view.purchase.quantity&&purchaseMaximum(game,2)==0);minus();plus();maximum();confirm();same(game,full);assert(nvs.blobs==blobs&&view.page==Page::GoodsBuy);cancel();assert(view.page==Page::CityGoods);++scenarios;
 }
}
static void coopSample(bool valid,bool down,int x,int y){auto p=coopplatform::toPortrait(x,y);captureCoopTouch(valid,down,p.x,p.y,now);tickCoopTraining(now);}
static void coopRelease(){coopSample(true,false,0,0);now+=31;coopSample(true,false,0,0);}
static void coopClick(int x,int y){coopRelease();++now;coopSample(true,true,x,y);now+=8;coopRelease();}
static void enterGuild(){view.page=Page::Tavern;press(120,240);assert(view.page==Page::Guild);}
static void menuIntegration(){
 for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls)for(bool member:{false,true}){
  setup(slot,slot,cls);game.guildMember=member;assert(journal.save(game));enterGuild();
  const auto before=game;const auto blobs=nvs.blobs;const auto seq=journal.seq;const auto active=journal.active;
  auto unchanged=[&](){same(game,before);assert(game.randomState==before.randomState&&game.p.life==before.p.life&&game.p.mana==before.p.mana&&game.p.xp==before.p.xp&&game.p.gold==before.p.gold);assert(nvs.blobs==blobs&&journal.seq==seq&&journal.active==active&&backend.slot==slot);assert(!fake::nvsWrites&&!fake::erases);};
  uint64_t previousToken=0;
  for(unsigned entry=0;entry<2;++entry){
   pendingTouch=true;press(175,238);assert(view.page==Page::CoopTraining&&coopIsOpen()&&!pendingTouch&&!gate.armed);
   assert(coopradio::owner==coopradio::Owner::Training&&!coopradio::acquire(coopradio::Owner::Pvp));unchanged();
   // The entry contact is still down. It must not become a landscape Exit.
   coopSample(true,true,280,10);coopRelease();assert(coopIsOpen()&&!coopClosing);
   coopClick(50,195);assert(coopSession.state()==coopplatform::SessionState::Hosting);assert(coopSession.sessionToken()&&coopSession.sessionToken()!=previousToken);previousToken=coopSession.sessionToken();unchanged();
   coopClick(280,10);assert(coopClosing&&coopIsOpen());now+=349;tickCoopTraining(now);assert(coopIsOpen());++now;tickCoopTraining(now);
   assert(!coopIsOpen()&&view.page==Page::Guild&&!pendingTouch&&!gate.armed&&coopradio::owner==coopradio::Owner::None);unchanged();
   // Portrait navigation requires a fresh release after leaving landscape.
   portrait(true,true,120,292);assert(view.page==Page::Guild);press(120,292);assert(view.page==Page::Village);unchanged();enterGuild();
  }
  durableEquals(before);otherSlotsUnchanged(blobs);++scenarios;
 }
 // A PvP lease or an existing arena prevents radio mutation before coop entry.
 setup(0,0,0);enterGuild();auto before=game;auto blobs=nvs.blobs;
 assert(coopradio::acquire(coopradio::Owner::Pvp));auto mutations=fake::mutations;press(175,238);
 assert(view.page==Page::Guild&&!coopIsOpen()&&coopradio::owner==coopradio::Owner::Pvp&&fake::mutations==mutations);same(game,before);assert(nvs.blobs==blobs);coopradio::release(coopradio::Owner::Pvp);
 arena.opened=true;press(175,238);assert(!coopIsOpen()&&fake::mutations==mutations);arena.opened=false;
 press(175,238);assert(coopIsOpen());coopClick(280,10);now+=350;tickCoopTraining(now);assert(!coopIsOpen()&&coopradio::owner==coopradio::Owner::None);same(game,before);assert(nvs.blobs==blobs&&!fake::nvsWrites&&!fake::erases);++scenarios;
}
int main(){
 static_assert(RPG_ENABLE_COOP_TRAINING==1,"This integration gate must compile the merged controller ON");
 regionalSelections();commerce();menuIntegration();
 std::printf("PASS: %u package12 ON real-controller scenarios, %u gated presses; SHOP buy/sell quantities, ration cap10, Save30/3-slot isolation, actual menu/runtime/portrait exit/reentry, PvP radio lease, campaign RNG/potions/XP/gold unchanged.\n",scenarios,presses);
}
