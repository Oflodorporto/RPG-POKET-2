#pragma once
#include "Rules.h"
namespace rpg {
inline bool canInspect(const Game& g){return g.phase==Phase::Hero&&g.enemyId<29&&g.enemyHp;}
inline const char* enemyAdvice(const Game& g,unsigned line){
 if(line==0){switch(enemyIntent(g)){case Intent::Prepare:return "Preparacao: inimigo nao ataca";case Intent::Heavy:return "Golpe forte: considere defesa";case Intent::Mend:return "Recompoe ate 4 HP neste turno";case Intent::Drain:return "Drena HP e ate 2 MP";default:return "Observe a intencao a cada turno";}}
 if(line==1)return physicalEnemy(g.enemyId)?"Furia reduz dano fisico a metade":"Furia nao reduz dano magico";
 if(undead(g.enemyId))return "Alvo de Expulsar e bonus divino";
 return "Defesa reduz dano de armas";
}
}
