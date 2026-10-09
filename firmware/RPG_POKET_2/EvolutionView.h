#pragma once
#include "ClassProgression.h"
template<class C>void drawEvolution(C& c,const rpg::Game& g,const ViewState& v){
 drawBackdrop(c,bg_character);scenicHeading(c,"ATRIBUTOS");char b[64];unsigned level=std::min(20u,std::max(1u,unsigned(g.p.level)));
 snprintf(b,sizeof(b),"%s / nivel %u",rpg::className(g.p.cls),level);panelLabel(c,10,50,220,b,UI_GOLD);
 snprintf(b,sizeof(b),"XP total %lu / proficiencia +%u",(unsigned long)rpg::dndXp[level-1],rpg::proficiency(level));panelLabel(c,10,65,220,b);
 unsigned next=rpg::nextAttributeLevel(g);if(next)snprintf(b,sizeof(b),"Proximo marco Nv %u: +2 pontos",next);else snprintf(b,sizeof(b),g.dndProgression?"Todos os marcos foram alcancados":"Heroi antigo: regras preservadas");panelLabel(c,10,80,220,b,UI_GOLD);
 for(int i=0;i<2;++i){scenicPanel(c,14+i*110,92,102,28);panelLabel(c,18+i*110,101,94,i?"Poderes >":"Trilha >");}
 panelLabel(c,10,124,220,g.dndProgression?"Toque no atributo para ver a previa":"Heroi antigo: regras preservadas",UI_MUTED);
 snprintf(b,sizeof(b),"Pontos disponiveis: %u",rpg::advancementPoints(g));panelLabel(c,10,139,220,b,UI_GREEN);
 for(unsigned i=0;i<6;++i){int x=14+int(i%2)*110,y=156+int(i/2)*34;scenicPanel(c,x,y,102,30);snprintf(b,sizeof(b),"%s %u",rpg::abilityName(i),g.attributes[i]);panelLabel(c,x+4,y+10,94,b,g.dndProgression?UI_WHITE:UI_MUTED);}
 panelLabel(c,10,258,220,*v.message?v.message:"Toque: +1 / Resistente custa 2",UI_GOLD);
 scenicPanel(c,14,272,102,40);panelLabel(c,20,287,90,"Voltar");scenicPanel(c,124,272,102,40);panelLabel(c,128,282,94,"Resistente",g.tough?UI_GREEN:UI_GOLD);panelLabel(c,128,297,94,g.tough?"Aprendido":"+2 HP / nivel");
}
