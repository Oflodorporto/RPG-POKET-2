#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/Progression.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace rpg;
Game warrior(unsigned level,unsigned seed){auto g=create(2,seed);g.p.xp=dndXp[level-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;return g;}
int main(){
 unsigned oldCrit=0,improved=0,superior=0;
 for(unsigned seed=1;seed<=10000;++seed){auto a=warrior(2,seed),b=warrior(3,seed),c=warrior(15,seed);assert(begin(a,7)&&begin(b,7)&&begin(c,7));assert(!act(a,Action::Attack)&&!act(b,Action::Attack)&&!act(c,Action::Attack));assert(!a.crit||b.crit);assert(!b.crit||c.crit);oldCrit+=a.crit;improved+=b.crit;superior+=c.crit;}
 assert(oldCrit>1000&&oldCrit<1400&&improved>1800&&improved<2200&&superior>2800&&superior<3200);
 for(unsigned cls=0;cls<4;++cls)for(unsigned level=1;level<=20;++level){auto g=create(cls,71);g.p.xp=dndXp[level-1];levelUp(g);assert(weaponCriticalChance(g)==(cls==2&&level>=3?(level>=15?30:20):12));g.dndProgression=false;assert(weaponCriticalChance(g)==12&&!survivorRecovery(g));}
 // Enemy critical chance remains 12%, even against the superior Champion.
 auto g=warrior(15,91);begin(g,7);g.phase=Phase::Enemy;auto oracle=g;int damage=rollDamage(oracle,encounterAttack(g),effectiveDefense(g));bool dodge=random(oracle)%100<6;auto hp=g.p.hp;enemy(g);assert(g.crit==oracle.crit&&g.p.hp==hp-(dodge?0:std::min<int>(hp,damage)));
 // Half-HP threshold, positive HP only, combat only; no effect in menus.
 for(unsigned level:{17u,18u,20u})for(unsigned hp=0;hp<=150;++hp){g=warrior(level,61);g.p.hp=std::min<unsigned>(hp,g.p.maxhp);g.phase=Phase::Home;assert(!survivorRecovery(g));g.phase=Phase::Hero;bool expected=level>=18&&g.p.hp&&unsigned(g.p.hp)*2<=g.p.maxhp;assert(bool(survivorRecovery(g))==expected);auto before=g.p.hp;startHeroTurn(g);assert(g.p.hp==(expected?before+5+abilityMod(g.attributes[2]):before));assert(g.p.hp<=g.p.maxhp);}
 g=warrior(18,42);g.p.hp=1;unsigned heal=5+abilityMod(g.attributes[2]);assert(begin(g,7)&&g.p.hp==1+heal);assert(!begin(g,7)&&g.p.hp==1+heal);
 // Extra action is the same turn. It cannot farm Survivor healing.
 assert(!act(g,Action::ActionSurge));auto before=g.p.hp;assert(!act(g,Action::Defensive)&&g.phase==Phase::Hero&&g.p.hp==before);assert(!act(g,Action::Defensive)&&g.phase==Phase::Enemy&&g.p.hp==before);
 // Enemy preparation/mending still starts one new hero turn, once.
 g=warrior(18,42);begin(g,7);g.p.hp=1;g.phase=Phase::Enemy;g.enemyBeat=1;assert(enemy(g)&&g.p.hp==1+heal);assert(!enemy(g)&&g.p.hp==1+heal);
 g=warrior(18,42);begin(g,2);g.p.hp=1;g.phase=Phase::Enemy;g.enemyBeat=1;assert(enemy(g)&&g.p.hp==1+heal);
 g=warrior(18,42);begin(g,7);g.p.hp=0;g.phase=Phase::Enemy;enemy(g);assert(g.phase==Phase::Lost&&!g.p.hp);
 // Read/reboot and rejected input do not grant another recovery.
 g=warrior(18,42);g.p.hp=1;begin(g,7);uint8_t bytes[SAVE_SIZE],again[SAVE_SIZE];assert(valid(g));encode(g,2,bytes);Game boot;uint32_t seq;assert(decode(bytes,boot,seq)==Decode::Ok);assert(boot.p.hp==g.p.hp);encode(boot,2,again);assert(!memcmp(bytes,again,SAVE_SIZE));auto rng=boot.randomState;assert(act(boot,Action(255)));assert(boot.p.hp==g.p.hp&&boot.randomState==rng);
 for(auto level:{3u,15u,18u}){auto gains=levelBenefits(g,level);bool found=false;for(unsigned i=0;i<gains.count;++i)found|=strstr(gains.lines[i],level==3?"Campeao":level==15?"superior":"Sobrevivente")!=nullptr;assert(found);}
 printf("PASS: 30000 Champion attacks, critical rates %u/%u/%u; enemy and legacy preservation, Survivor thresholds/turns/death/Surge/save19.\n",oldCrit,improved,superior);
}
