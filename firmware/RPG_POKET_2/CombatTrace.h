#pragma once
#include <stdint.h>
// Optional, bounded RAM receipt. This is not part of Game, a save or the wire.
// Values are captured by the resolver; presentation never consumes randomness.
namespace rpg {
enum class TraceStep:uint8_t {AttackStat, Equipment, Weapon, Critical, Attacks, Technique, Rage, Sacred,
 Dice, RayHit, Save, Evocation, Half, Dodge, Heavy, Guard, Resistance, Flee, Fixed, Defense, Restore, Skip, Fatigue};
enum class TraceDice:uint8_t {Spell, Missile, Ray1, Ray2, Ray3, Brutal, Smite, Wind};
struct CombatTraceStep {TraceStep kind=TraceStep::Fixed;int16_t a=0,b=0,c=0;};
struct CombatTrace {
 static constexpr unsigned maxSteps=24,maxDice=32;
 CombatTraceStep steps[maxSteps];uint8_t dice[maxDice]={};
 uint8_t count=0,diceCount=0,action=0,intent=0,actorClass=0,enemyId=0;
 bool valid=false,enemy=false,healing=false,damaging=false,overflow=false;
 uint16_t potential=0,net=0,applied=0,targetHp=0,mpCost=0,mpDrained=0;
 void start(uint8_t a,bool foe=false){*this=CombatTrace{};valid=true;action=a;enemy=foe;}
 void add(TraceStep k,int a=0,int b=0,int c=0){if(count<maxSteps)steps[count++]={k,int16_t(a),int16_t(b),int16_t(c)};else overflow=true;}
 void die(TraceDice group,unsigned sides,unsigned value){
  if(diceCount>=maxDice){overflow=true;return;}
  if(count&&steps[count-1].kind==TraceStep::Dice&&unsigned(steps[count-1].c)==(unsigned(group)<<8|sides))++steps[count-1].b;
  else add(TraceStep::Dice,diceCount,1,(unsigned(group)<<8)|sides);
  dice[diceCount++]=value;
 }
};
static_assert(sizeof(CombatTrace)<=256,"Combat trace must remain a small fixed RAM receipt");
inline void traceStep(CombatTrace* t,TraceStep k,int a=0,int b=0,int c=0){if(t)t->add(k,a,b,c);}
inline void traceDie(CombatTrace* t,TraceDice group,unsigned sides,unsigned value){if(t)t->die(group,sides,value);}
}
