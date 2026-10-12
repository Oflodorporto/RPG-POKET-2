#define main previousEnemyControllerRegression
#include "title_controller.cpp"
#undef main
static rpg::Game encounter(unsigned id,unsigned cls,unsigned seed,bool legacy=false){
 auto g=rpg::create(cls,seed);g.p.xp=rpg::dndXp[19];rpg::levelUp(g);g.tutorial=true;g.enemyId=id;g.ruinsWins=3;g.seenEnemies=1u<<id;
 if(id>=10&&id<14){g.eventStage=2;g.eventDay=20738;g.eventTier=id-10;g.city=id-10;g.eventOriginPage=uint8_t(Page::Explore);g.eventOriginCity=g.city;}
 if(id>=14&&id<18){g.discovery=4;g.discoverAmount=1;g.city=id-14;}
 g.enemyHp=rpg::encounterHp(g,id);g.phase=rpg::Phase::Enemy;g.fatigue=90;
 if(legacy){g.dndProgression=false;g.p.maxmp=rpg::totalMana(g);g.p.mp=g.p.maxmp;}
 assert(rpg::valid(g));return g;
}
int main(){
 unsigned cases=0,intents[5]={},skips=0,misses=0,zeros=0,damage=0;
 for(unsigned id=0;id<29;++id)for(unsigned cls=0;cls<4;++cls)for(unsigned beat=0;beat<3;++beat)for(unsigned seed=1;seed<=32;++seed)for(unsigned condition=0;condition<4;++condition){
  if(condition==3&&(cls!=1||!rpg::undead(id)))continue;
  nvs=Store{};journal.load(game);game=encounter(id,cls,seed);game.enemyBeat=beat;
  if(condition==1){game.guard=75;if(cls==3)game.rageTurns=3;}
  if(condition==2)game.enemyHp-=std::min<unsigned>(2,game.enemyHp-1);
  if(condition==3){game.oath=1;game.channelSpent=true;game.turnedTurns=1;}
  auto intent=rpg::enemyIntent(game);unsigned oldHp=game.enemyHp;auto expected=game;assert(rpg::enemy(expected));assert(journal.save(game));
  view=ViewState{};view.page=Page::Battle;combatFx=CombatFx{};now=50000;enemyAt=now+1;
  auto before=game;tickEnemyTurn(now);assert(!memcmp(&game,&before,sizeof(game))&&!combatFx.active());
  tickEnemyTurn(++now);assert(!memcmp(&game,&expected,sizeof(game))&&combatFx.active());
  auto result=combatFx.result;assert(result.enemyAction&&result.resolvedEnemyIntent==intent&&!result.enemyDefeated);
  assert(combatFx.length()==(intent==rpg::Intent::Mend?1200u:intent==rpg::Intent::Prepare?480u:id==5?960u:480u));
  ++intents[unsigned(intent)];++cases;skips+=result.kind==CombatFeedback::Skip;misses+=result.kind==CombatFeedback::Miss;zeros+=result.kind==CombatFeedback::NoDamage;damage+=result.injury();
  if(condition==3)assert(result.kind==CombatFeedback::Skip&&!result.attack());
  else if(intent==rpg::Intent::Mend)assert(result.kind==CombatFeedback::Heal&&result.amount==game.enemyHp-oldHp);
  else if(intent==rpg::Intent::Prepare)assert(result.kind==CombatFeedback::Buff&&result.amount==0);
  for(unsigned p:{0u,199u,200u,490u,491u,600u,650u,817u,818u,999u}){
   auto pose=enemyPose(true,result,p,combatFx.onHero);
   if(result.kind==CombatFeedback::Heal||result.kind==CombatFeedback::Skip)assert(pose==enemyArt::Pose::Idle);
   if(result.attack())assert(pose==(p<200?enemyArt::Pose::Anticipation:p<650?enemyArt::Pose::Attack:enemyArt::Pose::Idle));
   assert(pose!=enemyArt::Pose::Hurt);assert(enemyOpacity(true,result,p,false)==1000);
  }
  auto saved=nvs.blobs;for(unsigned i=0;i<8;++i){tapped(40,250);tickEnemyTurn(now+i);}assert(saved==nvs.blobs&&!memcmp(&game,&expected,sizeof(game)));
  rpg::Game recovered;rpg::FatigueJournal<NvsBackend> reboot(backend);assert(reboot.load(recovered)==rpg::Load::Ok);uint8_t a[rpg::SAVE_SIZE],b[rpg::SAVE_SIZE];rpg::encode(game,1,a);rpg::encode(recovered,1,b);assert(!memcmp(a,b,sizeof(a))&&recovered.fatigue==game.fatigue);
  auto length=combatFx.length();assert(!combatFx.expire(now+length-1));assert(combatFx.expire(now+length));tickEnemyTurn(now+length);assert(saved==nvs.blobs&&!memcmp(&game,&expected,sizeof(game)));
 }
 for(auto n:intents)assert(n);assert(skips&&misses&&zeros&&damage);
 // Hero injuries select hurt only on the actual enemy target. Lethal snapshots fade,
 // and previews/repeated rendering never change rules, RNG, saves or effect duration.
 unsigned lethal=0;
 for(unsigned id=0;id<29;++id)for(unsigned cls=0;cls<4;++cls)for(unsigned seed=1;seed<=32;++seed){
  nvs=Store{};journal.load(game);game=encounter(id,cls,seed);game.phase=rpg::Phase::Hero;game.enemyHp=1;assert(journal.save(game));
  auto expected=game;assert(!rpg::act(expected,rpg::Action::Attack));view=ViewState{};view.page=Page::Battle;combatFx=CombatFx{};action(rpg::Action::Attack);assert(!memcmp(&expected,&game,sizeof(game)));
  const auto result=combatFx.result;assert(result.enemyDefeated==(result.injury()&&game.enemyHp==0));lethal+=result.enemyDefeated;
  assert(enemyPose(true,result,600,false)==(result.injury()?enemyArt::Pose::Hurt:enemyArt::Pose::Idle));
  assert(enemyPose(true,result,600,true)!=enemyArt::Pose::Hurt);
  if(result.enemyDefeated){assert(enemyOpacity(true,result,650,true)==1000&&enemyOpacity(true,result,900,true)<1000&&enemyOpacity(true,result,1000,true)==0);}
  auto frozen=game;auto blobs=nvs.blobs;for(unsigned i=0;i<8;++i)tapped(120,280);assert(!memcmp(&frozen,&game,sizeof(game))&&blobs==nvs.blobs);
 }
 assert(lethal);
 // Failed write pauses presentation; a retry commits the same resolved state once.
 nvs=Store{};journal.load(game);game=encounter(28,1,123);assert(journal.save(game));auto expected=game;rpg::enemy(expected);nvs.fail=true;view=ViewState{};view.page=Page::Battle;combatFx=CombatFx{};enemyAt=now;tickEnemyTurn(now);assert(view.page==Page::SaveError&&!combatFx.active()&&!memcmp(&expected,&game,sizeof(game)));nvs.fail=false;tapped(120,269);assert(view.page!=Page::SaveError&&!combatFx.active()&&!memcmp(&expected,&game,sizeof(game)));
 for(unsigned id=0;id<29;++id){auto g=encounter(id,0,42,true);assert(rpg::enemyIntent(g)==rpg::Intent::Strike);}
 CombatFx wrap;wrap.start(Effect::Slash,true,UINT32_MAX-200);assert(!wrap.expire(278)&&wrap.expire(279));
 printf("PASS: %u all29/all5intent enemy dispatches, %u lethal hero cases; exact state/RNG/Save28/fatigue, captured intent, current clocks, repeated input, persistence/retry, legacy and wraparound.\n",cases,lethal);
}
