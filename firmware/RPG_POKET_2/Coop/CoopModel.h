#pragma once
// Training-only cooperative model. Enabled by the explicit local candidate flag;
// campaign writes/rewards and full class powers are deliberately absent.
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include "../Rules.h"
namespace coop {
constexpr uint8_t kRules=1, kPlayers=2, kPlayerCapacity=3, kEnemies=3, kActors=6;
constexpr uint8_t kNoPeer=255, kNoActor=255;
enum class Kind:uint8_t { Empty, Player, Enemy, Boss };
enum class Phase:uint8_t { Playing, Won, Lost };
enum class Action:uint8_t { Attack, Guard, Life, Mana, Focus };
enum class Error:uint8_t { None, Paused, Busy, Peer, Sequence, Conflict, Revision, Turn, Target, Resource, Invalid, Limit };
struct Actor {
 uint8_t id=0,owner=0; Kind kind=Kind::Empty; uint8_t species=0,cls=0,level=0,race=0,shirt=0,trousers=0;
 uint16_t hp=0,maxhp=0,mp=0,maxmp=0,attack=0,defense=0;
 uint8_t guard=0,criticalChance=0,attacks=0,fatiguePenalty=0,life=0,mana=0;
};
struct State {
 uint8_t rules=kRules; Phase phase=Phase::Playing; uint8_t wave=0; uint16_t round=1;
 uint8_t turn=0,players=kPlayers; uint32_t revision=0,rng=1,accepted[kPlayers]={}; Actor actors[kActors]{};
};
struct Command {uint32_t sequence=0,revision=0; uint8_t actor=0,target=0; Action action=Action::Attack;};
struct Effect {
 uint32_t beforeHash=0,commandSequence=0; uint8_t peer=kNoPeer,actor=kNoActor,target=kNoActor;
 Action action=Action::Attack; uint16_t amount=0; bool critical=false; uint8_t fromWave=0;
};
struct Receipt {uint64_t nonce=0; uint32_t revision=0,stateHash=0; Phase result=Phase::Playing; bool valid=false;};
inline bool sameCommand(const Command&a,const Command&b){return a.sequence==b.sequence&&a.revision==b.revision&&a.actor==b.actor&&a.target==b.target&&a.action==b.action;}
inline bool playerProfile(const rpg::Game& g,uint8_t peer,Actor& a){
 if(peer>=kPlayers||g.p.cls>3||g.p.level<1||g.p.level>99||(g.dndProgression&&g.p.level>20)||g.race>3||g.shirt>7||g.trousers>7||
    !g.p.hp||g.p.hp>g.p.maxhp||g.p.mp>g.p.maxmp||g.p.life>99||g.p.mana>99||g.fatigue>100)return false;
 // Validate fields before passing them to existing helpers that index fixed tables.
 for(uint8_t f:g.forge)if(f>3)return false;
 for(uint8_t e:g.equipped)if(e&&!rpg::gearId(e))return false;
 for(uint8_t x:g.attributes)if(x<1||x>30)return false;
 int attack=rpg::effectiveAttack(g),defense=rpg::effectiveDefense(g);
 if(attack<0||attack>1024||defense<0||defense>1024)return false;
 a={};a.id=peer+1;a.owner=peer;a.kind=Kind::Player;a.cls=g.p.cls;a.level=g.p.level;
 a.race=g.race;a.shirt=g.shirt;a.trousers=g.trousers;
 a.hp=g.p.hp;a.maxhp=g.p.maxhp;a.mp=g.p.mp;a.maxmp=g.p.maxmp;
 a.attack=uint16_t(attack);a.defense=uint16_t(defense);a.attacks=uint8_t(rpg::attacksPerAction(g));
 a.criticalChance=uint8_t(rpg::weaponCriticalChance(g));a.fatiguePenalty=rpg::fatigueCombatPenalty(g);
 a.life=g.p.life;a.mana=g.p.mana;return true;
}
inline void spawnWave(State&s){
 // Demonstration content only: 2 waves of 3 existing species, then one Guardian.
 static constexpr uint8_t species[2][3]={{0,1,2},{4,1,2}};
 for(uint8_t i=0;i<kEnemies;++i){Actor&a=s.actors[kPlayerCapacity+i];a={};if(s.wave==2&&i)continue;
  a.id=uint8_t(10+s.wave*kEnemies+i);a.owner=kNoPeer;a.kind=s.wave==2?Kind::Boss:Kind::Enemy;
  a.species=s.wave==2?3:species[s.wave][i];const auto&e=rpg::enemySpec(a.species);
  a.hp=a.maxhp=e.hp;a.attack=e.atk;a.defense=e.def;a.level=uint8_t(rpg::enemyRewardLevel(a.species));a.criticalChance=12;a.attacks=1;
 }
}
inline bool anyAlive(const State&s,bool players){for(uint8_t i=players?0:kPlayerCapacity;i<(players?kPlayers:kActors);++i)if(s.actors[i].hp)return true;return false;}
inline uint8_t firstEnemy(const State&s){for(uint8_t i=kPlayerCapacity;i<kActors;++i)if(s.actors[i].hp)return i;return kNoActor;}
inline uint8_t actorSlot(const State&s,uint8_t id){for(uint8_t i=0;i<kActors;++i)if(s.actors[i].kind!=Kind::Empty&&s.actors[i].id==id)return i;return kNoActor;}
inline bool initialize(State&s,const rpg::Game&one,const rpg::Game&two,uint32_t seed){State n{};n.rng=seed?seed:1;if(!playerProfile(one,0,n.actors[0])||!playerProfile(two,1,n.actors[1]))return false;spawnWave(n);s=n;return true;}
inline void advance(State&s){
 if(!anyAlive(s,true)){s.phase=Phase::Lost;return;}
 if(!anyAlive(s,false)){
  if(s.wave==2){s.phase=Phase::Won;return;}
  ++s.wave;s.round=1;spawnWave(s);s.turn=s.actors[0].hp?0:1;return;
 }
 for(uint8_t n=0;n<kActors;++n){s.turn=uint8_t((s.turn+1)%kActors);if(!s.turn)++s.round;if(s.actors[s.turn].hp)return;}
}
inline bool validState(const State&s);
inline Error legal(const State&s,uint8_t peer,const Command&c,bool enemy=false){
 if(!validState(s)||s.phase!=Phase::Playing)return Error::Invalid;
 if(c.revision!=s.revision)return Error::Revision;
 if(s.revision==UINT32_MAX||s.round==UINT16_MAX)return Error::Limit;
 if(c.actor>=kActors||c.target>=kActors||uint8_t(c.action)>uint8_t(Action::Focus))return Error::Invalid;
 if(c.actor!=s.turn||!s.actors[c.actor].hp)return Error::Turn;
 const Actor&a=s.actors[c.actor];const Actor&t=s.actors[c.target];
 if(enemy){if(a.owner!=kNoPeer||c.action!=Action::Attack)return Error::Peer;}
 else if(peer>=kPlayers||a.kind!=Kind::Player||a.owner!=peer)return Error::Peer;
 if(enemy?c.sequence!=0:(s.accepted[peer]==UINT32_MAX||c.sequence!=s.accepted[peer]+1))return Error::Sequence;
 if(!t.hp)return Error::Target;
 if(c.action==Action::Attack||c.action==Action::Focus){if((a.kind==Kind::Player)==(t.kind==Kind::Player))return Error::Target;}
 else if(c.target!=c.actor)return Error::Target;
 if(c.action==Action::Focus&&a.mp<2)return Error::Resource;
 if(c.action==Action::Life&&(!a.life||a.hp==a.maxhp))return Error::Resource;
 if(c.action==Action::Mana&&(!a.mana||a.mp==a.maxmp))return Error::Resource;
 return Error::None;
}
inline Error apply(State&s,uint8_t peer,const Command&c,Effect&fx,bool enemy=false){
 Error e=legal(s,peer,c,enemy);if(e!=Error::None)return e;
 fx={};fx.commandSequence=c.sequence;fx.peer=peer;fx.actor=s.actors[c.actor].id;fx.target=s.actors[c.target].id;fx.action=c.action;fx.fromWave=s.wave;
 Actor&a=s.actors[c.actor];Actor&t=s.actors[c.target];
 if(c.action==Action::Guard){a.guard=50;}
 else if(c.action==Action::Life||c.action==Action::Mana){bool m=c.action==Action::Mana;auto&v=m?a.mp:a.hp;auto&count=m?a.mana:a.life;fx.amount=rpg::recovery(v,m?a.maxmp:a.maxhp,m);v+=fx.amount;--count;}
 else {
  // Isolated scratch RNG: no campaign Game, finish(), rewards or progression writes.
  rpg::Game scratch{};scratch.randomState=s.rng;
  unsigned damage=unsigned(rpg::rollDamage(scratch,a.attack,t.defense,a.criticalChance));
  if(a.kind==Kind::Player)damage*=a.attacks; // Existing aggregate Extra Attack model.
  if(c.action==Action::Focus){a.mp-=2;damage+=3;} // Explicitly provisional demonstration skill.
  bool dodge=rpg::random(scratch)%100<6;fx.critical=scratch.crit;
  if(a.fatiguePenalty&&damage)damage-=std::min<unsigned>(a.fatiguePenalty,damage-1);
  damage=(damage*(100-t.guard)+99)/100;t.guard=0;
  fx.amount=uint16_t(dodge?0:std::min<unsigned>(t.hp,damage));t.hp-=fx.amount;s.rng=scratch.randomState;
 }
 ++s.revision;if(!enemy)s.accepted[peer]=c.sequence;advance(s);return Error::None;
}
inline Command enemyCommand(const State&s){Command c{};c.revision=s.revision;c.actor=s.turn;c.target=0;
 // Stable, transparent AI: focus the living player with lowest absolute HP; tie: player 1.
 if(!s.actors[0].hp||(s.actors[1].hp&&s.actors[1].hp<s.actors[0].hp))c.target=1;return c;}
inline bool validState(const State&s){
 if(s.rules!=kRules||uint8_t(s.phase)>2||s.wave>2||!s.round||s.players!=kPlayers||s.turn>=kActors||!s.rng)return false;
 if(uint64_t(s.accepted[0])+s.accepted[1]>s.revision)return false;
 for(uint8_t i=0;i<kActors;++i){const Actor&a=s.actors[i];bool empty=i==2||(s.wave==2&&i>=4);
  if(empty){if(a.id||a.owner||a.kind!=Kind::Empty||a.species||a.cls||a.level||a.race||a.shirt||a.trousers||a.hp||a.maxhp||a.mp||a.maxmp||a.attack||a.defense||a.guard||a.criticalChance||a.attacks||a.fatiguePenalty||a.life||a.mana)return false;continue;}
  if(!a.maxhp||a.hp>a.maxhp||a.mp>a.maxmp||a.attack>1024||a.defense>1024||a.guard>50||a.criticalChance>100||!a.attacks||a.attacks>4||a.fatiguePenalty>2||a.life>99||a.mana>99)return false;
  if(i<kPlayers){if(a.kind!=Kind::Player||a.owner!=i||a.id!=i+1||a.cls>3||a.level<1||a.level>99||a.race>3||a.shirt>7||a.trousers>7||a.species)return false;}
  else if(a.owner!=kNoPeer||a.id!=10+s.wave*kEnemies+i-kPlayerCapacity||a.kind!=(s.wave==2?Kind::Boss:Kind::Enemy)||a.species>=29||a.cls||a.race||a.shirt||a.trousers||a.mp||a.maxmp||a.life||a.mana||a.attacks!=1||a.level<1||a.level>20)return false;
 }
 bool p=anyAlive(s,true),e=anyAlive(s,false);
 return s.phase==Phase::Playing?(p&&e&&s.actors[s.turn].hp):s.phase==Phase::Won?(s.wave==2&&p&&!e):!p;
}
}
