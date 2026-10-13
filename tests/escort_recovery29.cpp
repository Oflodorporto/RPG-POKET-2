#include "../firmware/RPG_POKET_2/Persistence.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace rpg;
unsigned checks=0,camps=0,safe=0,danger=0;
std::vector<uint8_t> packed(const Game& g){std::vector<uint8_t> b(PERSISTENT_SAVE_SIZE);encodePersistent(g,1,0,b.data());return b;}
void check(Game& g){assert(persistentValid(g));auto b=packed(g);Game h;uint32_t seq;assert(decodePersistent(b.data(),0,h,seq)==Decode::Ok&&packed(h)==b);g=h;++checks;}
Game hero(unsigned cls,unsigned tier,unsigned seed){auto g=create(cls,seed);unsigned level=tier==1?5:tier==2?10:18;g.p.xp=dndXp[level-1];levelUp(g);g.tutorial=true;g.city=tier;g.eventStage=2;g.eventKind=2;g.eventTier=tier;g.eventLevel=level;g.eventDay=20733;g.eventOriginPage=9;g.rations=6;g.sleepKit=true;g.p.hp=1;g.p.mp=0;g.fatigue=71;check(g);return g;}
int main(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned tier=1;tier<4;++tier)for(unsigned seed=1;seed<=80;++seed)for(bool ration:{false,true})for(bool kit:{false,true}){
  auto g=hero(cls,tier,seed);auto initial=g;assert(escortCampAllowed(g)&&!startCamp(g,ration,kit));check(g);assert(g.rations==6-unsigned(ration)&&g.eventStage==2&&!g.eventProgress&&!valid(g));
  auto rolled=g;assert(startCamp(g,ration,kit)&&packed(g)==packed(rolled));assert(prepareDailyTrip(g,tier-1));assert(packed(g)==packed(rolled));
  assert(!finishEvent(g)&&!abandonEvent(g)&&!resumeEventBattle(g));assert(packed(g)==packed(rolled));assert(acceptCamp(g));check(g);
  if(g.campStage==3){++safe;assert(finishCamp(g));check(g);assert(!g.campStage&&g.eventStage==2&&!g.eventProgress&&g.city==tier&&g.p.gold==initial.p.gold);
    assert(g.p.hp==(ration?g.p.maxhp:std::min<unsigned>(g.p.maxhp,1+std::max(1,g.p.maxhp/2))));assert(g.fatigue==(ration?0:21));auto done=g;assert(!finishCamp(g)&&packed(g)==packed(done));
  }else {++danger;assert(g.campStage==2);for(auto outcome:{Phase::Won,Phase::Lost,Phase::Fled}){auto battle=g;
      if(outcome==Phase::Won){battle.enemyHp=0;finish(battle);}else if(outcome==Phase::Lost){battle.p.hp=0;finish(battle);}else battle.phase=Phase::Fled;
      check(battle);auto result=battle;assert(!finishEvent(battle)&&!resumeEventBattle(battle));assert(packed(battle)==packed(result));assert(resolveCamp(battle));check(battle);assert(battle.eventStage==2&&!battle.eventProgress&&battle.city==tier&&battle.p.gold==result.p.gold&&battle.p.xp==result.p.xp);
      if(outcome==Phase::Won){assert(battle.campStage==3&&finishCamp(battle));check(battle);}else assert(!battle.campStage&&battle.p.hp==(outcome==Phase::Lost?1:result.p.hp)&&battle.p.mp==result.p.mp&&battle.fatigue==result.fatigue);
      auto done=battle;assert(!resolveCamp(battle)&&packed(battle)==packed(done));assert(!dailyTripError(battle,tier-1));
    }}++camps;
 }
 // No broadened camping for other active missions, completed escort, wrong city or an active trip.
 for(unsigned kind=0;kind<8;++kind)if(kind!=2){auto g=hero(0,1,3);g.eventKind=kind;auto old=g;assert(startCamp(g,true,true)&&packed(g)==packed(old));}
 {auto g=hero(0,1,3);g.eventProgress=1;assert(startCamp(g,true,true));g.eventProgress=0;g.city=0;assert(startCamp(g,true,true));}
 {auto g=hero(0,1,3);assert(!prepareDailyTrip(g,0));auto old=g;assert(startCamp(g,true,true)&&packed(g)==packed(old));}
 printf("PASS escort recovery29: %u camps, %u safe/%u danger, %u Save29 snapshots; paid/rolled recovery, every combat outcome, mission retained, legacy frozen, other missions protected\n",camps,safe,danger,checks);
}
