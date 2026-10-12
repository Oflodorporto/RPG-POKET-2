#define main previousControllerRegression
#include "title_controller.cpp"
#undef main
int main(){
 unsigned passed=0,heals=0,buffs=0,guards=0,hits=0,misses=0;
 const rpg::Action actions[]={rpg::Action::Attack,rpg::Action::Offensive,rpg::Action::Defensive,rpg::Action::DivineSmite,rpg::Action::LayHands,rpg::Action::SecondWind,rpg::Action::ActionSurge,rpg::Action::RagePower,rpg::Action::MagicMissile,rpg::Action::BurningHands,rpg::Action::ScorchingRay,rpg::Action::Fireball,rpg::Action::SacredWeapon,rpg::Action::TurnUndead};
 for(unsigned cls=0;cls<4;++cls)for(unsigned seed=1;seed<=64;++seed)for(auto a:actions){
  journal.load(game);game=rpg::create(cls,seed);game.p.xp=rpg::dndXp[19];rpg::levelUp(game);game.tutorial=true;game.p.hp=game.p.maxhp/2;game.p.mp=game.p.maxmp;game.oath=cls==1;assert(rpg::begin(game,2));game.fatigue=89;assert(journal.save(game));
  auto expected=game;auto normalized=a;if(cls==0&&a==rpg::Action::Offensive)normalized=rpg::Action::MagicMissile;if(cls==0&&a==rpg::Action::Defensive)normalized=rpg::Action::ShieldSpell;
  const char* err=rpg::act(expected,normalized);if(err)continue;
  combatFx=CombatFx{};view=ViewState{};view.page=Page::Battle;action(a);
  assert(!memcmp(&game,&expected,sizeof(game)));++passed;
  if(combatFx.active()){
   const auto& r=combatFx.result;assert(!r.enemyAction);
   if(r.kind==CombatFeedback::Damage||r.kind==CombatFeedback::Heal)assert(r.amount==game.damage);else assert(r.amount==0);
   if(r.kind==CombatFeedback::Damage)assert(r.critical==game.crit);assert(r.injury()==(r.kind==CombatFeedback::Damage&&r.amount>0));
   heals+=r.kind==CombatFeedback::Heal;buffs+=r.kind==CombatFeedback::Buff;guards+=r.kind==CombatFeedback::Guard;hits+=r.kind==CombatFeedback::Damage;misses+=r.kind==CombatFeedback::Miss;
   auto saved=nvs.blobs;auto before=game;for(unsigned i=0;i<8;++i)tapped(40,250);assert(!memcmp(&before,&game,sizeof(game))&&saved==nvs.blobs);
   auto length=combatFx.length();assert(!combatFx.expire(now+length-1));assert(combatFx.expire(now+length));
  }
 }
 assert(passed&&heals&&buffs&&guards&&hits&&misses);

 unsigned enemyCases=0,mends=0,prepares=0,enemyHits=0,enemyMisses=0,zeroes=0,skips=0;
 for(unsigned id:{0u,1u,2u,5u})for(unsigned beat=0;beat<3;++beat)for(unsigned seed=1;seed<=32;++seed)for(unsigned condition=0;condition<5;++condition){
  if(condition==3&&!rpg::undead(id))continue;
  game=rpg::create(condition==3?1:3,seed);game.p.xp=rpg::dndXp[19];rpg::levelUp(game);assert(rpg::begin(game,id));game.phase=rpg::Phase::Enemy;game.enemyBeat=beat;game.fatigue=90;
  if(condition==1){game.guard=75;game.rageTurns=3;}
  if(condition==2)game.enemyHp-=std::min<unsigned>(2,game.enemyHp-1);
  if(condition==3){game.oath=1;game.channelSpent=true;game.turnedTurns=1;++skips;}
  if(condition==4)game.enemyHp-=std::min<unsigned>(5,game.enemyHp-1);
  unsigned hpBefore=game.enemyHp;auto intent=rpg::enemyIntent(game);auto expected=game;assert(rpg::enemy(expected));assert(journal.save(game));
  combatFx=CombatFx{};view=ViewState{};view.page=Page::Battle;now=100000;enemyAt=now+1;
  auto unchanged=game;tickEnemyTurn(now);assert(!memcmp(&game,&unchanged,sizeof(game))&&!combatFx.active());
  ++now;tickEnemyTurn(now);assert(!memcmp(&game,&expected,sizeof(game))&&combatFx.active());++enemyCases;
  auto r=combatFx.result;assert(r.enemyAction);assert(combatFx.onHero==(condition!=3&&intent!=rpg::Intent::Mend&&intent!=rpg::Intent::Prepare));
  if(condition==3){assert(r.kind==CombatFeedback::Skip&&r.amount==0&&!r.attack()&&r.noHit());}
  else if(intent==rpg::Intent::Mend){++mends;assert(r.kind==CombatFeedback::Heal&&r.amount==game.enemyHp-hpBefore);assert(r.amount==(condition==2?2:condition==4?4:0));}
  else if(intent==rpg::Intent::Prepare){++prepares;assert(r.kind==CombatFeedback::Buff&&r.amount==0);}
  else if(game.dodge){++enemyMisses;assert(r.kind==CombatFeedback::Miss&&r.amount==0);}
  else if(game.damage){++enemyHits;assert(r.kind==CombatFeedback::Damage&&r.amount==game.damage&&r.critical==game.crit);}
  else {++zeroes;assert(r.kind==CombatFeedback::NoDamage&&r.amount==0);}
  auto blobs=nvs.blobs;for(unsigned i=0;i<5;++i)tickEnemyTurn(now+i);assert(!memcmp(&game,&expected,sizeof(game))&&blobs==nvs.blobs);
  now+=combatFx.length();assert(combatFx.expire(now));tickEnemyTurn(now);tickEnemyTurn(now+1);assert(!memcmp(&game,&expected,sizeof(game))&&!combatFx.active()&&blobs==nvs.blobs);
 }
 assert(mends&&prepares&&enemyHits&&enemyMisses&&zeroes&&skips);
 printf("PASS: visual77 %u real enemy dispatches; capped Mend, Prepare, hit/miss/zero, turned skip, target direction, deadline/expiry/repeated ticks and exact state/RNG.\n",enemyCases);
 // A stale nonzero damage field can never make a benefit into an injury.
 game.damage=33;game.crit=true;game.dodge=false;
 for(auto a:{rpg::Action::LayHands,rpg::Action::SecondWind,rpg::Action::Defensive,rpg::Action::ShieldSpell,rpg::Action::RagePower,rpg::Action::ActionSurge,rpg::Action::SacredWeapon,rpg::Action::TurnUndead})assert(!heroCombatResult(game,a).injury());
 assert(!enemyCombatResult(game,rpg::Intent::Mend,game.enemyHp).injury());assert(!enemyCombatResult(game,rpg::Intent::Prepare,game.enemyHp).injury());
 game.damage=0;assert(heroCombatResult(game,rpg::Action::Attack).kind==CombatFeedback::NoDamage);assert(!enemyCombatResult(game,rpg::Intent::Strike,game.enemyHp).injury());game.dodge=true;assert(heroCombatResult(game,rpg::Action::Attack).kind==CombatFeedback::Miss);
 // Save failure still pauses before any presentation begins.
 game=rpg::create(0,77);rpg::begin(game,0);assert(journal.save(game));view.page=Page::Battle;combatFx=CombatFx{};nvs.fail=true;action(rpg::Action::Attack);assert(view.page==Page::SaveError&&!combatFx.active());nvs.fail=false;
 CombatFx timer;timer.start(Effect::MagicDarts,false,UINT32_MAX-200);assert(timer.length()==1100);assert(timer.progress(339)==490&&!timer.expire(898));assert(timer.expire(899));
 assert(magePose(179).heroFrame==1&&magePose(180).heroFrame==2&&magePose(540).enemyX==5&&magePose(600).enemyX==7&&magePose(1100).heroFrame==0);
 printf("PASS: visual77 %u actual controller actions match rules/Save28/fatigue/RNG; damage, miss, heal, guard, buff, repeated touch, failed save and exact timing.\n",passed);
}
