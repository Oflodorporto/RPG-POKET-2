#pragma once
// Include in exactly one original sketch/test translation unit, never runtime.
// This is the unchanged read-only eligibility predicate; no save or RNG writes.
#include "../Persistence.h"
namespace coopplatform {
// Conservative training-only gate. Durable spent abilities remain untouched;
// active combat buffs/pending actions are not silently imported or discarded.
bool trainingEligible(const rpg::Game& g){
 return rpg::persistentValid(g)&&g.phase==rpg::Phase::Home&&g.p.hp&&
 !g.tripStage&&!g.campStage&&!g.dungeonFlags&&!g.dungeonGenerator&&
 rpg::dailyStateEmpty(g.daily)&&!g.campaignStage&&!g.discovery&&g.eventStage!=2&&
 g.clubStage!=1&&g.clubStage!=2&&!g.guard&&!g.sacredTurns&&!g.turnedTurns&&
 !g.rageTurns&&!g.surgePending&&!g.surgeTurnUsed&&!g.chestLock&&!g.chestPick&&!g.chestTrap;
}
}
