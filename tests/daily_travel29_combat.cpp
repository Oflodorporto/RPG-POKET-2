#include "../firmware/RPG_POKET_2/Persistence.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
unsigned journeys=0, snapshots=0, won=0, lost=0, fled=0;
void check(const Game& g){assert(persistentValid(g));uint8_t b[PERSISTENT_SAVE_SIZE],c[PERSISTENT_SAVE_SIZE];encodePersistent(g,snapshots,1,b);Game h;uint32_t s;assert(decodePersistent(b,1,h,s)==Decode::Ok);encodePersistent(h,s,1,c);assert(!memcmp(b,c,sizeof b));++snapshots;}
int main(){for(unsigned cls=0;cls<4;cls++)for(unsigned level=1;level<=20;level++)for(unsigned origin=0;origin<4;origin++)for(unsigned dest=0;dest<4;dest++)if(origin!=dest)for(unsigned seed=1;seed<=8;seed++){
auto g=create(cls,seed*977+level);g.p.xp=dndXp[level-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=true;g.city=origin;g.rations=9;g.charts=g.charms=1;g.sleepKit=seed&1;g.fatigue=seed*12;check(g);assert(!prepareDailyTrip(g,dest));check(g);unsigned loops=0;
while(dailyTrip(g)) {assert(++loops<10000);assert(g.rations+g.daily.rationsPaid==9);check(g);
switch(g.daily.phase){case DailyPhase::RoadRoll:assert(acceptDailyRoad(g));break;case DailyPhase::Leg:assert(finishDailyLeg(g));break;case DailyPhase::NightRoll:assert(acceptDailyNight(g));break;case DailyPhase::Rest:assert(finishDailyRest(g));break;case DailyPhase::Tent:assert(continueDailyTrip(g));break;case DailyPhase::RoadBattle:case DailyPhase::NightBattle:if(g.phase==Phase::Hero){const Action a=Action((loops+seed)%18);if(act(g,a))assert(!act(g,Action::Attack));}else if(g.phase==Phase::Enemy)assert(enemy(g));else {won+=g.phase==Phase::Won;lost+=g.phase==Phase::Lost;fled+=g.phase==Phase::Fled;assert(resolveDailyBattle(g));}break;default:assert(false);}
check(g);}
assert(g.city==origin||g.city==dest);assert(g.city==dest||g.p.hp==1);assert(g.charts==0&&g.charms==0);++journeys;
}printf("PASS independent real-action daily simulation: %u journeys, %u snapshots, %u wins, %u defeats, %u flees across 4 classes/20 levels/12 routes\n",journeys,snapshots,won,lost,fled);}
