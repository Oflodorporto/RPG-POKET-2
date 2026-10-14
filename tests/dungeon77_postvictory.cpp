#define main priorControllerRegression
#include "title_controller.cpp"
#undef main

static void encounter(bool modern,unsigned cls,unsigned seed,unsigned place=0){
 nvs=Store{};backend.slot=0;assert(journal.load(game)==rpg::Load::Empty);
 game=modern?rpg::create(cls,seed):rpg::testHero(cls,seed);game.tutorial=true;game.city=1;game.crystals=1;
 if(place==1){game.campaignFlags=255;game.campaignEnding=2;game.dungeonClears=1;assert(!rpg::enterIsland(game));}
 else if(place==2){assert(modern);game.city=2;assert(rpg::offerEvent(game,20733,10));game.eventKind=3;assert(!rpg::acceptEvent(game,27));assert(!rpg::eventAct(game));}
 else assert(!rpg::enterDungeon(game,0));
 if(place==2){game.dungeonXY=0x21;rpg::dungeonFace(game,0);}else game.dungeonXY=0x72;
 assert(rpg::dungeonEnemyAhead(game)==0);assert(rpg::begin(game,rpg::dungeonSpawn(game,0).id));
 game.enemyHp=1;game.p.life=73;game.p.mana=71;assert(rpg::valid(game)&&journal.save(game));
 view=ViewState{};view.page=Page::Dungeon;combatFx=CombatFx{};menu=MenuState{};now=100;
}
static void acknowledge(unsigned route){
 if(route==0)tapped(100,100); // Existing upper-scene workaround stays compatible.
 else if(route==1)tapped(120,234); // Newly visible Continue, not a movement request.
 else {tapped(195,300);assert(view.page==Page::DungeonMenu);tapped(120,105);}
}
static void same(const rpg::Game& expected){uint8_t a[rpg::SAVE_SIZE],b[rpg::SAVE_SIZE];rpg::encode(expected,17,a);rpg::encode(game,17,b);assert(!memcmp(a,b,sizeof(a)));assert(game.fatigue==expected.fatigue&&game.randomState==expected.randomState);}
int main(){
 unsigned wins=0;
 for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned seed=1;seed<=16;++seed)for(bool reboot:{false,true})for(unsigned route=0;route<3;++route)for(bool fail:{false,true}){
  encounter(modern,cls,seed);action(rpg::Action::Attack);if(game.phase!=rpg::Phase::Won)continue;++wins;
  auto won=game;auto saved=nvs.blobs;auto seq=journal.seq;assert(combatFx.active());
  for(int y:{100,218,234,258,300})for(int x:{40,120,198})tapped(x,y);
  same(won);assert(saved==nvs.blobs&&seq==journal.seq); // Animation still gates every tap.
  now+=combatFx.length();assert(combatFx.expire(now));view.page=currentPage();
  if(reboot){selectSlot(0);assert(view.page==Page::Dungeon&&game.phase==rpg::Phase::Won);same(won);}
  tapped(80,300);assert(view.page==Page::Bag);tapped(40,294);assert(view.page==Page::Dungeon);
  tapped(195,300);assert(view.page==Page::DungeonMenu);
  tapped(80,240);assert(view.page==Page::DungeonMenu); // Hidden exit cannot bypass result.
  tapped(80,290);assert(view.page==Page::Dungeon);same(won);assert(saved==nvs.blobs);
  auto expected=game;assert(rpg::dungeonResolve(expected));nvs.fail=fail;acknowledge(route);same(expected);
  if(fail){assert(view.page==Page::SaveError&&saved==nvs.blobs&&seq==journal.seq);nvs.fail=false;tapped(100,265);same(expected);}
  assert(view.page==Page::Dungeon&&game.phase==rpg::Phase::Home&&(game.dungeonEnemies&1)&&journal.seq==seq+1);
  assert(game.p.xp==won.p.xp&&game.p.gold==won.p.gold&&game.p.life==won.p.life&&game.p.mana==won.p.mana);
  saved=nvs.blobs;seq=journal.seq;acknowledgeDungeonResult();same(expected);assert(saved==nvs.blobs&&seq==journal.seq);
  selectSlot(0);same(expected);assert(view.page==Page::Dungeon);
  tapped(120,218);assert(rpg::dungeonX(game)==3&&rpg::dungeonY(game)==7&&game.phase==rpg::Phase::Home);
 }
 // Nonstandard dungeon contexts retain their existing resolution and save paths.
 for(unsigned place:{1u,2u})for(unsigned cls=0;cls<4;++cls)for(unsigned route=0;route<3;++route){
  encounter(place==2,cls,17,place);game.enemyHp=0;rpg::finish(game);assert(game.phase==rpg::Phase::Won&&journal.save(game));selectSlot(0);
  auto expected=game;if(place==2)assert(rpg::resumeEventBattle(expected));else assert(rpg::dungeonResolve(expected));
  acknowledge(route);same(expected);assert(view.page==Page::Dungeon&&(game.dungeonEnemies&1));
 }
 // Defeat/flee never auto-exit, grant victory, or use the Continue hitbox in combat.
 for(auto phase:{rpg::Phase::Lost,rpg::Phase::Fled})for(unsigned route=0;route<3;++route){
  encounter(false,0,42);game.phase=phase;if(phase==rpg::Phase::Lost)game.p.hp=0;assert(journal.save(game));selectSlot(0);
  auto expected=game;assert(rpg::dungeonResolve(expected));
  if(phase==rpg::Phase::Lost){
   auto lost=game;auto saved=nvs.blobs;auto seq=journal.seq;
   for(auto xy:{std::pair<int,int>{100,100},{120,234},{195,300}}){tapped(xy.first,xy.second);same(lost);assert(view.page==Page::Dungeon&&saved==nvs.blobs&&seq==journal.seq);}
   if(route==0)tapped(124,180);else if(route==1)tapped(225,219);else{tapped(14,180);assert(view.page==Page::DungeonMenu);tapped(120,105);}
  }else acknowledge(route);
  same(expected);assert(!(game.dungeonEnemies&1));assert(view.page==(phase==rpg::Phase::Lost?Page::Recovery:Page::Dungeon));
 }
 for(auto phase:{rpg::Phase::Hero,rpg::Phase::Enemy}){encounter(false,0,42);game.phase=phase;assert(journal.save(game));auto before=game;auto saved=nvs.blobs;tapped(120,234);same(before);assert(saved==nvs.blobs);}
 // Interrupted acknowledgement reloads the old Won checkpoint; retry never pays twice.
 encounter(false,0,42);game.enemyHp=0;rpg::finish(game);assert(journal.save(game));auto won=game;nvs.fail=true;acknowledge(1);assert(view.page==Page::SaveError);nvs.fail=false;selectSlot(0);same(won);assert(game.phase==rpg::Phase::Won);acknowledge(1);assert(game.p.gold==won.p.gold&&game.p.xp==won.p.xp&&(game.dungeonEnemies&1));
 assert(wins);printf("PASS: %u real lethal-action continuations; legacy/new four classes, scene/button/menu, reboot, animation gating, save failure/retry, exact Save28/fatigue/RNG/rewards; island/event, defeat/flee and resumed movement.\n",wins);
}
