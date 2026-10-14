#include "../firmware/RPG_POKET_2/Rules.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace rpg;
void verify(const CombatTrace& t){
 assert(t.valid&&!t.overflow&&t.count<=t.maxSteps&&t.diceCount<=t.maxDice);int amount=0;bool dodge=false;unsigned dieCount=0;
 for(unsigned i=0;i<t.count;++i){auto s=t.steps[i];using K=TraceStep;switch(s.kind){
 case K::Weapon:amount=std::max(1,int(s.a)+s.b-1-s.c/3);break;
 case K::Critical:if(s.a<=s.b)amount+=amount/2;assert(amount==s.c);break;
 case K::Attacks:assert(amount==s.b);amount*=s.a;assert(amount==s.c);break;
 case K::Technique:assert(amount==s.b);amount=amount*s.a/100;assert(amount==s.c);break;
 case K::Rage:case K::Sacred:case K::Evocation:assert(amount==s.b);amount+=s.a;assert(amount==s.c);break;
 case K::Dice:{unsigned sides=s.c&255,group=unsigned(s.c)>>8;for(int j=0;j<s.b;++j){assert(unsigned(s.a+j)<t.diceCount);unsigned die=t.dice[s.a+j];assert(die>=1&&die<=sides);amount+=die;++dieCount;}if(group==unsigned(TraceDice::Missile))amount+=s.b;break;}
 case K::Half:case K::Resistance:assert(amount==s.a);amount/=2;assert(amount==s.b);break;
 case K::Fatigue:assert(amount==s.b&&amount>0&&(s.a==1||s.a==2));amount=std::max(1,amount-s.a);assert(amount==s.c);break;
 case K::Heavy:assert(amount==s.a);amount=amount*3/2;assert(amount==s.b);break;
 case K::Guard:assert(amount==s.b);amount=(amount*(100-s.a)+99)/100;assert(amount==s.c);break;
 case K::Dodge:dodge=s.a<=s.b;break;
 case K::Fixed:amount+=s.a;break;
 case K::Restore:amount=std::max(1,int(s.a)*s.b/100);assert(amount==s.c);break;
 default:break;
 }}
 assert(dieCount==t.diceCount);
 if(t.damaging){assert(t.net==(dodge?0:amount));assert(t.applied==std::min(t.targetHp,t.net));assert(t.potential>=t.net);}
 if(t.healing){assert(t.applied==std::min(t.targetHp,t.net));assert(t.potential==t.net);}
}
int main(){unsigned checked=0;for(unsigned cls=0;cls<4;++cls)for(unsigned level=1;level<=20;++level)for(unsigned seed=1;seed<=128;++seed)for(unsigned a=0;a<18;++a){
 auto g=create(cls,seed);g.p.xp=dndXp[level-1];levelUp(g);g.phase=Phase::Hero;g.enemyId=(seed+level)%29;g.enemyHp=(seed%3==0?1:enemySpec(g.enemyId).hp);g.p.life=g.p.mana=2;g.p.hp=std::max(1u,unsigned(g.p.maxhp)-seed%10-1);g.p.mp=g.p.maxmp-1;g.oath=1;g.sacredTurns=seed%2;g.rageTurns=seed%2;CombatTrace t;auto original=g;
 if(!act(g,Action(a),&t)){verify(t);++checked;if(t.damaging)assert(t.applied==original.enemyHp-g.enemyHp);}
 if(g.phase==Phase::Enemy){t=CombatTrace{};assert(enemy(g,&t));verify(t);++checked;}
 }
 auto g=create(0,1);g.p.xp=dndXp[2];levelUp(g);g.phase=Phase::Hero;g.enemyHp=17;CombatTrace fire;assert(!act(g,Action::BurningHands,&fire));assert(fire.dice[0]==4&&fire.dice[1]==2&&fire.dice[2]==4&&fire.potential==10&&fire.net==5&&fire.applied==5);verify(fire);
 printf("PASS: %u captured receipts independently reconstructed from actual dice, modifiers, crit/save/dodge, caps and resource changes; sizeof(trace)=%zu bytes\n",checked,sizeof(CombatTrace));
 return 0;
}
