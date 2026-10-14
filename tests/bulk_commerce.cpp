// Bulk commerce model and production journal tests; no controller or fake codec.
#include "../firmware/RPG_POKET_2/Trading.h"
#include "../firmware/RPG_POKET_2/Persistence.h"
#include <cassert>
#include <climits>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
using namespace rpg;

static unsigned cases=0;
static std::vector<uint8_t> packed(const Game& g){
 std::vector<uint8_t> b(PERSISTENT_SAVE_SIZE);encodePersistent(g,1,0,b.data());return b;
}
static void same(const Game& a,const Game& b){assert(packed(a)==packed(b));}
static Game hero(unsigned city=0,unsigned cls=0,unsigned level=20){
 auto g=create(cls,47);g.tutorial=true;g.p.xp=dndXp[level-1];levelUp(g);
 g.city=city;g.p.gold=100000;g.p.life=g.p.mana=0;g.rations=g.charts=g.charms=g.scrap=g.gazuas=g.crystals=0;
 g.owned=0;g.sleepKit=false;g.fatigue=37;assert(persistentValid(g));return g;
}
static void stock(Game& g,unsigned id,unsigned n){
 switch(id){case 0:g.p.life=n;break;case 1:g.p.mana=n;break;case 2:g.rations=n;break;case 3:g.charts=n;break;
 case 4:g.charms=n;break;case 5:g.scrap=n;break;case 6:g.gazuas=n;break;case 7:g.sleepKit=n;break;case 8:g.crystals=n;break;
 default:assert(tradeGear(id));if(n)g.owned|=1u<<(id-20);else g.owned&=~(1u<<(id-20));break;}
}
static unsigned expectedLimit(unsigned id){return id<2?99:id==2?10:id==3||id==4||id==6||id==8?9:id==7||tradeGear(id)?1:0;}
static bool offered(const Game& g,unsigned id){
 if(!expectedLimit(id))return false;
 if(id>=2&&id<=4)return id==unsigned(localGood(g.city))+2;
 if(id==8)return g.city==1;
 if(tradeGear(id))return cityStock(g,id-19)&&gearAllowed(id-19,g.p.cls,g.p.level);
 return true;
}
static void modelPurchases(){
 for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls)for(unsigned level:{1u,4u,8u,20u})for(unsigned id=0;id<40;++id){
  auto g=hero(city,cls,level),before=g;PurchaseQuote q;const bool eligible=offered(g,id);
  assert(purchaseLimit(id)==expectedLimit(id));assert((purchaseAvailability(g,id)==nullptr)==eligible);
  assert((quotePurchase(g,q,id)==nullptr)==eligible);same(g,before);
  if(!eligible){assert(!q.open&&purchaseMaximum(g,id)==0);++cases;continue;}
  const unsigned limit=expectedLimit(id),unit=purchaseUnit(g,id);
  assert(unit&&q.open&&q.item==id&&q.city==city&&q.stock==0&&q.quantity==1&&q.unit==unit);
  assert(purchaseMaximum(g,id)==limit);
  for(unsigned bad:{0u,limit+1,255u,65535u,UINT_MAX})assert(purchaseError(g,id,bad));
  for(unsigned bad:{0u,255u}){q.quantity=bad;assert(confirmPurchase(g,q));same(g,before);assert(q.open);}
  q.quantity=limit;assert(!confirmPurchase(g,q)&&!q.open);auto expected=before;stock(expected,id,limit);expected.p.gold-=unit*limit;same(g,expected);assert(persistentValid(g));
  auto settled=g;assert(confirmPurchase(g,q));same(g,settled);assert(purchaseMaximum(g,id)==0&&purchaseError(g,id,1));
  // Bag space wins over gold, and zero/full edge states never underflow.
  for(unsigned occupied:{0u,limit-1,limit})for(unsigned gold:{0u,unit-1,unit,unit*2-1,999999u}){
   g=before;stock(g,id,occupied);g.p.gold=gold;const unsigned maximum=std::min(limit-occupied,gold/unit);
   assert(purchaseMaximum(g,id)==maximum);assert(!quotePurchase(g,q,id));q.quantity=maximum?maximum:1;auto original=g;
   if(maximum){assert(!confirmPurchase(g,q));stock(original,id,occupied+maximum);original.p.gold-=maximum*unit;same(g,original);}
   else {assert(confirmPurchase(g,q));same(g,original);}++cases;
  }
  // A quote checks stock/city/unit again and validates changed funds, phase,
  // class and level before any inventory or gold mutation.
  for(unsigned change=0;change<5;++change){g=before;assert(!quotePurchase(g,q,id));q.quantity=limit;
   if(change==0)stock(g,id,1);else if(change==1)g.city=(city+1)%4;else if(change==2)g.p.gold=unit-1;else if(change==3)g.phase=Phase::Enemy;else q.unit=unit+1;
   auto changed=g;assert(confirmPurchase(g,q));same(g,changed);++cases;
  }
  if(tradeGear(id)&&gearFamily(id-19)<4){g=before;assert(!quotePurchase(g,q,id));g.p.cls=(cls+1)%4;auto changed=g;assert(confirmPurchase(g,q));same(g,changed);}
  if(tradeGear(id)&&gearLevel(id-19)>1){g=before;assert(!quotePurchase(g,q,id));g.p.level=1;auto changed=g;assert(confirmPurchase(g,q));same(g,changed);}
  ++cases;
 }
 auto g=hero();PurchaseQuote q;
 for(unsigned id:{255u,256u,65535u,UINT_MAX}){assert(purchaseAvailability(g,id)&&purchaseMaximum(g,id)==0&&quotePurchase(g,q,id));assert(!q.open);}
 // Rations use the established Save30 capacity of 10, not the old 9-unit UI assumption.
 g=hero(0);g.rations=9;assert(purchaseMaximum(g,2)==1);assert(!quotePurchase(g,q,2)&&!confirmPurchase(g,q)&&g.rations==10);
}
static void modelSales(){
 for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls)for(unsigned id=0;id<40;++id){
  auto g=hero(city,cls);SaleQuote q;
  if(!tradable(id)){auto before=g;assert(quoteSale(g,q,id)&&!q.open&&saleMaximum(g,id)==0);same(g,before);++cases;continue;}
  const unsigned count=id<2?99:id==2?10:id==7||tradeGear(id)?1:9;stock(g,id,count);const unsigned unit=tradeUnit(g,id);assert(unit);
  for(unsigned room:{0u,unit-1,unit,unit*2-1,unit*count}){
   auto h=g;h.p.gold=999999-room;const unsigned maximum=std::min(count,room/unit);assert(saleMaximum(h,id)==maximum);
   auto before=h;assert((quoteSale(h,q,id)==nullptr)==(maximum>0));
   if(maximum){
    for(unsigned bad:{0u,count+1,255u,UINT_MAX})assert(saleError(h,id,bad));
    q.quantity=maximum+1;assert(confirmSale(h,q));same(h,before);
    q.quantity=maximum;assert(!confirmSale(h,q)&&!q.open);stock(before,id,count-maximum);before.p.gold+=maximum*unit;same(h,before);
    auto settled=h;assert(confirmSale(h,q));same(h,settled);assert(persistentValid(h));
   }else same(h,before);++cases;
  }
  for(unsigned change=0;change<5;++change){auto h=g;assert(!quoteSale(h,q,id));q.quantity=count;
   if(change==0)stock(h,id,0);else if(change==1)h.city=(city+1)%4;else if(change==2)h.p.gold=999999;else if(change==3)h.phase=Phase::Enemy;else ++q.unit;
   auto changed=h;assert(confirmSale(h,q));same(h,changed);++cases;
  }
  if(tradeGear(id)){g.equipped[gearSlot(id-19)]=id-19;auto before=g;assert(saleMaximum(g,id)==0&&quoteSale(g,q,id));same(g,before);}
 }
 // Crystals and quest identifiers cannot be turned into sale stock. Ordinary
 // trading while a contract is active must leave that contract unchanged.
 auto g=hero();g.crystals=9;g.questId=1;g.questLevel=g.p.level;g.questProgress=1;auto before=g;
 for(unsigned id:{8u,9u,10u,11u,12u,13u,14u,15u,16u,17u,18u,19u,255u,256u,UINT_MAX}){SaleQuote q;assert(quoteSale(g,q,id)&&saleMaximum(g,id)==0);same(g,before);}
 g.p.life=3;SaleQuote q;assert(!quoteSale(g,q,0));q.quantity=3;assert(!confirmSale(g,q));assert(g.crystals==9&&g.questId==before.questId&&g.questLevel==before.questLevel&&g.questProgress==before.questProgress);
 // Every state which blocks shop trading must block both batch directions.
 for(unsigned state=0;state<10;++state){g=hero();g.p.life=3;
  switch(state){case 0:g.phase=Phase::Hero;break;case 1:g.campStage=1;break;case 2:g.tripStage=1;break;case 3:g.dungeonFlags=1;break;case 4:g.discovery=1;break;case 5:g.campaignStage=1;break;case 6:g.eventStage=2;break;case 7:g.clubStage=1;break;case 8:g.clubStage=2;break;default:g.p.gold=1000000;break;}
  before=g;PurchaseQuote purchase;SaleQuote sale;assert(quotePurchase(g,purchase,0)&&quoteSale(g,sale,0));assert(purchaseMaximum(g,0)==0&&saleMaximum(g,0)==0);same(g,before);++cases;
 }
}

static void escortPurchases(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned tier=1;tier<4;++tier){
  auto g=hero(tier,cls);g.eventStage=2;g.eventKind=2;g.eventTier=tier;g.eventLevel=tier==1?5:tier==2?10:18;g.eventDay=20733;g.eventOriginPage=9;g.p.hp=1;g.p.mp=0;
  assert(persistentValid(g)&&escortCampAllowed(g));auto before=g;PurchaseQuote kit;assert(!quotePurchase(g,kit,7)&&purchaseMaximum(g,7)==1&&!confirmPurchase(g,kit));
  auto expected=before;expected.sleepKit=true;expected.p.gold-=80;same(g,expected);assert(persistentValid(g));
  for(unsigned id=0;id<38;++id)if(id!=7){g=before;PurchaseQuote q;SaleQuote sale;assert(quotePurchase(g,q,id)&&purchaseMaximum(g,id)==0&&quoteSale(g,sale,id)&&saleMaximum(g,id)==0);same(g,before);++cases;}
  g=before;g.sleepKit=true;SaleQuote sale;assert(quoteSale(g,sale,7));
  for(unsigned kind=0;kind<8;++kind)if(kind!=2){g=before;g.eventKind=kind;auto unchanged=g;assert(quotePurchase(g,kit,7));same(g,unchanged);++cases;}
  for(unsigned change=0;change<12;++change){g=before;
   switch(change){case 0:g.eventProgress=1;break;case 1:g.city=0;break;case 2:g.phase=Phase::Hero;break;case 3:g.campStage=1;break;case 4:g.tripStage=1;break;case 5:g.dungeonFlags=1;break;case 6:g.discovery=1;break;case 7:g.campaignStage=1;break;case 8:g.clubStage=1;break;case 9:g.clubStage=2;break;case 10:g.p.gold=1000000;break;default:g.eventTier=0;break;}
   auto unchanged=g;assert(quotePurchase(g,kit,7)&&purchaseMaximum(g,7)==0);same(g,unchanged);++cases;
  }
 }
}

// Faults operate at the blob boundary used by the actual production journal.
// They simulate host persistence, not power loss on physical ESP flash.
enum class Failure {None,BeforeWrite,FalseAck,Readback,Torn};
struct PowerCut {};
struct FaultStore {
 bool ready=true;std::map<std::string,std::vector<uint8_t>> blobs;Failure failure=Failure::None;
 std::string failedRead;unsigned operations=0,cut=0,tornBytes=0;bool cutAfter=false;
 void before(){++operations;if(cut==operations&&!cutAfter)throw PowerCut{};}
 void after(){if(cut==operations&&cutAfter)throw PowerCut{};}
 Read presence(const char* key){before();auto result=blobs.count(key)?Read::Ok:Read::Missing;after();return result;}
 Read read(const char* key,uint8_t* b,unsigned n){before();Read result=Read::Missing;
  if(failedRead==key){failedRead.clear();result=Read::Error;}else {auto it=blobs.find(key);if(it!=blobs.end()){result=it->second.size()==n?Read::Ok:Read::Corrupt;if(result==Read::Ok)memcpy(b,it->second.data(),n);}}
  after();return result;
 }
 bool write(const char* key,const uint8_t* b,unsigned n){before();auto fault=failure;failure=Failure::None;
  if(fault==Failure::BeforeWrite){after();return false;}
  blobs[key]={b,b+(fault==Failure::Torn?std::min(tornBytes,n):n)};
  if(fault==Failure::Readback)failedRead=key;
  after();return fault!=Failure::FalseAck&&fault!=Failure::Torn;
 }
 bool erase(const char* key){before();blobs.erase(key);after();return true;}
};
using Backend=PersistentSlotBackend<FaultStore>;
using BulkJournal=PersistentJournal<Backend>;
static Game reboot(FaultStore& store,unsigned slot){Backend b(store);b.slot=slot;BulkJournal j(b);Game g;auto loaded=j.load(g);assert(loaded==Load::Ok||loaded==Load::Recovered);return g;}
static void seed(FaultStore& store,const Game& current,unsigned slot,bool migration){
 for(unsigned s=0;s<3;++s){Backend b(store);b.slot=s;BulkJournal j(b);Game g;assert(j.load(g)==Load::Empty);g=s==slot?current:hero(s,s);g.p.gold+=s;assert(j.save(g));
  if(s==slot&&migration){auto& bytes=store.blobs[b.key(false)];encodePersistentVersion(g,1,s,bytes.data(),29);assert(get16(bytes.data(),4)==29);}}
}
static void otherSlots(const FaultStore& store,const FaultStore& before,unsigned slot){
 for(unsigned s=0;s<3;++s)if(s!=slot){FaultStore original=before,actual=store;same(reboot(actual,s),reboot(original,s));for(const char* key:{s==0?"a":s==1?"a1":"a2",s==0?"b":s==1?"b1":"b2",s==0?"m0":s==1?"m1":"m2"}){auto a=store.blobs.find(key),b=before.blobs.find(key);assert((a==store.blobs.end())==(b==before.blobs.end()));if(a!=store.blobs.end())assert(a->second==b->second);}}
}
static Game transact(const Game& before,unsigned id,bool selling){auto after=before;
 if(selling){SaleQuote q;assert(!quoteSale(after,q,id));q.quantity=saleMaximum(after,id);assert(!confirmSale(after,q));assert(confirmSale(after,q));}
 else {PurchaseQuote q;assert(!quotePurchase(after,q,id));q.quantity=purchaseMaximum(after,id);assert(!confirmPurchase(after,q));assert(confirmPurchase(after,q));}
 assert(persistentValid(after));return after;
}
static void persistence(){
 static_assert(PERSISTENT_SAVE_FORMAT==30&&CURRENT_SAVE_FORMAT==30&&SAVE_RESET_EPOCH==1,"Bulk commerce must not change save format or epoch");
 for(unsigned slot=0;slot<3;++slot)for(bool migration:{false,true})for(bool selling:{false,true})for(unsigned id:{0u,1u,2u,3u,4u,5u,6u,7u,8u,20u,32u,35u}){
  if((selling&&!tradable(id))||(!selling&&id==5))continue;
  unsigned city=id==3?2:id==4?3:id==8?1:0;auto current=hero(city);if(selling)stock(current,id,id==7||tradeGear(id)?1:id<2?11:5);
  FaultStore original;seed(original,current,slot,migration);const auto before=reboot(original,slot),after=transact(before,id,selling);
  for(auto failure:{Failure::None,Failure::BeforeWrite,Failure::FalseAck,Failure::Readback,Failure::Torn}){
   auto store=original;Backend b(store);b.slot=slot;BulkJournal j(b);Game g;assert(j.load(g)==Load::Ok);same(g,before);g=transact(g,id,selling);
   store.failure=failure;store.tornBytes=79;const bool saved=j.save(g);
   assert(saved==(failure==Failure::None||failure==Failure::FalseAck));same(g,after); // RAM retains exactly one pending batch.
   auto observed=reboot(store,slot);same(observed,failure==Failure::BeforeWrite||failure==Failure::Torn?before:after);
   assert(j.save(g));same(reboot(store,slot),after);assert(get16(store.blobs[b.key(j.active)].data(),4)==30);otherSlots(store,original,slot);++cases;
  }
  // Every byte-shortened record must reboot to the previous entire transaction.
  for(unsigned length:{0u,1u,16u,63u,84u,124u,143u,155u,159u}){
   auto store=original;Backend b(store);b.slot=slot;BulkJournal j(b);Game g;assert(j.load(g)==Load::Ok);g=after;store.failure=Failure::Torn;store.tornBytes=length;assert(!j.save(g));same(reboot(store,slot),before);assert(j.save(g));same(reboot(store,slot),after);otherSlots(store,original,slot);++cases;
  }
  auto counted=original;Backend cb(counted);cb.slot=slot;BulkJournal cj(cb);Game cg;assert(cj.load(cg)==Load::Ok);counted.operations=0;assert(cj.save(after));const unsigned boundaries=counted.operations;
  for(unsigned cut=1;cut<=boundaries;++cut)for(bool cutAfter:{false,true}){
   auto store=original;Backend b(store);b.slot=slot;BulkJournal j(b);Game g;assert(j.load(g)==Load::Ok);store.operations=0;store.cut=cut;store.cutAfter=cutAfter;
   try{j.save(after);}catch(const PowerCut&){}store.cut=0;auto observed=reboot(store,slot);assert(packed(observed)==packed(before)||packed(observed)==packed(after));otherSlots(store,original,slot);++cases;
  }
 }
}
int main(){modelPurchases();modelSales();escortPurchases();persistence();std::printf("PASS: %u bulk-commerce model and real Save30 journal cases; 4 cities/classes, caps, stale/double quotes, protected items, Save29 migration, 3 slots and all-or-none fault/reboot boundaries.\n",cases);}
