#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main historicalControllerMain
#include "title_controller.cpp"
#undef main

static void unchanged(const rpg::Game& expected,unsigned seq){
 assert(!memcmp(&game,&expected,sizeof(game)));assert(journal.seq==seq);
 assert(rpg::persistentValid(game));
}
int main(){
 unsigned cases=0;
 for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls)
 for(unsigned seed=1;seed<=8;++seed){
  nvs=Store{};backend.slot=slot;assert(journal.load(game)==rpg::Load::Empty);
  game=rpg::create(cls,seed);game.p.xp=rpg::dndXp[4];rpg::levelUp(game);
  game.tutorial=true;game.city=1;game.crystals=2;assert(journal.save(game));
  menu=MenuState{};menu.sessionStarted=true;menu.activeSlot=slot;
  combatFx=CombatFx{};view=ViewState{};view.page=Page::Ruins;now=100;enemyAt=0;
  const auto outside=game;const auto before=journal.seq;
  auto bag=scenicUi::ruinsButtons[scenicUi::ruinsBag];
  tapped(bag.x+bag.w/2,bag.y+bag.h/2);assert(view.page==Page::TownBag);
  unchanged(outside,before);tapped(panelUi::bagBack.x+5,panelUi::bagBack.y+5);
  assert(view.page==Page::Ruins);unchanged(outside,before);
  view.page=Page::DungeonEntry;tapped(180,290);
  assert(view.page==Page::Dungeon&&game.dungeonGenerator==1&&game.dungeonRun==1&&game.crystals==1);
  assert(game.randomState==outside.randomState&&rpg::persistentValid(game));
  const auto map=rpg::ruinsLayout(game.dungeonSeed);
  // Select a valid cell beside the actual generated first enemy as a fixture.
  auto spawn=rpg::dungeonSpawn(game,0);bool placed=false;
  for(unsigned d=0;d<4&&!placed;++d){int x=spawn.x-rpg::dungeonDx[d],y=spawn.y-rpg::dungeonDy[d];
   if(!rpg::dungeonWall(game,x,y)&&rpg::dungeonEnemyAt(game,x,y)<0){
    game.dungeonXY=x|(y<<4);rpg::dungeonFace(game,d);placed=true;}}
  assert(placed&&rpg::dungeonEnemyAhead(game)==0);
  assert(!rpg::dungeonMove(game,1,0,0)&&game.phase==rpg::Phase::Hero);
  assert(rpg::persistentValid(game)&&journal.save(game));
  auto expected=game;const auto selected=cls? rpg::Action::Attack:rpg::Action::MagicMissile;
  assert(!rpg::act(expected,selected));action(selected);
  assert(!memcmp(&game,&expected,sizeof(game))&&view.heroTrace.valid);
  auto resolved=game;auto seq=journal.seq;
  now+=20;tapped(40,12);assert(view.traceOpen);
  for(unsigned i=0;i<12;++i){now+=200;tickEnemyTurn(now);tapped(185,259);tapped(44,259);}
  unchanged(resolved,seq);tapped(120,295);assert(!view.traceOpen);
  for(unsigned i=0;i<12;++i){tapped(120,295);tapped(40,250);tickEnemyTurn(now);}
  unchanged(resolved,seq);now+=200;tapped(170,12);unchanged(resolved,seq);
  if(game.phase==rpg::Phase::Enemy){now=enemyAt;auto reply=game;assert(rpg::enemy(reply));
   tickEnemyTurn(now);assert(!memcmp(&game,&reply,sizeof(game))&&view.enemyTrace.valid);
   resolved=game;seq=journal.seq;now+=200;tapped(40,12);assert(view.traceOpen);
   for(unsigned i=0;i<12;++i){tickEnemyTurn(now);tapped(185,259);}
   unchanged(resolved,seq);tapped(120,295);now+=200;tapped(170,12);}
  resolved=game;seq=journal.seq;assert(rpg::persistentValid(game));
  selectSlot(slot);unchanged(resolved,seq);
  assert(!view.traceOpen&&!view.heroTrace.valid&&!view.enemyTrace.valid);
  assert(!memcmp(&map,&rpg::ruinsLayout(game.dungeonSeed),sizeof(map)));
  ++cases;
 }
 printf("PASS: %u integrated controller cases, 3 slots/4 classes/8 seeds; package5 Ruins bag, paid generated entry, real combat receipts, timer/input quarantine, exact Game/save/RNG/map preservation on reload\n",cases);
}
