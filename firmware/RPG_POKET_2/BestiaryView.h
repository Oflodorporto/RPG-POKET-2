#pragma once
#include "Bestiary.h"
template<class C>void drawBestiary(C& c,const rpg::Game& g,const ViewState& v){
 drawBackdrop(c,bg_character);scenicHeading(c,"BESTIARIO DO HEROI");char b[64];auto label=[&](int y,const char* s,uint16_t color=UI_WHITE){panelLabel(c,12,y,216,s,color);};unsigned n=rpg::bestiaryCount(g);snprintf(b,sizeof(b),"Criaturas registradas: %u",n);label(51,b,UI_GOLD);
 if(!n){label(128,"Seu bestiario esta vazio");label(158,"Encontre criaturas ao explorar");label(183,"O registro pertence a este heroi",UI_MUTED);}
 else {unsigned id=rpg::bestiaryKnown(g,v.bestiaryIndex)?v.bestiaryIndex:rpg::bestiaryFirst(g);const auto& foe=rpg::enemySpec(id);label(74,foe.name,UI_GOLD);scenicPanel(c,14,114,102,28);panelLabel(c,18,123,94,"< Anterior");scenicPanel(c,124,114,102,28);panelLabel(c,128,123,94,"Proximo >");
  if(id>=14){c.fillRect(102,85,36,23,0x8b26);c.drawRect(102,85,36,23,UI_GOLD);c.fillRect(104,92,32,8,UI_INK);for(int i=0;i<5;++i)c.fillRect(105+i*6,93,3,5,UI_WHITE);}else {const uint16_t* pixels=id>=10?hippogriffArt::frames[0]:id>=4?regionEnemyFrame(id,0):id==0?sprites_goblin[0]:id==1?sprites_wolf[0]:id==2?sprites_skeleton[0]:sprites_guardian[0];magicSprite(c,pixels,80,86,103,81,34,31);}
  snprintf(b,sizeof(b),"HP base %u / ATQ %u / DEF %u",foe.hp,foe.atk,foe.def);label(158,b);label(181,rpg::physicalEnemy(id)?"Ataque fisico":"Ataque magico",UI_BLUE);label(204,rpg::undead(id)?"Morto-vivo / bonus divino":"Criatura / dano de arma");label(227,"Valores base; encontro pode variar",UI_MUTED);label(250,"Somente criaturas encontradas",UI_GREEN);
 }
 scenicPanel(c,14,278,212,32);label(289,"Voltar ao menu");
}
