#pragma once
#include "Bestiary.h"
template<class C>void drawBestiary(C& c,const rpg::Game& g,const ViewState& v){
 drawBackdrop(c,bg_character);scenicHeading(c,"BESTIARIO DO HEROI");char b[64];auto label=[&](int y,const char* s,uint16_t color=UI_WHITE){panelLabel(c,12,y,216,s,color);};unsigned n=rpg::bestiaryCount(g);snprintf(b,sizeof(b),"Criaturas registradas: %u",n);label(51,b,UI_GOLD);
 if(!n){label(128,"Seu bestiario esta vazio");label(158,"Encontre criaturas ao explorar");label(183,"O registro pertence a este heroi",UI_MUTED);}
 else {unsigned id=rpg::bestiaryKnown(g,v.bestiaryIndex)?v.bestiaryIndex:rpg::bestiaryFirst(g);const auto& foe=rpg::enemySpec(id);label(74,foe.name,UI_GOLD);scenicPanel(c,14,114,102,28);panelLabel(c,18,123,94,"< Anterior");scenicPanel(c,124,114,102,28);panelLabel(c,128,123,94,"Proximo >");
  enemyArt::drawBody(c,id,enemyArt::Pose::Idle,103,81,34,31);
  snprintf(b,sizeof(b),"HP base %u / ATQ %u / DEF %u",foe.hp,foe.atk,foe.def);label(158,b);label(181,rpg::physicalEnemy(id)?"Ataque fisico":"Ataque magico",UI_BLUE);label(204,rpg::undead(id)?"Morto-vivo / bonus divino":"Criatura / dano de arma");label(227,"Valores base; encontro pode variar",UI_MUTED);label(250,"Somente criaturas encontradas",UI_GREEN);
 }
 scenicPanel(c,14,278,212,32);label(289,"Voltar ao menu");
}
