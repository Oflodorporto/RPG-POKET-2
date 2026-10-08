#pragma once
#include "CampArt.h"
template<class Canvas> void drawCamp(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame){
  panelCampBackdrop(c,g,v);
  c.setTextWrap(false);auto text=[&](int x,int y,const char* s,int size=1,uint16_t color=UI_WHITE){c.fillRect(x-2,y-2,int(strlen(s))*6*size+4,8*size+4,UI_INK);c.setTextSize(size);c.setTextColor(color);c.setCursor(x,y);c.print(s);};
  auto center=[&](int y,const char* s,int size=1,uint16_t color=UI_WHITE){text((240-int(strlen(s))*6*size)/2,y,s,size,color);};
  auto box=[&](int x,int y,int w,int h){c.fillRect(x,y,w,h,UI_PANEL);c.drawRect(x,y,w,h,UI_GOLD);};
  auto button=[&](int x,int y,int w,const char* s){box(x,y,w,40);text(x+(w-int(strlen(s))*12)/2,y+12,s,2);};char b[64];

  if(v.page==Page::CampRest){
    center(85,g.campRation?(g.campKit?"Carne, fogueira e saco de dormir":"Carne e uma fogueira quentinha"):g.campKit?"Sem comida, mas com saco de dormir":"Sem comida... dormir no chao",1,UI_GOLD);
    magicSprite(c,campArt::heroes[g.p.cls][g.campRation?1:0][frame%2],56,64,18,114,84,96);
    magicSprite(c,campArt::fires[frame%4],48,48,116,156,64,64);
    if(g.campKit)magicSprite(c,campArt::sleepingbag,48,48,170,188,64,48);
    box(14,242,212,18);c.fillRect(17,245,206*std::min(1000u,v.campProgress)/1000,12,UI_GREEN);
    snprintf(b,sizeof(b),"Tempo passando... %u%%",std::min(1000u,v.campProgress)/10);center(269,b);
    snprintf(b,sizeof(b),"HP %u/%u / MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);center(295,b);return;
  }
  if(v.page==Page::CampRoll){
    unsigned value=menu.rollReady?g.campRoll:1+(frame*7)%20;
    snprintf(b,sizeof(b),"%u",value);box(99,119,42,38);center(127,b,3,UI_GOLD);center(174,"D20",1);
    c.fillRect(25,184,190,81,0x0843);
    snprintf(b,sizeof(b),"Total %u / alvo %u",rpg::campTotal(g),rpg::campDifficulty(g));center(191,menu.rollReady?b:"Rolando...");
    center(206,menu.rollReady?(rpg::campSafe(g)?"LOCAL SEGURO":"EMBOSCADA!"):"Preparando o acampamento",1,menu.rollReady&&rpg::campSafe(g)?UI_GREEN:UI_GOLD);
    snprintf(b,sizeof(b),"Sorte +%u / Sobrevivencia +%u",rpg::luck(g),rpg::survival(g));center(226,b);
    snprintf(b,sizeof(b),"Kit +%u / D20 %s",g.campKit?2:0,menu.rollReady?"concluido":"em movimento");center(246,b);
    button(14,272,212,menu.rollReady?(rpg::campSafe(g)?"Descansar":"Enfrentar"):"Aguarde");return;
  }
  snprintf(b,sizeof(b),"D20 + atributos / alvo %u",rpg::campDifficulty(g));center(82,b);
  box(14,100,212,40);snprintf(b,sizeof(b),"Racao: %s / tem %u",menu.campRation?"SIM":"NAO",g.rations);center(115,b,1,UI_GOLD);
  box(14,152,212,40);center(167,g.sleepKit?(menu.campKit?"Kit: SIM / +2 no teste":"Kit: NAO / dormir no chao"):"Kit: comprar por 80 ouro",1,UI_GOLD);
  center(210,menu.campRation?"Recupera 100% de HP e MP":"Recupera ate 50% dos maximos");center(227,"Falha: lutar antes de descansar");center(247,v.message,1,UI_RED);
  button(14,272,102,"Voltar");button(124,272,102,"Acampar");
}
