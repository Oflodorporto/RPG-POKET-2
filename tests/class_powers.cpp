#include "LegacySave.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include "TestHero.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace rpg;
Game atLevel(unsigned cls,unsigned lv){auto g=create(cls,47);g.p.xp=dndXp[lv-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;return g;}
Game checkpoint(const Game& g){assert(valid(g));uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,7,b);Game copy;uint32_t seq;assert(decode(b,copy,seq)==Decode::Ok&&seq==7);encode(copy,7,c);assert(!memcmp(b,c,SAVE_SIZE));return copy;}
void rejected(Game& g,Action a){uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,9,b);assert(act(g,a));encode(g,9,c);assert(!memcmp(b,c,SAVE_SIZE));}
int main(){
 auto g=atLevel(1,1);g.p.hp=1;assert(!usePower(g,Action::LayHands));assert(g.p.hp==6&&g.laySpent==5);g=checkpoint(g);auto hp=g.p.hp;assert(usePower(g,Action::LayHands)&&g.p.hp==hp);assert(rest(g)&&g.laySpent==0);
 assert(swearDevotion(g));g=atLevel(1,3);assert(!swearDevotion(g)&&g.oath==1);assert(swearDevotion(g));g=checkpoint(g);g.p.hp=g.p.maxhp=100;assert(begin(g,7));assert(!act(g,Action::SacredWeapon));assert(g.channelSpent&&g.sacredTurns==3);g=checkpoint(g);assert(enemy(g));rejected(g,Action::TurnUndead);
 for(unsigned i=0;i<3;++i){assert(!act(g,Action::Attack));assert(g.sacredTurns==2-i);g=checkpoint(g);if(g.phase==Phase::Enemy)enemy(g);else break;}
 g=atLevel(1,3);swearDevotion(g);begin(g,0);rejected(g,Action::TurnUndead);assert(!g.channelSpent);g=atLevel(1,3);swearDevotion(g);begin(g,5);assert(!act(g,Action::TurnUndead));assert(incomingCeiling(g)==0);g=checkpoint(g);for(unsigned i=0;i<2;++i){auto before=g.p.hp;assert(enemy(g)&&g.p.hp==before);assert(g.turnedTurns==1-i);if(!i){assert(!act(g,Action::Defensive));g=checkpoint(g);}}assert(g.channelSpent);
 // A completed camp refreshes the pool; a pending camp and a potion never do.
 home(g);g.phase=Phase::Home;g.sacredTurns=g.turnedTurns=0;g.laySpent=3;g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;assert(!startCamp(g,false,false));g=checkpoint(g);assert(g.channelSpent&&g.laySpent==3);g.campStage=3;assert(finishCamp(g)&&!g.channelSpent&&!g.laySpent);g=checkpoint(g);
 for(auto a:{Action::MagicMissile,Action::BurningHands,Action::ShieldSpell,Action::ScorchingRay,Action::Fireball}){g=atLevel(0,powerLevel(a));begin(g,7);auto mp=g.p.mp;assert(!act(g,a));assert(g.p.mp==mp-powerCost(a));g=checkpoint(g);if(a==Action::MagicMissile)assert(g.damage>=6&&g.damage<=15&&!g.dodge&&!g.crit);if(a==Action::ShieldSpell)assert(g.guard==75);if(powerLevel(a)>1){g=atLevel(0,powerLevel(a)-1);begin(g,7);rejected(g,a);}g=atLevel(0,powerLevel(a));begin(g,7);g.p.mp=0;rejected(g,a);}
 g=atLevel(0,1);begin(g,0);rejected(g,Action::LayHands);g=testHero(0,1);begin(g,0);rejected(g,Action::MagicMissile);
 // Save12 migration preserves every old field and leaves new powers unused.
 g=atLevel(1,3);uint8_t b[SAVE_SIZE];encode(g,2,b);legacyFormat(b,12);put32(b,124,crc(b,124));Game copy;uint32_t seq;assert(decode(b,copy,seq)==Decode::Ok&&!copy.oath&&!copy.laySpent&&!copy.channelSpent&&copy.p.level==3&&copy.dndProgression);copy=checkpoint(copy);
 for(unsigned offset=109;offset<114;++offset){encode(g,2,b);b[offset]=255;put32(b,124,crc(b,124));assert(decode(b,copy,seq)==Decode::Corrupt);}
 puts("PASS: oath choice, level/class/MP gates, no mutation on reject, heal pool/rest, shared channel, sacred duration, undead two turns, 5 spells and save13/read12.");
}
