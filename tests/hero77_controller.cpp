#define main priorTitleControllerRegression
#include "title_controller.cpp"
#undef main

#include <array>
#include <fstream>

namespace {
constexpr unsigned actionCount=unsigned(rpg::Action::DivineSmite)+1;
struct Coverage {unsigned accepted=0,rejected=0,damage=0,miss=0,zero=0,critical=0,heal=0,guard=0,buff=0;};
Coverage coverage[2][4][actionCount];
rpg::Action normalized(unsigned cls,bool modern,rpg::Action a){
 if(modern&&cls==0&&a==rpg::Action::Offensive)return rpg::Action::MagicMissile;
 if(modern&&cls==0&&a==rpg::Action::Defensive)return rpg::Action::ShieldSpell;
 return a;
}
bool animated(rpg::Action a){return a!=rpg::Action::Life&&a!=rpg::Action::Mana&&a!=rpg::Action::Flee;}
bool selfTarget(rpg::Action a){return a==rpg::Action::Defensive||a==rpg::Action::ShieldSpell||a==rpg::Action::LayHands||a==rpg::Action::SecondWind||a==rpg::Action::RagePower||a==rpg::Action::ActionSurge||a==rpg::Action::SacredWeapon;}
CombatFeedback expectedFeedback(const rpg::Game& g,rpg::Action a){
 if(a==rpg::Action::LayHands||a==rpg::Action::SecondWind)return CombatFeedback::Heal;
 if(a==rpg::Action::Defensive||a==rpg::Action::ShieldSpell)return CombatFeedback::Guard;
 if(a==rpg::Action::RagePower||a==rpg::Action::ActionSurge||a==rpg::Action::SacredWeapon||a==rpg::Action::TurnUndead)return CombatFeedback::Buff;
 return g.dodge?CombatFeedback::Miss:g.damage?CombatFeedback::Damage:CombatFeedback::NoDamage;
}
void fixture(unsigned cls,bool modern,unsigned level,unsigned seed,bool dungeon=false){
 nvs=Store{};backend.slot=0;assert(journal.load(game)==rpg::Load::Empty);
 game=modern?rpg::create(cls,seed):rpg::testHero(cls,seed);
 if(modern){game.p.xp=rpg::dndXp[level-1];rpg::levelUp(game);game.oath=cls==1&&level>=3;}
 game.tutorial=true;game.city=1;game.p.life=game.p.mana=2;
 if(dungeon){game.crystals=1;assert(!rpg::enterDungeon(game,0));game.dungeonXY=0x72;game.dungeonFlags=(game.dungeonFlags&3)|4;}
 assert(rpg::begin(game,2));game.p.hp=std::max(1u,unsigned(game.p.maxhp)/2);game.p.mp=game.p.maxmp;game.fatigue=seed%101;
 assert(rpg::valid(game)&&journal.save(game));view=ViewState{};view.page=dungeon?Page::Dungeon:Page::Battle;combatFx=CombatFx{};menu.journey=Journey{};now=UINT32_MAX-200;
}
void sameState(const rpg::Game& expected,const Store& stored,uint32_t seq){
 assert(!memcmp(&game,&expected,sizeof(game)));assert(game.randomState==expected.randomState);
 assert(nvs.blobs==stored.blobs&&journal.seq==seq);
 uint8_t a[rpg::SAVE_SIZE],b[rpg::SAVE_SIZE];rpg::encode(game,seq,a);rpg::encode(expected,seq,b);
 assert(rpg::get16(a,4)==28&&!memcmp(a,b,sizeof(a)));
}
void runCase(unsigned cls,bool modern,unsigned level,unsigned seed,rpg::Action requested,bool dungeon=false){
 fixture(cls,modern,level,seed,dungeon);if(requested==rpg::Action::Mana){assert(game.p.mp);--game.p.mp;assert(journal.save(game));}const auto a=normalized(cls,modern,requested);unsigned hp=game.p.hp;
 // The oracle resolves once through Rules and saves through an independent real
 // Save28 + auxiliary-fatigue journal. The controller must produce identical bytes.
 Store expectedStore=nvs;NvsBackend expectedBackend(expectedStore);rpg::FatigueJournal<NvsBackend> expectedJournal(expectedBackend);rpg::Game loaded;
 assert(expectedJournal.load(loaded)==rpg::Load::Ok);auto expected=game;const char* error=rpg::act(expected,a);
 auto& c=coverage[modern][cls][unsigned(requested)];
 if(!error)assert(expectedJournal.save(expected));
 action(requested);sameState(expected,expectedStore,expectedJournal.seq);
 if(error){++c.rejected;assert(!combatFx.active());return;}
 ++c.accepted;
 assert(combatFx.active()==animated(a));
 if(!animated(a))return;
 const auto& result=combatFx.result;
 assert(result.hasHeroAction&&result.heroAction==a&&!result.enemyAction);
 assert(result.kind==expectedFeedback(game,a));assert(combatFx.onHero==selfTarget(a));
 assert(result.amount==((result.kind==CombatFeedback::Damage||result.kind==CombatFeedback::Heal)?game.damage:0));
 assert(result.critical==(result.kind==CombatFeedback::Damage&&game.crit));
 assert(result.recoveredHp==(cls==3&&a==rpg::Action::Defensive?game.p.hp-hp:0));
 assert(result.injury()==(result.kind==CombatFeedback::Damage&&result.amount>0));
 c.damage+=result.kind==CombatFeedback::Damage;c.miss+=result.kind==CombatFeedback::Miss;c.zero+=result.kind==CombatFeedback::NoDamage;c.critical+=result.critical;c.heal+=result.kind==CombatFeedback::Heal;c.guard+=result.kind==CombatFeedback::Guard;c.buff+=result.kind==CombatFeedback::Buff;
 if(a==rpg::Action::TurnUndead)assert(!combatFx.onHero&&game.turnedTurns==2&&!result.injury()&&result.amount==0&&game.damage==0);
 const auto length=combatFx.length();
 const auto effect=combatFx.kind;const unsigned expectedLength=effect==Effect::DivineSlash?1050:effect==Effect::MagicDarts||effect==Effect::FlameVolley?1100:effect==Effect::FireBurst||effect==Effect::Radiant||effect==Effect::Lightning?1200:effect==Effect::Rage||effect==Effect::Projectile?960:effect==Effect::Thrust?640:480;assert(length==expectedLength);assert(enemyAt==uint32_t(combatFx.started+850));
 for(unsigned elapsed:{0u,1u,59u,60u,unsigned(length/2),unsigned(length-1)}){
  now=uint32_t(combatFx.started+elapsed);assert(combatFx.progress(now)==elapsed*1000/length);
  assert(!combatFx.expire(now));
  for(int tap=0;tap<4;++tap){tapped(40,250);tapped(170,290);tapped(80,100);tickEnemyTurn(now);}
  sameState(expected,expectedStore,expectedJournal.seq);
 }
 now=uint32_t(combatFx.started+length);assert(combatFx.progress(now)==1000);assert(combatFx.expire(now)&&!combatFx.active());assert(!combatFx.expire(now+1));
 sameState(expected,expectedStore,expectedJournal.seq);
}
}

int main(int argc,char** argv){
 unsigned cases=0;
 for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned level:{1u,2u,3u,5u,11u,20u}){
  if(!modern&&level!=3)continue;
  for(unsigned seed=1;seed<=64;++seed)for(unsigned index=0;index<actionCount;++index){runCase(cls,modern,level,seed,rpg::Action(index));++cases;}
 }
 // Dungeon dispatch uses the same resolved action and never changes spatial,
 // range, reward, save, fatigue or RNG semantics while its effect runs.
 for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned index=0;index<actionCount;++index){runCase(cls,modern,modern?20:3,17,rpg::Action(index),true);++cases;}
 // Seed 94 resolves all three real Scorching Ray attack rolls to zero.
 runCase(0,true,3,94,rpg::Action::ScorchingRay);++cases;
 unsigned damage=0,miss=0,zero=0,critical=0,heal=0,guard=0,buff=0;
 for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned index=0;index<actionCount;++index){
  const auto& c=coverage[modern][cls][index];damage+=c.damage;miss+=c.miss;zero+=c.zero;critical+=c.critical;heal+=c.heal;guard+=c.guard;buff+=c.buff;
  if(index<=unsigned(rpg::Action::Flee))assert(c.accepted);
  if(!modern&&rpg::powerAction(rpg::Action(index)))assert(!c.accepted&&c.rejected);
 }
 for(unsigned index=unsigned(rpg::Action::MagicMissile);index<actionCount;++index){unsigned accepted=0;for(unsigned cls=0;cls<4;++cls)accepted+=coverage[1][cls][index].accepted;assert(accepted);}
 assert(damage&&miss&&zero&&critical&&heal&&guard&&buff);
 // Guard + actual capped recovery is separate from the Rules damage field.
 for(bool modern:{false,true})for(unsigned missing:{0u,1u,2u,99u}){
  fixture(3,modern,20,33);game.p.hp=game.p.maxhp-std::min(missing,unsigned(game.p.maxhp)-1);unsigned hp=game.p.hp;assert(journal.save(game));
  action(rpg::Action::Defensive);assert(combatFx.active()&&combatFx.onHero);assert(game.damage==0&&game.guard==50);
  assert(combatFx.result.kind==CombatFeedback::Guard&&combatFx.result.amount==0&&!combatFx.result.injury());
  assert(combatFx.result.recoveredHp==game.p.hp-hp);assert(combatFx.result.recoveredHp==std::min(missing,std::max(1u,unsigned(game.p.maxhp)/4)));
 }
 // Snapshot semantics never reuse stale damage/critical state for benefits.
 for(unsigned cls=0;cls<4;++cls){game.p.cls=cls;game.damage=77;game.crit=true;game.dodge=true;
  for(auto a:{rpg::Action::LayHands,rpg::Action::SecondWind,rpg::Action::Defensive,rpg::Action::ShieldSpell,rpg::Action::RagePower,rpg::Action::ActionSurge,rpg::Action::SacredWeapon,rpg::Action::TurnUndead}){
   const auto r=heroCombatResult(game,a);assert(r.hasHeroAction&&r.heroAction==a&&!r.injury()&&!r.critical&&r.recoveredHp==0);
  }
 }
 for(bool dodge:{false,true}){game.damage=0;game.crit=true;game.dodge=dodge;const auto r=heroCombatResult(game,rpg::Action::Attack);assert(r.kind==(dodge?CombatFeedback::Miss:CombatFeedback::NoDamage)&&!r.injury()&&r.amount==0);}
 // Save failure locks the already-resolved action without starting presentation;
 // retry persists that same resolution, not another roll or resource charge.
 fixture(0,true,5,55);auto expected=game;assert(!rpg::act(expected,rpg::Action::BurningHands));nvs.fail=true;action(rpg::Action::BurningHands);assert(view.page==Page::SaveError&&!combatFx.active()&&!memcmp(&game,&expected,sizeof(game)));nvs.fail=false;tapped(80,270);assert(!memcmp(&game,&expected,sizeof(game))&&!combatFx.active()&&view.page==Page::Battle);
 if(argc>1){std::ofstream csv(argv[1]);assert(csv);csv<<"progression,class,action,accepted,rejected,damage,miss,zero,critical,heal,guard,buff\n";for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned index=0;index<actionCount;++index){const auto& c=coverage[modern][cls][index];csv<<(modern?"new":"legacy")<<','<<cls<<','<<index<<','<<c.accepted<<','<<c.rejected<<','<<c.damage<<','<<c.miss<<','<<c.zero<<','<<c.critical<<','<<c.heal<<','<<c.guard<<','<<c.buff<<'\n';}}
 printf("PASS: hero77 %u controller cases, every legacy/new class action; exact Game/Save28/auxiliary-fatigue/RNG, normalized action identity, hit/miss/%u zero/critical/heal/guard/buff, TurnUndead target, capped barbarian heal, repeated taps, UINT32 wrap, and failed-save retry.\n",cases,zero);
}
