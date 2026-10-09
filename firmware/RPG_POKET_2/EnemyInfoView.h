#pragma once
#include "EnemyInfo.h"
template<class C>void drawEnemyInfo(C& c,const rpg::Game& g,const ViewState&){
 drawBackdrop(c,bg_character);scenicHeading(c,"CONHECA O INIMIGO");char b[64];
 auto label=[&](int y,const char* s,uint16_t color=UI_WHITE){panelLabel(c,12,y,216,s,color);};
 if(!rpg::canInspect(g)){label(130,"Ficha disponivel no seu turno",UI_MUTED);scenicPanel(c,14,278,212,32);label(289,"Voltar");return;}
 const auto& foe=rpg::enemySpec(g.enemyId);label(51,foe.name,UI_GOLD);
 const uint16_t* pixels=g.enemyId>=10?hippogriffArt::frames[0]:g.enemyId>=4?regionEnemyFrame(g.enemyId,0):g.enemyId==0?sprites_goblin[0]:g.enemyId==1?sprites_wolf[0]:g.enemyId==2?sprites_skeleton[0]:sprites_guardian[0];
 if(g.enemyId>=18)finaleFoe(c,g.enemyId,97,65,0);else if(g.enemyId>=14)drawMimic(c,82,42,0,true);else magicSprite(c,pixels,80,86,97,65,46,49);
 snprintf(b,sizeof(b),"HP %u/%u / ATQ %u / DEF %u",g.enemyHp,rpg::encounterHp(g,g.enemyId),rpg::encounterAttack(g),foe.def);label(119,b);
 label(138,rpg::physicalEnemy(g.enemyId)?"Ataque fisico":"Ataque magico",UI_BLUE);label(156,rpg::undead(g.enemyId)?"Natureza: morto-vivo":"Natureza: criatura",UI_MUTED);
 label(179,rpg::intentName(g),UI_GOLD);snprintf(b,sizeof(b),"Proximo golpe: ate %u HP",rpg::incomingCeiling(g,g.guard));label(197,b,UI_RED);
 label(218,rpg::enemyAdvice(g,0));label(235,rpg::enemyAdvice(g,1));label(252,rpg::enemyAdvice(g,2));scenicPanel(c,14,278,212,32);label(289,"Voltar a batalha");
}

