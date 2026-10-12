#define main originalTitleTests
#include "title_controller.cpp"
#undef main
#include <fstream>
int main(int argc,char** argv){using namespace rpg;
 unsigned transactions=0,cycles=0;std::ofstream report;if(argc>1){report.open(argv[1]);report<<"level,city,hunt_gold,gear_set_cost,hp_cost,mp_cost,maximum_surplus_sale,local_contract_available,missions_for_gear\n";}
 for(unsigned level:{1u,5u,8u,12u,15u,18u,20u})for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city){auto g=create(cls,17);g.p.xp=dndXp[level-1];levelUp(g);g.city=city;g.p.gold=10000;g.p.life=g.p.mana=99;g.rations=g.charts=g.charms=g.scrap=g.gazuas=9;g.sleepKit=true;g.owned=(1u<<18)-1;
  unsigned total=0;for(unsigned item=0;item<38;++item){if(!tradable(item))continue;auto h=g;SaleQuote q;assert(!quoteSale(h,q,item));auto before=h; q.quantity=tradeStock(h,item)+1;assert(confirmSale(h,q)&&h.p.gold==before.p.gold&&tradeStock(h,item)==tradeStock(before,item));q.quantity=tradeStock(h,item);auto value=q.unit*q.quantity;assert(!confirmSale(h,q)&&h.p.gold==before.p.gold+value&&tradeStock(h,item)==0);auto paid=h;assert(confirmSale(h,q)&&h.p.gold==paid.p.gold);assert(valid(h));uint8_t bytes[SAVE_SIZE],copy[SAVE_SIZE];encode(h,31,bytes);Game boot;uint32_t seq;assert(decode(bytes,boot,seq)==Decode::Ok);encode(boot,seq,copy);assert(!memcmp(bytes,copy,SAVE_SIZE));total+=value;++transactions;}
  SaleQuote denied;assert(quoteSale(g,denied,8)&&quoteSale(g,denied,255));g.equipped[0]=cls*3+1;assert(quoteSale(g,denied,19+g.equipped[0]));g.equipped[0]=0;
  unsigned gearCost=0;for(unsigned slot=0;slot<3;++slot){unsigned best=0;for(unsigned i=0;i<cityGearCount(g);++i){auto id=cityGearOffer(g,i);if(gearSlot(id)==slot&&gearAllowed(id,cls,level)&&gearTier(id)>gearTier(best))best=id;}if(best)gearCost+=cityGearPrice(g,best);}
  auto id=contractOffer(city,0);unsigned income=contractGold(g,id,level)+contract(id).count*enemySpec(city==0?1:city==1?2:city==2?25:27).gold;
  if(report&&cls==0)report<<level<<','<<city<<','<<income<<','<<gearCost<<','<<unsigned(potionPrice(g,false))<<','<<unsigned(potionPrice(g,true))<<','<<total<<','<<(level>=contractLevel(id))<<','<<(level>=contractLevel(id)?(gearCost+income-1)/income:0)<<'\n';
 }
 // All regional buy/resell pairs are strictly loss-making, including gazuas and rations.
 for(unsigned item=0;item<38;++item)if(tradable(item)&&item!=5)for(unsigned a=0;a<4;++a)for(unsigned b=0;b<4;++b){auto h=create(0,13);h.p.xp=dndXp[19];levelUp(h);h.city=a;h.p.gold=10000;unsigned old=h.p.gold;const char* err=nullptr;
  if(tradeGear(item)){unsigned id=item-19;h.p.cls=gearFamily(id)<4?gearFamily(id):0;h.p.maxmp=totalMana(h);if(!cityStock(h,id))continue;err=buyGear(h,id);}
  else if(item<2)err=buyPotion(h,item==1);else if(item==6)err=buyGazua(h);else if(item==7)err=buySleepKit(h);else {if(localGood(a)!=item-2)continue;err=buyGood(h);}
  assert(!err);h.city=b;SaleQuote q;assert(!quoteSale(h,q,item));assert(!confirmSale(h,q)&&h.p.gold<old);++cycles;
 }
 auto veteran=create(0,42);veteran.p.xp=dndXp[7];levelUp(veteran);veteran.city=1;veteran.owned=1u<<17;assert(!cityStock(veteran,18)&&!equipGear(veteran,18)&&valid(veteran));
 // Existing migration suites run unchanged; no save format change.
 // Actual controller: quantities, cancel, stale quote, protected gear, duplicate taps, save failure/retry, three slots.
 nvs=Store{};for(unsigned slot=0;slot<3;++slot){backend.slot=slot;journal.active=-1;journal.seq=0;journal.blocked=false;game=create(0,41+slot);game.tutorial=true;game.p.gold=1000+slot;game.p.life=3;assert(journal.save(game));}
 backend.slot=0;assert(journal.load(game)==Load::Ok);auto untouched=nvs.blobs;view.page=Page::Market;tapped(60,290);assert(view.page==Page::TradeList);tapped(120,242);assert(view.page==Page::TradeConfirm);tapped(175,191);assert(view.sale.quantity==2);tapped(60,290);assert(view.page==Page::TradeList&&nvs.blobs==untouched&&game.p.life==3);
 tapped(120,242);tapped(175,191);nvs.fail=true;tapped(175,290);assert(view.page==Page::SaveError&&!view.sale.open&&game.p.life==1&&game.p.gold==1010);Game before;assert(journal.load(before)==Load::Ok&&before.p.life==3&&before.p.gold==1000);nvs.fail=false;tapped(110,269);assert(view.page==Page::TradeReceipt&&game.p.life==1&&game.p.gold==1010);tapped(175,290);tapped(175,290);assert(game.p.life==1&&game.p.gold==1010);assert(journal.load(before)==Load::Ok&&before.p.life==1&&before.p.gold==1010);
 for(unsigned slot=1;slot<3;++slot){backend.slot=slot;assert(journal.load(before)==Load::Ok&&before.p.gold==1000+slot&&before.p.life==3);}backend.slot=0;assert(journal.load(game)==Load::Ok);view.page=Page::TradeList;tapped(120,242);auto q=view.sale;game.p.life=2;assert(confirmSale(game,q)&&game.p.gold==1010);game.p.life=1;
 view.sale=q;selectSlot(1);assert(!view.sale.open);view.sale=q;openTitle();assert(!view.sale.open);
 auto cap=create(0,3);cap.p.life=2;cap.p.gold=999999;SaleQuote quote;assert(quoteSale(cap,quote,0));cap.p.gold=999990;assert(!quoteSale(cap,quote,0));quote.quantity=2;assert(confirmSale(cap,quote)&&cap.p.life==2&&cap.p.gold==999990);quote.quantity=0;assert(confirmSale(cap,quote));
 printf("PASS:commerce75 %u inventory transactions, %u cross-region loss-making cycles; cancellation/quantity/caps/protection/actual taps; failed-save retry/reboot and3slots.\n",transactions,cycles);
}
