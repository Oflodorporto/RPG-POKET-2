#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main priorFatigue7Controller
#include "title_controller.cpp"
#undef main
#include "Fatigue7Trace.h"
static void traceFixture(unsigned cls,unsigned level,unsigned seed){
 nvs=Store{};backend.slot=0;assert(journal.load(game)==rpg::Load::Empty);game=rpg::create(cls,seed);game.p.xp=rpg::dndXp[level-1];rpg::levelUp(game);game.oath=cls==1;game.tutorial=true;game.city=1;game.p.life=game.p.mana=2;assert(rpg::begin(game,2));game.p.hp=std::max(1u,unsigned(game.p.maxhp)/2);assert(rpg::valid(game)&&journal.save(game));view=ViewState{};view.page=Page::Battle;combatFx=CombatFx{};menu.journey=Journey{};now=0;enemyAt=850;
}
static void unchanged(const rpg::Game& g,unsigned seq){assert(!memcmp(&g,&game,sizeof(game)));assert(journal.seq==seq);}
static void skip(){tapped(170,12);}
static void details(){tapped(40,12);}
static void back(){tapped(120,295);}
int main(){unsigned checked=0;
 for(unsigned cls=0;cls<4;++cls)for(unsigned fatigue:{69u,70u,71u,100u}){
  traceFixture(cls,5,1);game.fatigue=fatigue;assert(journal.save(game));
  auto expected=game;auto a=cls?rpg::Action::Attack:rpg::Action::BurningHands;
  rpg::CombatTrace receipt;assert(!rpg::act(expected,a,&receipt));nvs.fail=true;action(a);
  assert(view.page==Page::SaveError&&!combatFx.active());assert(!memcmp(&expected,&game,sizeof(game)));
  assert(sameFatigue7Trace(receipt,view.heroTrace));auto resolved=game;auto seq=journal.seq;
  for(unsigned i=0;i<20;++i){details();skip();tickEnemyTurn(now);}unchanged(resolved,seq);
  nvs.fail=false;tapped(120,285);assert(journal.seq==seq+1);assert(!memcmp(&resolved,&game,sizeof(game)));
  seq=journal.seq;now=20;details();assert(view.traceOpen);
  for(unsigned i=0;i<20;++i){now+=100;tickEnemyTurn(now);tapped(185,259);tapped(44,259);}unchanged(resolved,seq);
  assert(sameFatigue7Trace(receipt,view.heroTrace));back();now+=200;skip();unchanged(resolved,seq);
  selectSlot(0);assert(!view.heroTrace.valid&&!view.traceOpen);unchanged(resolved,seq);
  assert(rpg::fatigueCombatPenalty(game)==(fatigue>70?(fatigue>=90?2:1):0));++checked;
 }
 printf("PASS fatigue7 controller: %u cases, all classes and 69/70/71/100; save-failure retry, repeated details/skip/timers, exact Game/receipt/RNG, reload without duplicate damage\n",checked);
 return 0;
}
