#pragma once
#include "CampArt.h"
#include "PersonalArt.h"
namespace campUi {
// Reuse the real hero's calm idle pose and its existing race/clothes masks.
// Fit the original camp envelopes without extra art or a render buffer.
template<class Canvas> void hero(Canvas& c,const rpg::Game& g,int x,int y,int w,int h){
  for(int py=0;py<h;++py)for(int px=0;px<w;++px){
    const auto color=personalPixel(g,0,(py*120/h)*112+px*112/w);
    if(color!=0xf81f)c.fillRect(x+px,y+py,1,1,color);
  }
}
}
template<class Canvas> void drawCamp(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame){
  panelCampBackdrop(c,g,v);
  c.setTextWrap(false);auto text=[&](int x,int y,const char* s,int size=1,uint16_t color=UI_WHITE){c.setTextSize(size);c.setTextColor(color);c.setCursor(x,y);c.print(s);};
  auto center=[&](int y,const char* s,int size=1,uint16_t color=UI_WHITE){text((240-int(strlen(s))*6*size)/2,y,s,size,color);};
  auto box=[&](int x,int y,int w,int h){c.fillRect(x,y,w,h,UI_PANEL);c.drawRect(x,y,w,h,UI_GOLD);};
  auto button=[&](int x,int y,int w,const char* s){box(x,y,w,40);text(x+(w-int(strlen(s))*12)/2,y+12,s,2);};char b[64];

  if(v.page==Page::CampRest){
    // Hide the concept's baked caption before drawing the actual rest choices.
    c.fillRect(14,81,212,17,0x0843);
    center(85,g.campRation?(g.campKit?"Carne, fogueira e saco de dormir":"Carne e uma fogueira quentinha"):g.campKit?"Sem comida, mas com saco de dormir":"Sem comida... dormir no chao",1,UI_GOLD);
    campUi::hero(c,g,18,114,84,96);
    magicSprite(c,campArt::fires[frame%4],48,48,116,156,64,64);
    if(g.campKit)magicSprite(c,campArt::sleepingbag,48,48,170,188,64,48);
    c.fillRect(14,236,212,77,0x0843);box(14,242,212,18);c.fillRect(17,245,206*std::min(1000u,v.campProgress)/1000,12,UI_GREEN);
    snprintf(b,sizeof(b),"Tempo passando... %u%%",std::min(1000u,v.campProgress)/10);center(269,b);
    snprintf(b,sizeof(b),"HP %u/%u / MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);center(295,b);return;
  }
  if(v.page==Page::CampRoll){
    d20Ui::draw(c,120,132,39,g.campRoll,menu.rollReady,uint32_t(menu.renderNow-menu.rollStarted));
    center(178,"D20",1,UI_GOLD);
    c.fillRect(25,184,190,81,0x0843);
    snprintf(b,sizeof(b),"Total %u / alvo %u",rpg::campTotal(g),rpg::campDifficulty(g));center(191,menu.rollReady?b:"Rolando...");
    center(206,menu.rollReady?(rpg::campSafe(g)?"LOCAL SEGURO":"EMBOSCADA!"):"Preparando o acampamento",1,menu.rollReady&&rpg::campSafe(g)?UI_GREEN:UI_GOLD);
    snprintf(b,sizeof(b),"Sorte +%u / Sobrevivencia +%u",rpg::luck(g),rpg::survival(g));center(226,b);
    snprintf(b,sizeof(b),"Kit +%u / D20 %s",g.campKit?2:0,menu.rollReady?"concluido":"em movimento");center(246,b);
    button(14,272,212,menu.rollReady?(rpg::campSafe(g)?"Descansar":"Enfrentar"):"Aguarde");return;
  }
  snprintf(b,sizeof(b),"D20 + atributos / alvo %u",rpg::campDifficulty(g));center(82,b);
  c.fillRect(14,93,212,12,0x0843);
  snprintf(b,sizeof(b),"Fadiga %u/100 / %s",g.fatigue,rpg::fatigueName(g));
  center(95,b,1,scenicFatigueColor(g));
  campUi::hero(c,g,38,108,58,66);
  magicSprite(c,campArt::fires[frame%4],48,48,121,136,42,42);
  if(menu.campKit)magicSprite(c,campArt::sleepingbag,48,48,170,149,43,29);
  c.fillRect(18,180,205,87,0x0843);
  center(183,menu.campRation?"Recupera 100% de HP e MP":"Recupera ate 50% de HP e MP");
  center(194,menu.campRation?"Fadiga: 0 apos descansar":"Fadiga: -50 apos descansar",1,UI_GREEN);
  box(14,207,212,33);snprintf(b,sizeof(b),"Racao: %s / tem %u",menu.campRation?"SIM":"NAO",g.rations);center(218,b,1,UI_GOLD);
  box(14,244,212,26);center(253,g.sleepKit?(menu.campKit?"Kit: SIM / +2 no teste":"Kit: NAO / dormir no chao"):"Kit: comprar por 80 ouro",1,UI_GOLD);
  if(*v.message){c.fillRect(3,169,234,12,UI_INK);center(171,v.message,1,UI_RED);}
  button(14,272,102,"Voltar");button(124,272,102,"Acampar");
}
