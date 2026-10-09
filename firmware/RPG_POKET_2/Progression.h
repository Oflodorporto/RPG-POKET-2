#pragma once
#include "Rules.h"
#include <stdio.h>
namespace rpg {
// Executable milestones only. SRD reference titles remain in ClassProgression.h.
struct LevelBenefits {unsigned count=0;char lines[6][40]{};void add(const char* s){if(count<6)snprintf(lines[count++],40,"%s",s);} };
inline LevelBenefits levelBenefits(const Game& g,unsigned level){
 LevelBenefits out;if(!g.dndProgression||level<1||level>20)return out;char b[40];
 for(unsigned a=unsigned(Action::MagicMissile);a<=unsigned(Action::DivineSmite);++a){auto action=Action(a);unsigned cls=action==Action::RagePower?3:action==Action::SecondWind||action==Action::ActionSurge?2:a>=unsigned(Action::LayHands)?1:0;if(cls==g.p.cls&&powerLevel(action)==level)out.add(powerName(action));}
 if(g.p.cls==0&&level==10)out.add("Evocacao: +mod INT no dano");
 if(g.p.cls==0&&level==18)out.add("Maestria: misseis e raios sem MP");
 if(g.p.cls==1&&level==2)out.add("Investida: tecnica de ataque");
 if(g.p.cls==1&&level==11)out.add("Punicao aprimorada: +1d8/golpe");
 if(g.p.cls==1&&level==3)out.add("Escolha o juramento nos Poderes");
 if(level==1)return out;
 Game before=g,after=g;before.p.level=level-1;after.p.level=level;
 unsigned points=(improvementCount(g.p.cls,level)-improvementCount(g.p.cls,level-1))*2;
 if(points){snprintf(b,sizeof(b),"+%u pontos / ou talento",points);out.add(b);}
 if(attacksPerAction(after)>attacksPerAction(before)){snprintf(b,sizeof(b),"Ataque: %u golpes por acao",attacksPerAction(after));out.add(b);}
 if(proficiency(level)>proficiency(level-1)){snprintf(b,sizeof(b),"Proficiencia +%u",proficiency(level));out.add(b);}
 if(g.p.cls==1){snprintf(b,sizeof(b),"Impor as maos: reserva %u HP",5*level);out.add(b);}
 if(g.p.cls==2&&level==3)out.add("Campeao: criticos em 20%");
 if(g.p.cls==2&&level==15)out.add("Critico superior: 30%");
 if(g.p.cls==2&&level==18)out.add("Sobrevivente: cura 5 + mod CON");
 if(g.p.cls==2&&level==17)out.add("Surto: 2 usos por descanso");
 if(g.p.cls==3){if(level==20)out.add("Furia: usos sem limite");else if(rageUses(after)>rageUses(before)&&level!=1){snprintf(b,sizeof(b),"Furia: %u usos por descanso",rageUses(after));out.add(b);}if(rageBonus(after)>rageBonus(before)){snprintf(b,sizeof(b),"Furia: +%u dano por golpe",rageBonus(after));out.add(b);}}
 if(survival(after)>survival(before)){snprintf(b,sizeof(b),"Sobrevivencia +%u",survival(after)-survival(before));out.add(b);}
 if(luck(after)>luck(before)){snprintf(b,sizeof(b),"Sorte +%u",luck(after)-luck(before));out.add(b);}
 return out;
}
inline unsigned nextBenefitLevel(const Game& g){if(!g.dndProgression)return 0;for(unsigned l=g.p.level+1;l<=20;++l)if(levelBenefits(g,l).count)return l;return 0;}
inline unsigned nextAttributeLevel(const Game& g){if(!g.dndProgression)return 0;for(unsigned l=g.p.level+1;l<=20;++l)if(improvementCount(g.p.cls,l)>improvementCount(g.p.cls,l-1))return l;return 0;}
inline const char* abilityEffect(unsigned i){static const char* text[]={"Ataque fisico e forcar trancas","Defesa e abrir com gazuas","HP por nivel e retroativo","Ataque e mana do Mago","Sobrevivencia na viagem","Mana e Arma sagrada do Paladino","+2 HP por nivel, inclusive antigos"};return text[std::min(6u,i)];}
}
