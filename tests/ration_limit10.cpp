// Reuse production Save29/30 journal fault injection, without running its main.
#define main existingSave29RegressionMain
#include "save29.cpp"
#undef main

// Captured from baseline dfc68c49f3264d4f2d3642266d44b82ca9b7c563,
// before the ration limit changed. This fixture is a canonical Save29 record.
static const uint8_t historicalRation9[PERSISTENT_SAVE_SIZE]={
 0x50,0x4b,0x54,0x32,0x1d,0x00,0xa0,0x00,0x7b,0x00,0x00,0x00,0x02,0x01,0x08,0x05,
 0x02,0x02,0x0c,0x00,0x0c,0x00,0x08,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x41,0x01,
 0x00,0x00,0x00,0x00,0x11,0x00,0x78,0x56,0x34,0x12,0x00,0x00,0x00,0x00,0x00,0x10,
 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x03,0x04,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x10,0x0e,0x0e,0x08,0x0c,0x0a,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x43,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x71,0xa5,0xea,0x04
};
static unsigned rationChecks=0;
static void rationRoundtrip(Game& g,unsigned slot=0){
 assert(persistentValid(g));const auto before=packed(g,123,slot);Game boot;uint32_t seq=0;
 assert(decodePersistent(before.data(),slot,boot,seq)==Decode::Ok&&seq==123&&same(g,boot));
 assert(boot.randomState==g.randomState);g=boot;++rationChecks;
}
static void rationCodec(){
 static_assert(RATION_LIMIT==10&&PERSISTENT_SAVE_FORMAT==30&&CURRENT_SAVE_FORMAT==30&&SAVE_RESET_EPOCH==1,"Keep Save30/epoch1");
 static_assert(PERSISTENT_SAVE_SIZE==160&&SAVE_SIZE==128&&sizeof(Game{}.rations)==1,"No new save bytes or inventory fields");
 auto historical=create(2,0x12345678);historical.rations=9;historical.p.gold=321;historical.race=3;historical.shirt=4;historical.trousers=5;historical.fatigue=67;historical.tutorial=true;
 for(unsigned format:{29u,30u})for(unsigned n=0;n<=RATION_LIMIT;++n){
  auto expected=std::vector<uint8_t>(historicalRation9,historicalRation9+PERSISTENT_SAVE_SIZE);
  put16(expected.data(),4,format);expected[84]=n;put32(expected.data(),156,crc(expected.data(),156));
  if(format==30&&n==9)assert(get32(expected.data(),156)==0x6ff0dde3u);
  auto g=historical;g.rations=n;std::vector<uint8_t> b(PERSISTENT_SAVE_SIZE);encodePersistentVersion(g,123,2,b.data(),format);
  assert(b==expected);Game h;uint32_t seq=0;assert(decodePersistent(expected.data(),2,h,seq)==Decode::Ok&&seq==123&&same(g,h));++rationChecks;
 }
 for(unsigned format:{29u,30u})for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls)for(unsigned n=0;n<256;++n){
  auto g=hero(cls);g.rations=n;assert(persistentValid(g)==(n<=RATION_LIMIT));
  std::vector<uint8_t> b(PERSISTENT_SAVE_SIZE);encodePersistentVersion(g,41,slot,b.data(),format);
  Game restored=hero((cls+1)%4);const auto old=restored;uint32_t seq=77;
  const auto status=decodePersistent(b.data(),slot,restored,seq);
  if(n<=RATION_LIMIT)assert(status==Decode::Ok&&restored.rations==n&&same(restored,g)&&seq==41);
  else assert(status==Decode::Corrupt&&same(restored,old)&&seq==77);
  ++rationChecks;
 }
 // All unrelated capacities retain their old boundaries.
 auto g=hero(0);g.rations=RATION_LIMIT;g.charts=g.charms=g.scrap=g.gazuas=g.crystals=9;g.p.life=g.p.mana=99;assert(persistentValid(g));
 for(unsigned i=0;i<7;++i){auto bad=g;uint8_t* count[]={&bad.charts,&bad.charms,&bad.scrap,&bad.gazuas,&bad.crystals,&bad.p.life,&bad.p.mana};++*count[i];assert(!persistentValid(bad));++rationChecks;}
}
static void rationShop(){
 for(unsigned city=0;city<4;++city){auto g=hero(0);g.city=city;g.p.gold=10000;g.rations=g.charts=g.charms=0;
  const auto id=localGood(city);const unsigned limit=id==0?10:9;const unsigned price[]={6,12,12,25};
  assert(goodLimit(id)==limit&&goodPrice(city)==price[city]);
  for(unsigned n=0;n<limit;++n){auto before=g;assert(!buyGood(g));assert(goodCount(g,id)==n+1&&g.p.gold==before.p.gold-price[city]&&g.randomState==before.randomState);rationRoundtrip(g);}
  auto full=g;const char* error=buyGood(g);assert(error&&same(full,g));assert(strstr(error,id==0?"10":"9"));
  goodCount(g,id)=limit-1;g.p.gold=price[city]-1;full=g;assert(buyGood(g)&&same(full,g));
 }
}
static void rationLootAndRewards(){
 for(unsigned kind:{1u,2u,3u})for(unsigned n:{8u,9u,10u}){auto g=hero(0);g.rations=n;g.discovery=kind;g.discoverLoot=3;g.discoverAmount=1;const auto old=g;
  assert(persistentValid(g));auto err=collectDiscovery(g);
  if(n==RATION_LIMIT)assert(err&&same(g,old));else {assert(!err&&g.rations==n+1);rationRoundtrip(g);const auto paid=g;assert(collectDiscovery(g)&&same(g,paid));}
  ++rationChecks;
 }
 for(unsigned loot:{1u,2u,4u,5u}){auto g=hero(0);g.discovery=1;g.discoverLoot=loot;g.discoverAmount=1;g.p.life=g.p.mana=99;g.scrap=9;const auto old=g;assert(collectDiscovery(g)&&same(g,old));++rationChecks;}
 for(unsigned cls=0;cls<4;++cls)for(unsigned n:{8u,9u,10u}){auto g=create(cls,43);g.p.xp=dndXp[4];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=true;g.rations=n;
  assert(offerEvent(g,20733,10));g.eventKind=7;assert(!acceptEvent(g,49));
  while(!eventReady(g)){assert(!eventAct(g,0));if(g.phase==Phase::Hero){g.enemyHp=0;finish(g);assert(resumeEventBattle(g));}rationRoundtrip(g);}
  const auto gold=g.p.gold;const auto reward=eventGold(g);assert(g.rations==n&&finishEvent(g));assert(g.rations==std::min(10u,n+1)&&g.p.gold==gold+reward);rationRoundtrip(g);
  const auto paid=g;assert(!finishEvent(g)&&same(g,paid));
 }
}
static void rationTravelAndCamp(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned origin=0;origin<4;++origin)for(unsigned dest=0;dest<4;++dest)if(origin!=dest){
  auto g=dailyHero(cls,origin,7);g.rations=RATION_LIMIT;assert(!prepareDailyTrip(g,dest)&&g.rations==RATION_LIMIT-1);rationRoundtrip(g);
  // Daily validation has an independent upper bound: 10 is representable,
  // 11 is corrupt even when ordinary travel and remaining-supply checks pass.
  auto full=g;full.rations=RATION_LIMIT;assert(dailyTravelValid(full));rationRoundtrip(full);full.rations=RATION_LIMIT+1;assert(!dailyTravelValid(full)&&!persistentValid(full));
  assert(acceptDailyRoad(g));
  while(dailyTrip(g)){
   assert(g.rations+g.daily.rationsPaid==RATION_LIMIT);rationRoundtrip(g);
   if(dailyBattle(g)){g.enemyHp=0;finish(g);assert(resolveDailyBattle(g));}
   else if(g.daily.phase==DailyPhase::Leg)assert(finishDailyLeg(g));
   else if(g.daily.phase==DailyPhase::NightRoll)assert(acceptDailyNight(g));
   else if(g.daily.phase==DailyPhase::Rest)assert(finishDailyRest(g));
   else if(g.daily.phase==DailyPhase::Tent)assert(continueDailyTrip(g));else assert(false);
  }
  assert(g.city==dest&&g.rations==RATION_LIMIT-dailyTripDays(origin,dest));rationRoundtrip(g);
 }
 auto g=hero(0);g.rations=RATION_LIMIT;g.fatigue=30;assert(!startCamp(g,true,false)&&g.rations==RATION_LIMIT-1);rationRoundtrip(g);auto paid=g;assert(startCamp(g,true,false)&&same(g,paid));
}
static void rationJournals(){
 for(unsigned slot=0;slot<3;++slot)for(unsigned format:{29u,30u}){
  Store s;for(unsigned other=0;other<3;++other){auto g=hero(other);g.rations=9;g.p.gold=1000+other;seed(s,other,g);}
  Backend io(s);io.slot=slot;
  if(format==29)for(bool side:{false,true}){auto& b=s.blobs[io.key(side)];Game old;uint32_t seq=0;assert(decodePersistent(b.data(),slot,old,seq)==Decode::Ok);encodePersistentVersion(old,seq,slot,b.data(),29);}
  const auto before=s;J j(io);Game old;assert(j.load(old)==Load::Ok&&old.rations==9&&s.blobs==before.blobs);
  auto next=old;assert(!buyGood(next)&&next.rations==RATION_LIMIT);verifySaveCuts(before,slot,old,next,false);
  assert(j.save(next));J boot(io);Game resumed;assert(boot.load(resumed)==Load::Ok&&same(resumed,next));assertOthers(before,s,slot);
  assert(get16(s.blobs[io.key(boot.active!=0)].data(),4)==30);assert(s.blobs[io.metaKey()]==before.blobs.at(io.metaKey()));
  auto full=resumed;assert(buyGood(resumed)&&same(resumed,full));auto invalid=full;invalid.rations=RATION_LIMIT+1;const auto stable=s.blobs;assert(!boot.save(invalid)&&s.blobs==stable);
 }
}
int main(){rationCodec();rationShop();rationLootAndRewards();rationTravelAndCamp();rationJournals();
 printf("PASS ration_limit10: %u codec/gameplay checks, %u power cuts, %u I/O/ACK retries; historical bytes, Save29/30 epoch1, 10 allowed/11+ rejected, limits/prices, loot/reward/camp/travel, all-slot migration\n",rationChecks,cuts,faults);
}
