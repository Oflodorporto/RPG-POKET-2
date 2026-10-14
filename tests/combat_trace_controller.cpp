#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main priorTitleControllerRegression
#include "title_controller.cpp"
#undef main
void traceFixture(unsigned cls=0,unsigned level=5,unsigned seed=42){
 nvs=Store{};backend.slot=0;assert(journal.load(game)==rpg::Load::Empty);game=rpg::create(cls,seed);game.p.xp=rpg::dndXp[level-1];rpg::levelUp(game);game.oath=cls==1;game.tutorial=true;game.city=1;game.p.life=game.p.mana=2;assert(rpg::begin(game,2));game.p.hp=std::max(1u,unsigned(game.p.maxhp)/2);assert(rpg::valid(game)&&journal.save(game));view=ViewState{};view.page=Page::Battle;combatFx=CombatFx{};menu.journey=Journey{};now=0;enemyAt=850;
}
void unchanged(const rpg::Game& g,unsigned seq){assert(!memcmp(&g,&game,sizeof(game)));assert(journal.seq==seq);}
void skip(){tapped(170,12);}
void details(){tapped(40,12);}
void back(){tapped(120,295);}
int main(){
 // The exact hardware admission helper accepts receipt controls, not action taps.
 traceFixture();for(auto phase:{rpg::Phase::Hero,rpg::Phase::Enemy}){game.phase=phase;for(bool effect:{false,true})for(int y=0;y<320;++y)for(int x=0;x<240;++x){bool expected=(combatTraceUi::detailsHit(x,y)||combatTraceUi::skipHit(x,y))||(!effect&&phase==rpg::Phase::Hero);assert(combatTraceUi::touchAllowed(view,game,effect,false,x,y)==expected);assert(!combatTraceUi::touchAllowed(view,game,effect,true,x,y));}}view.traceOpen=true;assert(combatTraceUi::touchAllowed(view,game,true,false,120,295));
 // Hero result is committed once, even when skip/details overlap auto-turn time.
 traceFixture();game.enemyHp=17;auto expected=game;assert(!rpg::act(expected,rpg::Action::Attack));action(rpg::Action::Attack);assert(!memcmp(&game,&expected,sizeof(game)));assert(view.heroTrace.valid);unsigned seq=journal.seq;auto resolved=game;
 now=20;skip();assert(!combatFx.active());for(unsigned i=0;i<20;++i){skip();details();back();tapped(40,250);tickEnemyTurn(now);}unchanged(resolved,seq);
 now=300;details();assert(view.traceOpen);auto receipt=view.heroTrace;
 for(unsigned i=0;i<20;++i){now+=200;tickEnemyTurn(now);tapped(185,259);tapped(44,259);tapped(170,50);tapped(50,50);}assert(!memcmp(&receipt,&view.heroTrace,sizeof(receipt)));unchanged(resolved,seq);
 unsigned paused=now-300;back();assert(!view.traceOpen);assert(enemyAt==850+paused);for(unsigned i=0;i<20;++i){back();skip();tapped(40,250);}unchanged(resolved,seq);
 now=enemyAt-1;tickEnemyTurn(now);unchanged(resolved,seq);now=enemyAt;auto enemyExpected=game;assert(rpg::enemy(enemyExpected));tickEnemyTurn(now);assert(!memcmp(&game,&enemyExpected,sizeof(game)));assert(journal.seq==seq+1&&view.enemyTrace.valid&&view.traceEnemy);resolved=game;seq=journal.seq;
 // Reading already-resolved enemy action pauses no damage: it is already applied.
 details();assert(view.traceOpen);for(unsigned i=0;i<20;++i){tickEnemyTurn(now);skip();}unchanged(resolved,seq);back();now+=200;skip();for(unsigned i=0;i<20;++i)skip();unchanged(resolved,seq);
 // Killing action still requires separate outcome acknowledgement.
 traceFixture();game.enemyHp=1;action(rpg::Action::MagicMissile);assert(game.phase==rpg::Phase::Won);resolved=game;seq=journal.seq;now=20;skip();assert(view.page==Page::Result);now=300;details();assert(view.traceOpen);now=600;back();for(unsigned i=0;i<20;++i){back();skip();tickEnemyTurn(now);}unchanged(resolved,seq);now=1000;tapped(120,285);assert(game.phase==rpg::Phase::Home);assert(journal.seq==seq+1);
 // A lethal enemy hit is committed before presentation, and detail close cannot revive/ack.
 bool lostChecked=false;for(unsigned seed=1;seed<50&&!lostChecked;++seed){traceFixture(0,5,seed);game.p.hp=1;game.phase=rpg::Phase::Enemy;assert(journal.save(game));auto lethal=game;assert(rpg::enemy(lethal));if(lethal.phase!=rpg::Phase::Lost)continue;now=1000;tickEnemyTurn(now);assert(!memcmp(&lethal,&game,sizeof(game)));resolved=game;seq=journal.seq;skip();assert(view.page==Page::Result);now+=200;details();assert(view.traceOpen&&view.enemyTrace.applied==1);now+=200;back();for(unsigned i=0;i<20;++i){back();skip();tickEnemyTurn(now);}unchanged(resolved,seq);lostChecked=true;}assert(lostChecked);
 // Both dungeon bosses retain the receipt after the killing animation.
 for(unsigned boss:{8u,24u}){traceFixture();game.phase=rpg::Phase::Home;game.dungeonFlags=boss==24?49:3;game.dungeonXY=0x27;game.dungeonLoot=boss==24?192:8;game.campaignFlags=boss==24?255:0;game.campaignEnding=boss==24?2:0;game.dungeonClears=boss==24?1:0;assert(rpg::begin(game,boss));game.enemyHp=1;assert(rpg::persistentValid(game));action(rpg::Action::MagicMissile);assert(game.phase==rpg::Phase::Won&&rpg::dungeonBossWon(game));resolved=game;seq=journal.seq;now=20;skip();assert(view.page==Page::DungeonVictory);now=300;details();assert(view.traceOpen&&view.heroTrace.valid);now=600;back();for(unsigned i=0;i<20;++i){back();skip();tickEnemyTurn(now);}unchanged(resolved,seq);}
 // Three misses retain individual hit tests and no imaginary damage dice.
 traceFixture(0,3,94);action(rpg::Action::ScorchingRay);assert(view.heroTrace.valid&&view.heroTrace.diceCount==0&&view.heroTrace.applied==0);unsigned tests=0;for(unsigned i=0;i<view.heroTrace.count;++i)tests+=view.heroTrace.steps[i].kind==rpg::TraceStep::RayHit;assert(tests==3);resolved=game;seq=journal.seq;now=20;details();for(unsigned i=0;i<20;++i){skip();tickEnemyTurn(now);}unchanged(resolved,seq);
 // Slot reload clears raw rolls; persisted state is unchanged and not rerolled.
 auto state=game;selectSlot(0);assert(!view.heroTrace.valid&&!view.enemyTrace.valid&&!view.traceOpen);assert(!memcmp(&state,&game,sizeof(game)));now=2000;details();assert(view.traceOpen);unchanged(state,seq);
 // Save failure retains one resolved receipt but offers no bypass of retry.
 traceFixture();nvs.fail=true;action(rpg::Action::MagicMissile);assert(view.page==Page::SaveError&&!combatFx.active());resolved=game;seq=journal.seq;for(unsigned i=0;i<20;++i){details();skip();tickEnemyTurn(now);}unchanged(resolved,seq);assert(!view.traceOpen);nvs.fail=false;tapped(120,285);assert(journal.seq==seq+1);assert(!memcmp(&resolved,&game,sizeof(game)));
 // UI clock wrapping cannot reroll, consume resources, or resume twice.
 traceFixture();now=UINT32_MAX-100;action(rpg::Action::Attack);resolved=game;seq=journal.seq;details();now=200;back();for(unsigned i=0;i<20;++i){back();skip();tickEnemyTurn(now);}unchanged(resolved,seq);
 printf("PASS: Save29 controller receipt/skip/detail/page/tab/back spam, committed enemy damage, miss/fatal/save failure/reload/wrap, no duplicate resolution or outcome acknowledgement\n");
}
