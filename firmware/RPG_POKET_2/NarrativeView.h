#pragma once
#include "Narrative.h"
template<class Canvas> void drawNarrative(Canvas& c,const rpg::Game& g,const ViewState& v){
 auto text=[&](int x,int y,const char* s,int size=1,uint16_t color=UI_WHITE){c.fillRect(x-2,y-2,int(strlen(s))*6*size+4,size*8+4,UI_INK);c.setTextSize(size);c.setTextColor(color);c.setCursor(x,y);c.print(s);};
 auto center=[&](int y,const char* s,int size=1,uint16_t color=UI_WHITE){text((240-int(strlen(s))*6*size)/2,y,s,size,color);};
 auto button=[&](int x,int y,int w,const char* s){c.fillRect(x,y,w,40,UI_PANEL);c.drawRect(x,y,w,40,UI_GOLD);text(x+(w-int(strlen(s))*6)/2,y+15,s);};
 char b[64];
 if(v.page==Page::Prologue){const auto& scene=story::opening[menu.storyIndex%4];drawBackdrop(c,menu.storyIndex==0?bg_village:menu.storyIndex==3?bg_guild:bg_tavern);
  center(15,"AS CINZAS DA",2,UI_GOLD);center(36,"PRIMEIRA AURORA",2,UI_GOLD);center(75,scene.speaker,1,UI_GOLD);
  for(unsigned i=0;i<4;++i)center(116+i*25,scene.lines[i]);snprintf(b,sizeof(b),"Aeldra / cena %u de 4",menu.storyIndex+1);center(231,b);
  button(14,272,102,game.tutorial?"Voltar":"Pular");button(124,272,102,menu.storyIndex==3?"Continuar":"Proxima");return;
 }
 drawBackdrop(c,v.page==Page::Continent?bg_world:v.page==Page::People||v.page==Page::Dialogue?(g.city==0?bg_village:g.city==1?bg_explore1:g.city==2?bg_port:bg_castle):bg_character);
 if(v.page==Page::Continent){const auto& r=story::regions[menu.regionIndex%8];center(12,"VALDARIA",2,UI_GOLD);center(49,"Continente / atlas");center(86,r.name,2,UI_GOLD);center(114,r.subtitle);center(150,r.route);
  center(178,menu.regionIndex?"Regiao futura / mapa em preparo":"Aeldra e a regiao onde voce esta");center(202,menu.regionIndex?"Sem viagem disponivel ainda":"Carvalho, Ruinas, Mares e Aurora");
  button(14,224,102,"Anterior");button(124,224,102,"Proxima");button(14,272,102,"Voltar");button(124,272,102,menu.regionIndex?"Em preparo":"Mapa Aeldra");return;
 }
 if(v.page==Page::People){center(12,"PESSOAS",2,UI_GOLD);center(45,placeName(g.city));
  for(unsigned i=0;i<3;++i){const auto& p=story::person(g.city,i);c.fillRect(14,78+i*54,212,48,UI_PANEL);c.drawRect(14,78+i*54,212,48,UI_GOLD);center(87+i*54,p.name,1,UI_GOLD);center(107+i*54,p.role);}
  center(249,"Toque no nome para conversar");button(14,272,212,"Voltar");return;
 }
 if(v.page==Page::Dialogue){const auto& p=story::person(g.city,menu.personIndex);center(14,p.name,1,UI_GOLD);center(41,p.role);
  if(g.city==1&&menu.personIndex==2&&!story::arconteKnown(g)){center(105,"Um eco permanece na Cripta.");center(133,"Encontre-o antes de ouvir sua voz.");}
  else for(unsigned i=0;i<4;++i)center(87+i*29,p.lines[i]);
  center(223,"As Cinzas da Primeira Aurora",1,UI_GOLD);button(14,272,212,"Voltar");return;
 }
 unsigned index=menu.chapterIndex%6;center(12,"DIARIO DE AELDRA",2,UI_GOLD);center(46,story::chapterNames[index],1,UI_GOLD);
 bool known=index<=story::knownChapter(g);const auto& chapter=story::chapters[index];center(73,known?chapter.speaker:"AINDA POR DESCOBRIR",1,known?UI_GREEN:UI_MUTED);
 if(known){for(unsigned i=0;i<4;++i)center(100+i*23,chapter.lines[i]);}
 else {center(113,"Continue a jornada em Aeldra.");center(143,"Esta pagina evita revelar");center(165,"o que voce ainda nao descobriu.");}
 if(index==2&&g.guardianDefeated)center(191,"Guardiao do Limiar: vencido",1,UI_GREEN);
 if(index==2&&story::arconteKnown(g))center(209,"Cripta de Vaelor: concluida",1,UI_GREEN);
 button(14,224,102,"Anterior");button(124,224,102,"Proxima");
 button(14,272,102,"Voltar");button(124,272,102,"Relembrar");
}
