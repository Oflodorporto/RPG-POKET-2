#include "../firmware/RPG_POKET_2/Save.h"
#include <array>
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstring>

using namespace rpg;

using SaveBytes=std::array<uint8_t,SAVE_SIZE>;
static SaveBytes bytes(const Game& g,unsigned seq=17){SaveBytes b{};encode(g,seq,b.data());return b;}
static void unchanged(const Game& before,const Game& after){assert(before.fatigue==after.fatigue&&bytes(before)==bytes(after));}
static Game hero(unsigned cls,unsigned level,unsigned city=0){
  auto g=create(cls,123);g.p.xp=dndXp[level-1];levelUp(g);g.city=city;
  g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;return g;
}

static void boundsAndSave(){
  assert(Game{}.fatigue==0&&create(0,1).fatigue==0);
  for(unsigned value=0;value<256;++value){
    auto g=create(0,1);g.fatigue=value;
    assert(fatigueBand(g)==(value<40?0:value<70?1:value<90?2:3));
    assert(fatigueTravelPenalty(g)==(value>=90));
    assert(fatigueName(g)&&*fatigueName(g));
    for(unsigned amount:{0u,1u,4u,50u,100u,255u,UINT_MAX}){
      auto a=g;addFatigue(a,amount);unsigned old=std::min(value,100u);
      assert(a.fatigue==old+std::min(amount,100-old));
      auto b=g;recoverFatigue(b,amount);assert(b.fatigue==old-std::min(old,amount));
    }
  }
  // The RAM field has no offset or flag in Save28, even at every band boundary.
  static_assert(SAVE_SIZE==128,"Fatigue must not grow Save28");
  static_assert(LEGACY_SAVE_FORMAT==28,"Fatigue must not change Save28");
  for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race)
  for(unsigned level:{1u,5u,10u,18u,20u}){
    auto g=hero(cls,level);g.race=race;g.shirt=7;g.trousers=6;g.rations=9;
    assert(valid(g));auto original=bytes(g);
    for(unsigned fatigue=0;fatigue<=100;++fatigue){
      g.fatigue=fatigue;assert(valid(g)&&bytes(g)==original);
      Game loaded;loaded.fatigue=100;uint32_t seq=0;
      assert(decode(original.data(),loaded,seq)==Decode::Ok&&seq==17);
      assert(loaded.fatigue==0&&bytes(loaded)==original);
    }
  }
}

static void exploration(){
  bool combat=false,find=false,chest=false,meeting=false,mimic=false,locked=false,cargo=false;
  for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city)
  for(bool night:{false,true})for(unsigned seed=1;seed<=256;++seed){
    auto g=hero(cls,10,city);g.randomState=seed;g.fatigue=39;
    assert(startDiscovery(g,night)&&g.fatigue==43);
    auto accepted=g;assert(!startDiscovery(g,night));unchanged(accepted,g);
    assert(!explore(g));unchanged(accepted,g);
    combat|=g.phase==Phase::Hero;find|=g.discovery==1;
    chest|=g.discovery==2;meeting|=g.discovery==3;
    mimic|=g.discoverLoot==7;locked|=g.chestLock!=0;
    if(g.discovery){
      if(g.chestLock){g.gazuas=9;attemptLock(g,true);assert(g.fatigue==43);}
      collectDiscovery(g);assert(g.fatigue==43);
      clearDiscovery(g);assert(g.fatigue==43);
    }
  }
  assert(combat&&find&&chest&&meeting&&mimic&&locked);
  for(unsigned seed=1;seed<=128;++seed){
    auto g=hero(2,10);g.randomState=seed;g.questId=5;g.questLevel=10;g.fatigue=40;
    assert(startDiscovery(g)&&g.fatigue==44);
    if(g.discovery==6){cargo=true;assert(!collectDiscovery(g)&&g.fatigue==44&&g.questProgress==1);}
  }
  assert(cargo);
  for(unsigned level:{1u,5u,10u,18u,20u})for(unsigned cls=0;cls<4;++cls)
  for(unsigned city=0;city<4;++city){
    auto g=hero(cls,level,city);g.fatigue=98;assert(explore(g)&&g.fatigue==100);
    auto accepted=g;assert(!explore(g));unchanged(accepted,g);
    assert(act(g,Action(255)));unchanged(accepted,g);
    assert(!act(g,Action::Attack)&&g.fatigue==100);
    if(g.phase==Phase::Enemy)assert(enemy(g)&&g.fatigue==100);
  }
  auto boss=hero(2,5,1);boss.ruinsWins=3;boss.fatigue=89;
  assert(explore(boss,true)&&boss.enemyId==3&&boss.fatigue==93);
  auto accepted=boss;assert(!explore(boss,true));unchanged(accepted,boss);
  for(unsigned block=0;block<7;++block){
    auto g=hero(2,5,1);g.fatigue=50;
    if(block==0)g.phase=Phase::Hero;
    if(block==1)g.campStage=1;
    if(block==2)g.tripStage=1;
    if(block==3)g.dungeonFlags=1;
    if(block==4)g.discovery=1;
    if(block==5)g.p.hp=0;
    // The final case has too few wins for the direct boss entrance.
    auto before=g;assert(!explore(g,block==6));unchanged(before,g);
  }
  for(unsigned block=0;block<4;++block){
    auto g=hero(2,5);g.fatigue=50;
    if(block==0)g.campaignStage=1;
    if(block==1)g.eventStage=2;
    if(block>=2)g.clubStage=block-1;
    auto before=g;assert(!startDiscovery(g));unchanged(before,g);
  }
  // Twenty-five accepted explorations fill the scale, with no turn-based charge.
  auto journey=hero(2,20);
  for(unsigned n=1;n<=30;++n){
    assert(explore(journey)&&journey.fatigue==std::min(100u,n*4));
    journey.phase=Phase::Fled;assert(home(journey));
    assert(journey.fatigue==std::min(100u,n*4));
  }
}

static void dungeonMovement(){
  // Ordinary dungeon, island and letter expedition use the same move path.
  for(unsigned mode=0;mode<3;++mode){
    auto g=hero(2,18,1);g.fatigue=39;
    if(mode==0){g.crystals=1;assert(!enterDungeon(g,0));}
    else if(mode==1){g.campaignEnding=2;g.campaignFlags=127;assert(!enterIsland(g));}
    else {g.eventStage=2;g.eventKind=3;g.eventTier=1;g.eventLevel=18;assert(!eventAct(g));}
    assert(g.fatigue==39);
    auto entry=g;assert(!dungeonMove(g,0,0,0));unchanged(entry,g);
    for(unsigned turn=0;turn<4;++turn){auto xy=g.dungeonXY;assert(!dungeonMove(g,0,0,1)&&g.fatigue==39&&g.dungeonXY==xy);}
    dungeonFace(g,3);auto wall=g;assert(dungeonMove(g,1,0,0));unchanged(wall,g);
    dungeonFace(g,1);auto xy=g.dungeonXY;
    assert(!dungeonMove(g,1,0,0)&&g.dungeonXY!=xy&&g.fatigue==40);
    xy=g.dungeonXY;assert(!dungeonMove(g,-1,0,0)&&g.dungeonXY!=xy&&g.fatigue==41);
    // A side step counts once, without needing a heading change.
    dungeonFace(g,0);assert(!dungeonMove(g,0,1,0)&&g.fatigue==42);
    if(mode==2){dungeonFace(g,3);assert(!dungeonMove(g,1,0,0));dungeonFace(g,0);assert(!dungeonMove(g,1,0,0));}
    assert(dungeonEnemyAhead(g)==0||mode<2);
    if(mode<2)dungeonFace(g,1);
    assert(dungeonEnemyAhead(g)==0&&!dungeonWall(g,dungeonX(g),dungeonY(g)));
    auto fatigue=g.fatigue;xy=g.dungeonXY;
    assert(!dungeonMove(g,1,0,0)&&g.phase==Phase::Hero&&g.dungeonXY==xy&&g.fatigue==fatigue);
    auto combat=g;assert(dungeonMove(g,1,0,0));unchanged(combat,g);
    assert(!act(g,Action::Defensive)&&g.fatigue==fatigue);
    if(g.phase==Phase::Enemy)assert(enemy(g)&&g.fatigue==fatigue);
    g.enemyHp=0;finish(g);assert(dungeonResolve(g)&&g.fatigue==fatigue);
    assert(!dungeonMove(g,1,0,0)&&g.dungeonXY!=xy&&g.fatigue==fatigue+1);
    g.fatigue=100;assert(!dungeonMove(g,-1,0,0)&&g.fatigue==100);
    assert(leaveDungeon(g)&&g.fatigue==100);
    auto outside=g;assert(dungeonMove(g,1,0,0));unchanged(outside,g);
  }
}

static unsigned travelMatrix(){
  std::array<unsigned,20> seeds{};unsigned found=0;
  for(unsigned seed=1;found<20;++seed){auto g=create(0,seed);unsigned face=random(g)%20;if(!seeds[face]){seeds[face]=seed;++found;}}
  unsigned cases=0,maxAmbushIncrease=0;
  for(bool dnd:{false,true})for(unsigned cls=0;cls<4;++cls)
  for(unsigned level:{1u,5u,10u,18u,20u})for(unsigned wisdom:{8u,12u,20u})
  for(unsigned city=0;city<4;++city)for(unsigned dest=0;dest<4;++dest){
    if(city==dest)continue;
    for(unsigned inventory=0;inventory<8;++inventory)
    for(unsigned fatigue:{0u,39u,40u,69u,70u,89u,90u,100u}){
      auto base=hero(cls,level,city);base.dndProgression=dnd;base.attributes[4]=wisdom;
      base.p.mp=base.p.maxmp=totalMana(base);
      base.rations=inventory&1?2:0;base.charts=inventory&2?2:0;base.charms=inventory&4?2:0;
      base.fatigue=fatigue;assert(valid(base));
      // Independent 7.5 baseline: no fatigue in survival, luck or the supplies.
      unsigned oldSurvival=survival(base)+(base.rations?2:0)+(base.charts?1:0);
      unsigned oldLuck=luck(base)+(base.charts?1:0)+(base.charms?2:0);
      unsigned oldSafe=0,newSafe=0;
      auto untouched=base;unsigned preview=tripSafety(base,dest);unchanged(untouched,base);
      for(unsigned die=1;die<=20;++die){
        auto trip=base;trip.randomState=seeds[die-1];assert(!prepareTrip(trip,dest));
        assert(trip.tripRoll==die&&trip.fatigue==fatigue);
        assert(trip.tripSurvival==oldSurvival-(fatigue>=90)&&trip.tripSurvival<=13);
        assert(trip.tripLuck==oldLuck&&survival(trip)==survival(base));
        assert(trip.rations==base.rations-(base.rations!=0));
        assert(trip.charts==base.charts-(base.charts!=0));
        assert(trip.charms==base.charms-(base.charms!=0));
        assert(trip.tripTotal==die+trip.tripSurvival+trip.tripLuck);
        bool oldResult=die==20||(die!=1&&die+oldSurvival+oldLuck>=routeDifficulty(city,dest));
        oldSafe+=oldResult;newSafe+=tripSafe(trip);
        if(die==1)assert(!tripSafe(trip));
        if(die==20)assert(tripSafe(trip));
        auto prepared=trip;assert(prepareTrip(trip,dest));unchanged(prepared,trip);
        // Even a later fatigue change cannot reinterpret a persisted trip roll.
        bool result=tripSafe(trip);trip.fatigue=fatigue?0:100;assert(tripSafe(trip)==result);
      }
      assert(preview==newSafe*5&&oldSafe>=newSafe);
      unsigned increase=(oldSafe-newSafe)*5;assert(increase<=5);
      if(fatigue<90)assert(increase==0);
      maxAmbushIncrease=std::max(maxAmbushIncrease,increase);++cases;
    }
  }
  assert(maxAmbushIncrease==5);
  // Lowest unprovisioned wizard reaches zero without unsigned underflow.
  auto low=hero(0,1);low.dndProgression=false;low.p.mp=low.p.maxmp=totalMana(low);
  low.rations=low.charts=low.charms=0;low.fatigue=100;
  assert(tripSurvivalBonus(low)==0&&!prepareTrip(low,1)&&low.tripSurvival==0&&valid(low));
  auto saved=bytes(low);Game loaded;uint32_t seq;
  assert(decode(saved.data(),loaded,seq)==Decode::Ok&&tripSafe(loaded)==tripSafe(low)&&bytes(loaded)==saved);
  for(unsigned dest:{0u,4u,255u}){auto g=hero(2,5);g.fatigue=90;auto before=g;assert(prepareTrip(g,dest));unchanged(before,g);}
  return cases;
}

static void campAndRest(){
  for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city)
  for(unsigned fatigue:{0u,1u,39u,40u,49u,50u,51u,69u,70u,89u,90u,100u})
  for(bool food:{false,true})for(bool kit:{false,true})for(unsigned die:{1u,20u}){
    auto g=hero(cls,10,city);g.fatigue=fatigue;g.rations=2;g.sleepKit=kit;g.p.hp=1;g.p.mp=0;
    auto noFatigue=g;noFatigue.fatigue=0;
    assert(!startCamp(g,food,kit)&&g.rations==2-unsigned(food)&&g.fatigue==fatigue);
    assert(!startCamp(noFatigue,food,kit));
    g.campRoll=noFatigue.campRoll=die;
    assert(campTotal(g)==campTotal(noFatigue)&&campSafe(g)==campSafe(noFatigue));
    auto started=g;assert(startCamp(g,food,kit));unchanged(started,g);
    assert(!finishCamp(g));unchanged(started,g);
    assert(acceptCamp(g)&&g.fatigue==fatigue);
    auto accepted=g;assert(!acceptCamp(g));unchanged(accepted,g);
    if(die==1){assert(g.campStage==2);g.enemyHp=0;finish(g);assert(resolveCamp(g)&&g.campStage==3&&g.fatigue==fatigue);}
    auto hp=g.p.hp,mp=g.p.mp,maxhp=g.p.maxhp,maxmp=g.p.maxmp;
    assert(finishCamp(g)&&g.fatigue==(food?0:fatigue-std::min(50u,fatigue)));
    assert(g.p.hp==(food?maxhp:std::min<unsigned>(maxhp,hp+std::max(1,maxhp/2))));
    assert(g.p.mp==(food?maxmp:std::min<unsigned>(maxmp,mp+std::max(1,maxmp/2))));
    assert(g.rations==2-unsigned(food)&&g.sleepKit==kit&&!g.campStage&&!powersSpent(g));
    auto completed=g;assert(!finishCamp(g));unchanged(completed,g);
  }
  for(bool food:{false,true})for(auto phase:{Phase::Lost,Phase::Fled}){
    auto g=hero(2,10);g.fatigue=90;g.rations=1;g.p.hp=1;
    assert(!startCamp(g,food,false));g.campRoll=1;assert(acceptCamp(g));
    if(phase==Phase::Lost){g.p.hp=0;finish(g);}else g.phase=phase;
    auto mp=g.p.mp;assert(resolveCamp(g)&&g.fatigue==90&&g.p.hp==1&&g.p.mp==mp&&g.rations==1-unsigned(food));
    auto failed=g;assert(!finishCamp(g)&&!resolveCamp(g));unchanged(failed,g);
  }
  // Fatigue alone enables both kinds of camp at full HP/MP with unused powers.
  for(bool food:{false,true}){
    auto g=hero(2,10);g.fatigue=1;g.rations=1;
    assert(!startCamp(g,food,false));g.campRoll=20;
    assert(acceptCamp(g)&&finishCamp(g)&&!g.fatigue);
    auto rested=g;assert(startCamp(g,false,false));unchanged(rested,g);
  }
  auto g=hero(2,10);g.fatigue=90;g.rations=0;
  auto before=g;assert(startCamp(g,true,false));unchanged(before,g);
  assert(startCamp(g,false,true));unchanged(before,g);
  g.eventStage=2;before=g;assert(startCamp(g,false,false));unchanged(before,g);
  g.eventStage=0;g.phase=Phase::Hero;before=g;assert(!rest(g));unchanged(before,g);
  g.phase=Phase::Home;g.p.hp=1;g.p.mp=0;g.windSpent=1;
  auto gold=g.p.gold;auto ration=g.rations;
  assert(rest(g)&&!g.fatigue&&g.p.hp==g.p.maxhp&&g.p.mp==g.p.maxmp&&!powersSpent(g));
  assert(g.p.gold==gold&&g.rations==ration);
  before=g;assert(rest(g));unchanged(before,g);
}

static void unaffectedRules(){
  // The approved package7 penalty begins strictly above 70%. Keep the full
  // prior combat oracle at that inclusive safe boundary; fatigue7.cpp covers
  // the intentional 71+ damage-only delta, plus high-fatigue powers and healing.
  for(unsigned cls=0;cls<4;++cls)for(unsigned level:{1u,5u,10u,18u,20u})
  for(unsigned city=0;city<4;++city)for(unsigned seed=1;seed<=16;++seed)
  for(unsigned action=0;action<=unsigned(Action::DivineSmite);++action){
    auto fresh=hero(cls,level,city);fresh.randomState=seed;
    fresh.p.hp=std::max(1u,unsigned(fresh.p.maxhp)/2);fresh.p.mp=fresh.p.maxmp/2;
    auto tired=fresh;tired.fatigue=70;
    assert(effectiveAttack(fresh)==effectiveAttack(tired)&&effectiveDefense(fresh)==effectiveDefense(tired));
    assert(totalMana(fresh)==totalMana(tired)&&survival(fresh)==survival(tired));
    assert(begin(fresh,7)&&begin(tired,7)&&bytes(fresh)==bytes(tired));
    assert((act(fresh,Action(action))==nullptr)==(act(tired,Action(action))==nullptr));
    assert(bytes(fresh)==bytes(tired)&&fresh.fatigue==0&&tired.fatigue==70);
    assert(enemy(fresh)==enemy(tired)&&bytes(fresh)==bytes(tired));
    assert(fresh.fatigue==0&&tired.fatigue==70);
  }
  for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city)
  for(unsigned seed=1;seed<=40;++seed)for(bool pick:{false,true}){
    auto fresh=hero(cls,10,city);fresh.randomState=seed;fresh.discovery=2;fresh.chestLock=1;
    fresh.chestTrap=true;fresh.discoverAmount=10;fresh.gazuas=2;
    auto tired=fresh;tired.fatigue=100;
    assert(lockChance(fresh,pick)==lockChance(tired,pick));
    assert(!attemptLock(fresh,pick)&&!attemptLock(tired,pick)&&bytes(fresh)==bytes(tired));
    assert(fresh.fatigue==0&&tired.fatigue==100);
  }
  for(unsigned cls=0;cls<4;++cls)for(int result:{-1,0,1}){
    auto fresh=hero(cls,10);fresh.guildMember=true;fresh.p.gold=100;
    auto tired=fresh;tired.fatigue=100;
    assert(!reserveFight(fresh,123)&&!reserveFight(tired,123));
    assert(startFight(fresh,123)&&startFight(tired,123));
    assert(settleFight(fresh,123,result)&&settleFight(tired,123,result));
    assert(bytes(fresh)==bytes(tired)&&fresh.fatigue==0&&tired.fatigue==100);
  }
}

struct RetryStore {
  SaveBytes saved[2]{};bool present[2]={},failWrite=false,failRead=false;
  Read read(const char* key,uint8_t* out){if(failRead)return Read::Error;unsigned i=key[0]=='b';if(!present[i])return Read::Missing;memcpy(out,saved[i].data(),SAVE_SIZE);return Read::Ok;}
  bool write(const char* key,const uint8_t* in){if(failWrite)return false;unsigned i=key[0]=='b';memcpy(saved[i].data(),in,SAVE_SIZE);present[i]=true;return true;}
};
static void retryIsPersistenceOnly(){
  RetryStore io;Journal<RetryStore> journal(io);Game loaded;assert(journal.load(loaded)==Load::Empty);
  auto g=hero(2,10);g.fatigue=89;assert(startDiscovery(g)&&g.fatigue==93);
  auto accepted=g;io.failWrite=true;assert(!journal.save(g));unchanged(accepted,g);
  io.failWrite=false;io.failRead=true;assert(!journal.save(g));unchanged(accepted,g);
  io.failRead=false;assert(journal.save(g));unchanged(accepted,g);
  assert(!startDiscovery(g));unchanged(accepted,g);
  // Repeated persistence after completing rest cannot consume another ration or heal twice.
  g=hero(2,10);g.fatigue=99;g.rations=2;
  assert(!startCamp(g,true,false)&&g.rations==1);g.campRoll=20;
  accepted=g;io.failWrite=true;assert(!journal.save(g));unchanged(accepted,g);
  io.failWrite=false;assert(journal.save(g));unchanged(accepted,g);
  assert(acceptCamp(g)&&finishCamp(g)&&!g.fatigue&&g.rations==1);
  auto completed=g;io.failWrite=true;assert(!journal.save(g));unchanged(completed,g);
  io.failWrite=false;assert(journal.save(g));unchanged(completed,g);
  assert(!finishCamp(g));unchanged(completed,g);
}

int main(){
  boundsAndSave();exploration();dungeonMovement();unsigned cases=travelMatrix();campAndRest();unaffectedRules();retryIsPersistenceOnly();
  printf("PASS: fatigue76, %u travel scenarios / %u D20 outcomes; maximum ambush increase 5 percentage points, bands/bounds, once-only exploration/movement/rest, unchanged Save28 and persistence-only retries.\n",cases,cases*20);
}
