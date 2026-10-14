#pragma once
#include "../firmware/RPG_POKET_2/CombatTrace.h"
#include <cstring>
// Compare every meaningful receipt field, excluding compiler-owned padding.
inline bool sameFatigue7Trace(const rpg::CombatTrace& a,const rpg::CombatTrace& b){
 if(a.count!=b.count||a.diceCount!=b.diceCount||a.action!=b.action||a.intent!=b.intent||a.actorClass!=b.actorClass||a.enemyId!=b.enemyId||
    a.valid!=b.valid||a.enemy!=b.enemy||a.healing!=b.healing||a.damaging!=b.damaging||a.overflow!=b.overflow||
    a.potential!=b.potential||a.net!=b.net||a.applied!=b.applied||a.targetHp!=b.targetHp||a.mpCost!=b.mpCost||a.mpDrained!=b.mpDrained||
    memcmp(a.dice,b.dice,sizeof(a.dice)))return false;
 for(unsigned i=0;i<a.count;++i){const auto& x=a.steps[i];const auto& y=b.steps[i];if(x.kind!=y.kind||x.a!=y.a||x.b!=y.b||x.c!=y.c)return false;}
 return true;
}
