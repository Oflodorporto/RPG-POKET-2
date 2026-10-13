#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main unusedD20TitleMain
#include "title_controller.cpp"
#undef main

// Exercise the real controller snapshot with naturally generated results. The
// animation must reveal an already saved roll, never generate another one.
namespace {
enum class RollKind { Departure, Night, Camp };
unsigned cases=0, earlyTaps=0, heldTaps=0, reloads=0, failures=0;
unsigned coverage[3][21]{};

std::vector<uint8_t> packed(const rpg::Game& g) {
  std::vector<uint8_t> result(rpg::PERSISTENT_SAVE_SIZE);
  rpg::encodePersistent(g, 1, 0, result.data());
  return result;
}
void sameGame(const rpg::Game& expected) {
  assert(game.randomState==expected.randomState);
  assert(packed(game)==packed(expected));
  assert(rpg::persistentValid(game));
}
unsigned face(RollKind kind,const rpg::Game& g) {
  return kind==RollKind::Departure?g.tripRoll:
         kind==RollKind::Night?g.daily.roll:g.campRoll;
}
Page rollPage(RollKind kind) {
  return kind==RollKind::Departure?Page::TravelRoll:Page::CampRoll;
}
bool accept(RollKind kind,rpg::Game& g) {
  return kind==RollKind::Departure?rpg::acceptDailyRoad(g):
         kind==RollKind::Night?rpg::acceptDailyNight(g):rpg::acceptCamp(g);
}
bool safe(RollKind kind,const rpg::Game& g) {
  return kind==RollKind::Departure?rpg::tripSafe(g):
         kind==RollKind::Night?rpg::dailyNightSafe(g):rpg::campSafe(g);
}
unsigned destination(unsigned city) { return city==3?2:city+1; }

struct Fixture { rpg::Game before,rolled; };
Fixture naturalFixture(RollKind kind,unsigned result,unsigned cls,unsigned city,
                       bool ration,bool kit) {
  for(unsigned seed=1;seed<10000;++seed) {
    auto g=rpg::create(cls,seed);
    g.tutorial=true;g.originStory=true;g.originPage=8;
    g.city=city;g.rations=9;g.charts=g.charms=2;g.sleepKit=true;
    g.p.hp=1;g.p.mp=0;g.fatigue=75;
    if(kind==RollKind::Night) {
      assert(!rpg::prepareDailyTrip(g,destination(city),kit));
      assert(rpg::acceptDailyRoad(g));
      if(g.daily.phase==rpg::DailyPhase::RoadBattle) {
        g.enemyHp=0;rpg::finish(g);
        assert(g.phase==rpg::Phase::Won&&rpg::resolveDailyBattle(g));
      }
      assert(g.daily.phase==rpg::DailyPhase::Leg);
    }
    Fixture fixture{g,g};
    if(kind==RollKind::Departure)
      assert(!rpg::prepareDailyTrip(fixture.rolled,destination(city)));
    else if(kind==RollKind::Night)
      assert(rpg::finishDailyLeg(fixture.rolled));
    else
      assert(!rpg::startCamp(fixture.rolled,ration,kit));
    if(face(kind,fixture.rolled)!=result)continue;
    assert(rpg::persistentValid(fixture.before));
    assert(rpg::persistentValid(fixture.rolled));
    return fixture;
  }
  assert(false&&"Every face must be reached by the real RNG");
  return {};
}

void reset(const Fixture& fixture,unsigned slot,uint32_t start) {
  nvs=Store{};backend.slot=slot;
  assert(journal.load(game)==rpg::Load::Empty);
  game=fixture.before;assert(journal.save(game));
  view=ViewState{};menu=MenuState{};combatFx=CombatFx{};
  now=start;selectSlot(slot);sameGame(fixture.before);
}
void retrySave(const rpg::Game& expected,Page target) {
  assert(view.page==Page::SaveError);sameGame(expected);
  const auto durable=nvs.blobs;
  // Timer ticks and unsuccessful save retries cannot accept or reroll it.
  for(unsigned i=0;i<3;++i) {
    now+=10000;tickTravelProgress(now);tapped(120,269);
    assert(view.page==Page::SaveError&&nvs.blobs==durable);sameGame(expected);
  }
  nvs.fail=false;tapped(120,269);
  assert(view.page==target);sameGame(expected);++failures;
}
void reload(const rpg::Game& expected,Page expectedPage) {
  const auto durable=nvs.blobs;
  now+=701;selectSlot(backend.slot);
  assert(view.page==expectedPage&&nvs.blobs==durable);
  sameGame(expected);++reloads;
}
void rejectEarly(RollKind kind,const rpg::Game& expected) {
  const auto durable=nvs.blobs;
  const uint32_t started=menu.rollStarted;
  assert(!menu.rollReady&&view.page==rollPage(kind));
  for(unsigned elapsed:{0u,1u,119u,120u,600u,1200u,1680u,1798u,1799u}) {
    now=started+elapsed;tickTravelProgress(now);
    assert(!menu.rollReady&&view.page==rollPage(kind));
    for(unsigned i=0;i<3;++i) {
      tapped(175,290);++earlyTaps;
      assert(!menu.rollReady&&view.page==rollPage(kind)&&nvs.blobs==durable);
      sameGame(expected);
    }
  }
}
void makeReady(RollKind kind,const rpg::Game& expected) {
  const auto durable=nvs.blobs;
  pendingTouch=true;gate.armed=true;
  now=menu.rollStarted+1800;tickTravelProgress(now);
  assert(menu.rollReady&&!pendingTouch&&!gate.armed);
  assert(view.page==rollPage(kind)&&nvs.blobs==durable);sameGame(expected);
  // A held touch across the exact reveal boundary is deliberately disarmed.
  for(unsigned i=0;i<8;++i) {
    assert(!gate.update(true,true,now+i));++heldTaps;
  }
  assert(face(kind,game)>=1&&face(kind,game)<=20);
}
void caseFor(RollKind kind,unsigned result,unsigned cls,unsigned city,unsigned slot) {
  const bool ration=(slot+result)%2,kit=(slot+result/2)%2;
  const auto fixture=naturalFixture(kind,result,cls,city,ration,kit);
  const bool startFails=cls&1,acceptFails=cls&2;
  // Half the matrix crosses the 32-bit millis wrap during the roll gate.
  const uint32_t started=(city+slot)%2?UINT32_MAX-1000u:10000u;
  reset(fixture,slot,kind==RollKind::Night?started-Journey::duration:started);
  if(kind==RollKind::Departure) {
    view.page=Page::TravelConfirm;view.tripDestination=destination(city);
    nvs.fail=startFails;tapped(175,290);
  } else if(kind==RollKind::Night) {
    assert(view.page==Page::Travel&&menu.journey.active);
    nvs.fail=startFails;now+=Journey::duration;tickTravelProgress(now);
  } else {
    openCamp();assert(view.page==Page::CampSetup);
    menu.campRation=ration;menu.campKit=kit;
    nvs.fail=startFails;tapped(175,290);
  }
  if(startFails)retrySave(fixture.rolled,rollPage(kind));
  assert(view.page==rollPage(kind)&&!menu.rollReady);
  sameGame(fixture.rolled);assert(face(kind,game)==result);
  // The first saved checkpoint contains both the face and exact post-roll RNG.
  rpg::Game durableGame;NvsJournal freshJournal(backend);
  assert(freshJournal.load(durableGame)==rpg::Load::Ok);
  assert(packed(durableGame)==packed(fixture.rolled));
  rejectEarly(kind,fixture.rolled);
  reload(fixture.rolled,rollPage(kind)); // restart during animation
  rejectEarly(kind,fixture.rolled);makeReady(kind,fixture.rolled);
  auto durable=nvs.blobs;
  now+=120000;tickTravelProgress(now); // a revealed roll never auto-accepts
  assert(menu.rollReady&&nvs.blobs==durable&&view.page==rollPage(kind));
  sameGame(fixture.rolled);
  reload(fixture.rolled,rollPage(kind)); // restart after reveal
  assert(!menu.rollReady);rejectEarly(kind,fixture.rolled);
  makeReady(kind,fixture.rolled);

  auto accepted=fixture.rolled;assert(accept(kind,accepted));
  const bool isSafe=safe(kind,fixture.rolled);
  const Page next=isSafe?(kind==RollKind::Departure?Page::Travel:Page::CampRest):Page::Battle;
  // A fresh release and press is required. Expected game/RNG comes from exactly
  // one rules transition; unsafe encounters may legitimately consume RNG here.
  assert(!gate.update(true,false,now+1));
  assert(!gate.update(true,false,now+31));
  assert(gate.update(true,true,now+32));now+=32;
  nvs.fail=acceptFails;tapped(175,290);
  if(acceptFails)retrySave(accepted,next);
  assert(view.page==next);sameGame(accepted);
  assert(face(kind,game)==result);
  durable=nvs.blobs;
  for(unsigned i=0;i<16;++i) {
    if(gate.update(true,true,now+i))tapped(175,290);
    ++heldTaps;assert(view.page==next&&nvs.blobs==durable);sameGame(accepted);
  }
  auto repeat=accepted;assert(!accept(kind,repeat));
  assert(packed(repeat)==packed(accepted));
  if(isSafe)assert(accepted.randomState==fixture.rolled.randomState);
  reload(accepted,next);
  assert(face(kind,game)==result);
  ++coverage[unsigned(kind)][result];++cases;
}
}

int main() {
  for(auto kind:{RollKind::Departure,RollKind::Night,RollKind::Camp})
    for(unsigned result=1;result<=20;++result)
      for(unsigned cls=0;cls<4;++cls)
        for(unsigned city=0;city<4;++city)
          for(unsigned slot=0;slot<3;++slot)
            caseFor(kind,result,cls,city,slot);
  for(unsigned kind=0;kind<3;++kind)
    for(unsigned result=1;result<=20;++result)assert(coverage[kind][result]==48);
  printf("PASS D20 controller: %u natural-face cases (1..20, departure/night/camp, "
         "4 classes, 4 cities, 3 slots); %u early taps, %u held taps, %u reloads, "
         "%u save failures; exact 1800ms gate, millis wrap, persisted result/RNG, "
         "single acceptance and no reroll.\n",cases,earlyTaps,heldTaps,reloads,failures);
}
