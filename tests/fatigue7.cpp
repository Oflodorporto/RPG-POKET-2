// Package7: independent damage-only delta and the real receipt reconstruction.
#define main priorCombatTraceSemantics
#include "combat_trace_semantics.cpp"
#undef main
#include "../firmware/RPG_POKET_2/Persistence.h"
#include "../firmware/RPG_POKET_2/ClubDuel.h"
#include "Fatigue7Trace.h"
#include <climits>

static unsigned penalty(unsigned fatigue){return fatigue>70?(fatigue>=90?2:1):0;}
static unsigned reduced(unsigned value,unsigned fatigue){return value?std::max(1,int(value)-int(penalty(fatigue))):0;}
static unsigned fatigueSteps(const CombatTrace& t){unsigned n=0;for(unsigned i=0;i<t.count;++i)n+=t.steps[i].kind==TraceStep::Fatigue;return n;}
static Game fixture(unsigned cls,unsigned level,unsigned seed,bool modern){
 auto g=create(cls,seed);g.p.xp=dndXp[level-1];levelUp(g);g.dndProgression=modern;
 g.p.hp=std::max(1u,unsigned(g.p.maxhp)/2);g.p.mp=g.p.maxmp=totalMana(g);
 g.p.life=g.p.mana=3;g.oath=1;g.city=1;g.enemyId=seed%29;g.phase=Phase::Hero;g.enemyHp=60000;
 g.sacredTurns=cls==1?3:0;g.rageTurns=cls==3?3:0;g.forge[0]=seed%4;
 return g;
}
int main(){
 unsigned actions=0,receipts=0,misses=0,caps=0,enemyActions=0;
 for(unsigned fatigue=0;fatigue<=100;++fatigue){auto g=create(0,1);g.fatigue=fatigue;auto before=g;
  assert(fatigueCombatPenalty(g)==penalty(fatigue));
  for(unsigned damage:{0u,1u,2u,3u,20u,1000u,UINT_MAX}){
   auto expected=damage?damage-std::min(damage-1,penalty(fatigue)):0;
   assert(fatigueCombatDamage(g,damage)==expected);assert(!memcmp(&g,&before,sizeof(g)));
  }
 }
 for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)
 for(unsigned level:{1u,2u,3u,5u,9u,11u,15u,18u,20u})for(unsigned seed=1;seed<=64;++seed)
 for(unsigned a=0;a<=unsigned(Action::DivineSmite);++a){
  auto original=fixture(cls,level,seed,modern);auto fresh=original;CombatTrace untired;
  auto accepted=act(fresh,Action(a),&untired)==nullptr;if(accepted)verify(untired);
  for(unsigned fatigue:{69u,70u,71u,89u,90u,100u}){
   auto g=original;g.fatigue=fatigue;auto noTrace=g;CombatTrace trace;
   assert((act(g,Action(a),&trace)==nullptr)==accepted);assert((act(noTrace,Action(a))==nullptr)==accepted);
   assert(!memcmp(&g,&noTrace,sizeof(g)));assert(g.randomState==fresh.randomState);
   assert(effectiveAttack(g)==effectiveAttack(fresh)&&effectiveDefense(g)==effectiveDefense(fresh));
   assert(totalMana(g)==totalMana(fresh)&&g.p.atk==fresh.p.atk&&g.p.def==fresh.p.def);
   auto expected=fresh;expected.fatigue=fatigue;
   if(accepted){verify(trace);++receipts;
    assert(trace.potential==untired.potential&&trace.diceCount==untired.diceCount);
    assert(!memcmp(trace.dice,untired.dice,sizeof(trace.dice)));
    if(trace.damaging){
     assert(trace.net==reduced(untired.net,fatigue));
     assert(fatigueSteps(trace)==unsigned(penalty(fatigue)&&untired.net));
     expected.damage=trace.net;expected.enemyHp=original.enemyHp-trace.net;
     if(!untired.net){++misses;assert(!trace.net&&!trace.applied&&!fatigueSteps(trace));}
    }else assert(!fatigueSteps(trace));
   }
   assert(!memcmp(&g,&expected,sizeof(g)));++actions;
  }
  // Enemy damage, criticals, drains, healing, guard and rage remain unchanged.
  auto enemyStart=original;enemyStart.enemyHp=std::max(1u,unsigned(enemySpec(enemyStart.enemyId).hp)/2);
  auto freshEnemy=enemyStart;freshEnemy.phase=Phase::Enemy;freshEnemy.enemyBeat=seed%3;
  freshEnemy.guard=seed%2?75:0;CombatTrace freshReceipt;assert(enemy(freshEnemy,&freshReceipt));
  for(unsigned fatigue:{69u,70u,71u,100u}){auto g=enemyStart;g.phase=Phase::Enemy;g.enemyBeat=seed%3;g.guard=seed%2?75:0;g.fatigue=fatigue;CombatTrace t;assert(enemy(g,&t));verify(t);assert(!fatigueSteps(t));auto expected=freshEnemy;expected.fatigue=fatigue;assert(!memcmp(&g,&expected,sizeof(g)));assert(sameFatigue7Trace(t,freshReceipt));++enemyActions;}
 }
 // Reduction is after spell saves / aggregated multiattacks and before HP cap.
 // Event combats suppress loot rolls so exact resolution RNG can be compared
 // even when a deduction changes whether a tiny remaining target dies.
 for(unsigned cls=0;cls<4;++cls)for(unsigned seed=1;seed<=64;++seed)
 for(unsigned hp:{1u,2u,3u,4u})for(unsigned fatigue:{69u,70u,71u,100u})
 for(Action a:{Action::Attack,Action::Offensive,Action::MagicMissile,Action::BurningHands,Action::ScorchingRay,Action::Fireball,Action::DivineSmite}){
  auto fresh=fixture(cls,20,seed,true);fresh.enemyHp=hp;fresh.eventStage=2;auto g=fresh;g.fatigue=fatigue;
  CombatTrace untired,t;bool accepted=!act(fresh,a,&untired);assert((!act(g,a,&t))==accepted);if(!accepted)continue;
  verify(t);assert(t.net==reduced(untired.net,fatigue)&&t.applied==std::min(hp,unsigned(t.net)));
  assert(g.enemyHp==hp-t.applied&&g.damage==t.applied&&g.randomState==fresh.randomState);
  assert((g.phase==Phase::Won)==(t.applied==hp));++caps;
 }
 // Actual duel engine never reads solo fatigue: profiles and moves stay equal.
 for(unsigned cls=0;cls<4;++cls)for(unsigned fatigue:{69u,70u,71u,100u}){
  auto fresh=create(cls,7),tired=fresh;tired.fatigue=fatigue;
  club::Duel a,b;a.begin(2,true,fresh.p.cls,fresh.p.level,1,3,nullptr,nullptr,0);b.begin(2,true,tired.p.cls,tired.p.level,1,3,nullptr,nullptr,0);
  a.ready=b.ready=true;assert(!memcmp(a.fighters,b.fighters,sizeof(a.fighters)));
  assert(a.play(club::Action::Attack,1)&&b.play(club::Action::Attack,1));
  assert(a.lastDamage==b.lastDamage&&!memcmp(a.fighters,b.fighters,sizeof(a.fighters)));
 }
 // Save30 continues to persist just fatigue, never a stale derived penalty.
 for(unsigned cls=0;cls<4;++cls)for(unsigned fatigue:{69u,70u,71u,100u}){
  auto g=create(cls,77);g.fatigue=fatigue;uint8_t bytes[PERSISTENT_SAVE_SIZE];encodePersistent(g,13,2,bytes);
  Game loaded;uint32_t seq=0;assert(decodePersistent(bytes,2,loaded,seq)==Decode::Ok&&seq==13);
  assert(loaded.fatigue==fatigue&&fatigueCombatPenalty(loaded)==penalty(fatigue));
  assert(rest(loaded)&&!loaded.fatigue&&!fatigueCombatPenalty(loaded));
 }
 printf("PASS fatigue7: %u action comparisons, %u reconstructed receipts, %u zero-damage outcomes, %u HP caps, %u unchanged enemy actions; all classes, legacy/modern, thresholds 69/70/71/89/90/100, no added RNG, stats/PvP/Save30 preserved\n",actions,receipts,misses,caps,enemyActions);
}
