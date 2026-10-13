#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Persistence.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace rpg;
static unsigned checks=0,routes=0,nights=0,roadDanger=0,nightDanger=0;
static std::vector<uint8_t> bytes(const Game& g){std::vector<uint8_t> b(PERSISTENT_SAVE_SIZE);encodePersistent(g,17,2,b.data());return b;}
static void check(Game& g){
  assert(dailyTravelValid(g));assert(persistentValid(g));auto b=bytes(g);Game h;uint32_t sequence=0;
  assert(decodePersistent(b.data(),2,h,sequence)==Decode::Ok&&sequence==17&&bytes(h)==b);g=h;++checks;
}
static void noChange(const Game& g,const Game& old){assert(bytes(g)==bytes(old));}
static void win(Game& g){g.enemyHp=0;finish(g);assert(g.phase==Phase::Won);check(g);auto old=g;finish(g);noChange(g,old);assert(!home(g));noChange(g,old);}
static void roadToLeg(Game& g){
  assert(acceptDailyRoad(g));check(g);
  if(g.daily.phase==DailyPhase::RoadBattle){++roadDanger;win(g);assert(resolveDailyBattle(g));check(g);}
  assert(g.daily.phase==DailyPhase::Leg);auto old=g;assert(!acceptDailyRoad(g));noChange(g,old);
}
static void allRoutes(){
  for(unsigned cls=0;cls<4;++cls)for(unsigned origin=0;origin<4;++origin)for(unsigned dest=0;dest<4;++dest)for(unsigned seed=1;seed<=120;++seed){
    if(origin==dest)continue;auto g=testHero(cls,seed);g.city=origin;g.race=(seed+cls)%4;g.shirt=seed%8;g.trousers=(seed+4)%8;
    g.rations=9;g.charts=g.charms=2;g.sleepKit=seed%2;g.fatigue=83;--g.p.hp;--g.p.mp;
    const unsigned days=dailyTripDays(origin,dest);assert(days==2*unsigned(abs(int(origin)-int(dest))));
    const auto identity=g;assert(!prepareDailyTrip(g,dest));check(g);assert(g.rations==8&&g.charts==1&&g.charms==1);
    auto old=g;assert(prepareDailyTrip(g,dest));noChange(g,old);assert(g.daily.totalDays==days&&g.city==origin);
    roadToLeg(g);
    for(unsigned day=1;day<=days;++day){
      assert(g.daily.day==day&&g.daily.rationsPaid==day&&g.daily.nightsDone==day-1&&g.rations==9-day);
      assert(finishDailyLeg(g));check(g);
      if(day==days){assert(!dailyTrip(g)&&g.city==dest&&g.rations==9-days);break;}
      assert(g.city==origin&&g.daily.phase==DailyPhase::NightRoll&&g.daily.region==origin+(dest>origin?int(day/2):-int(day/2)));
      old=g;assert(!finishDailyLeg(g));noChange(g,old);const auto rolled=g.randomState;
      check(g);assert(g.randomState==rolled);const bool safe=dailyNightSafe(g);assert(acceptDailyNight(g));check(g);
      if(!safe){++nightDanger;assert(g.daily.phase==DailyPhase::NightBattle);win(g);assert(resolveDailyBattle(g));check(g);}
      assert(g.daily.phase==DailyPhase::Rest&&g.daily.nightsDone==day&&g.p.hp==g.p.maxhp&&g.p.mp==g.p.maxmp&&!g.fatigue);
      old=g;assert(!acceptDailyNight(g)&&!resolveDailyBattle(g)&&!continueDailyTrip(g));noChange(g,old);
      assert(finishDailyRest(g));check(g);assert(g.daily.phase==DailyPhase::Tent&&g.rations==9-day);
      old=g;assert(!finishDailyRest(g)&&!finishDailyLeg(g));noChange(g,old);
      assert(continueDailyTrip(g));check(g);old=g;assert(!continueDailyTrip(g));noChange(g,old);++nights;
    }
    assert(g.p.cls==identity.p.cls&&g.race==identity.race&&g.shirt==identity.shirt&&g.trousers==identity.trousers);
    assert(g.charts==1&&g.charms==1);old=g;assert(!finishDailyLeg(g));noChange(g,old);++routes;
  }
}
static Game endangered(bool night,unsigned escort){
  for(unsigned seed=1;seed<10000;++seed){
    auto g=testHero(0,seed);g.city=1;g.rations=9;g.p.xp=80;g.p.hp=7;g.p.mp=3;g.fatigue=71;
    if(escort==1){g.questId=9;g.questLevel=3;}
    if(escort==2){g.p.level=5;g.p.maxmp=totalMana(g);g.eventStage=2;g.eventDay=20733;g.eventKind=2;g.eventTier=1;g.eventLevel=5;g.eventOriginPage=2;}
    assert(!prepareDailyTrip(g,0));if(!night){if(tripSafe(g))continue;assert(acceptDailyRoad(g));check(g);return g;}
    roadToLeg(g);assert(finishDailyLeg(g));if(dailyNightSafe(g))continue;
    assert(acceptDailyNight(g));check(g);return g;
  }assert(false);return Game{};
}
static void outcomes(){
  for(bool night:{false,true})for(unsigned escort=0;escort<3;++escort)for(auto outcome:{Phase::Won,Phase::Lost,Phase::Fled}){
    auto g=endangered(night,escort);const auto origin=g.city;const auto before=g;
    if(outcome==Phase::Won)win(g);
    else if(outcome==Phase::Lost){g.p.hp=0;finish(g);check(g);}
    else {g.phase=Phase::Fled;clearMartialCombat(g);check(g);}
    const auto gold=g.p.gold,xp=g.p.xp;const auto hp=g.p.hp,mp=g.p.mp;const auto fatigue=g.fatigue,rations=g.rations;
    assert(resolveDailyBattle(g));check(g);assert(g.p.gold==gold&&g.p.xp==xp&&g.rations==rations&&g.city==origin);
    if(outcome==Phase::Lost){assert(!dailyTrip(g)&&g.p.hp==1&&g.p.mp==mp&&g.fatigue==fatigue&&xp<before.p.xp);}
    else if(outcome==Phase::Fled&&escort){assert(!dailyTrip(g)&&g.p.hp==hp&&g.p.mp==mp&&g.fatigue==fatigue);}
    else if(!night){assert(g.daily.phase==DailyPhase::Leg&&g.p.hp==hp&&g.p.mp==mp&&g.fatigue==fatigue);}
    else if(outcome==Phase::Fled){assert(g.daily.phase==DailyPhase::Tent&&g.daily.result==3&&g.p.hp==hp&&g.p.mp==mp&&g.fatigue==fatigue);}
    else {assert(g.daily.phase==DailyPhase::Rest&&g.daily.result==2&&g.p.hp==g.p.maxhp&&g.p.mp==g.p.maxmp&&!g.fatigue);}
    assert(!g.questProgress&&!g.eventProgress);auto old=g;assert(!resolveDailyBattle(g));noChange(g,old);
  }
  // Completed escorts advance exactly once at arrival and never award mission payment here.
  for(unsigned mode=1;mode<3;++mode){auto g=endangered(false,mode);win(g);assert(resolveDailyBattle(g));const auto gold=g.p.gold;assert(finishDailyLeg(g));assert(acceptDailyNight(g));
    if(dailyBattle(g)){win(g);assert(resolveDailyBattle(g));}assert(finishDailyRest(g));assert(continueDailyTrip(g));
    const auto beforeArrival=g;assert(finishDailyLeg(g));check(g);assert(g.city==0&&(mode==1?g.questProgress:g.eventProgress)==1&&g.p.gold==beforeArrival.p.gold&&g.p.gold>=gold);
    auto old=g;assert(!finishDailyLeg(g));noChange(g,old);
  }
}
static void supplyAndRegion(){
  for(unsigned n=0;n<6;++n){auto g=testHero(0,3);g.rations=n;g.charts=g.charms=2;auto old=g;assert(dailyTripError(g,3)&&prepareDailyTrip(g,3));noChange(g,old);}
  for(unsigned cls=0;cls<4;++cls){auto g=create(cls,55);g.rations=9;g.sleepKit=true;g.fatigue=99;g.p.hp=1;g.p.mp=0;
    if(cls==1)g.laySpent=1;if(cls==2)g.windSpent=1;if(cls==3)g.rageSpent=1;
    assert(!prepareDailyTrip(g,1));roadToLeg(g);assert(finishDailyLeg(g));
    // Select a naturally safe result, without reusing RNG on acceptance.
    g.daily.roll=20;g.daily.total=20+g.daily.survivalBonus+g.daily.luckBonus+2*g.daily.kit;check(g);
    assert(acceptDailyNight(g));check(g);assert(!g.laySpent&&!g.windSpent&&!g.rageSpent&&!g.fatigue&&g.p.hp==g.p.maxhp&&g.p.mp==g.p.maxmp);
  }
  // Encounter scaling uses the camp region, never the unchanged departure city.
  auto g=create(0,2);g.tripStage=4;g.city=0;g.daily.phase=DailyPhase::NightBattle;g.daily.region=1;g.enemyId=0;
  assert(encounterHp(g,0)==enemySpec(0).hp&&encounterAttack(g)==enemySpec(0).atk);
  g.city=3;g.daily.region=0;assert(encounterHp(g,0)==6&&encounterAttack(g)==2);
  for(unsigned r=0;r<4;++r)for(unsigned seed=0;seed<16;++seed)assert(dailyEnemyInRegion(r,regionalEnemyAt(g,r,seed,true)));
}
int main(){allRoutes();outcomes();supplyAndRegion();assert(roadDanger&&nightDanger);printf("PASS daily29: %u routes, %u nights, %u persisted/reboot checks, %u road and %u night dangers; supply exactness, repeated input, all outcomes, escort, identity and regional scaling\n",routes,nights,checks,roadDanger,nightDanger);}
