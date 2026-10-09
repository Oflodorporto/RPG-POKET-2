#pragma once
template<class C>void drawLocks(C& c,const rpg::Game& g,const ViewState& v){
 backdropPeriod=menu.clockValid&&menu.dayCycle?menu.worldPeriod:worldClock::Period::Day;
 drawBackdrop(c,g.city==0?bg_explore0:g.city==1?bg_explore1:g.city==2?bg_explore2:bg_explore3);c.setTextWrap(false);
 auto label=[&](int y,const char* s,uint16_t color=UI_WHITE,int size=1){scenicPanel(c,4,y-5,232,8*size+14);c.setTextColor(color);c.setTextSize(size);c.setCursor((240-int(strlen(s))*6*size)/2,y+2);c.print(s);};
 auto button=[&](int y,const char* s,bool enabled=true,int height=28){scenicPanel(c,14,y,212,height);c.setTextSize(1);c.setTextColor(enabled?UI_GOLD:UI_MUTED);c.setCursor((240-int(strlen(s))*6)/2,y+(height-8)/2);c.print(s);};
 char b[64];bool shop=v.page==Page::Lockpicks;
 label(12,shop?"GAZUAS":"BAU TRANCADO",UI_GOLD,2);label(42,*v.message?v.message:placeName(g.city),*v.message?UI_RED:UI_WHITE);
 if(shop){c.fillRect(90,83,55,6,UI_GOLD);c.fillRect(141,83,6,32,UI_GOLD);c.fillRect(88,89,6,8,UI_GOLD);c.fillRect(90,101,42,5,UI_MUTED);c.fillRect(128,101,5,16,UI_MUTED);
  snprintf(b,sizeof(b),"Gazuas na bolsa: %u de 9",g.gazuas);label(140,b);label(166,"Cada tentativa usa 1 gazua.");label(192,"Destreza / sem proficiencia extra.");snprintf(b,sizeof(b),"Preco %u / ouro %lu",rpg::gazuaPrice(g),(unsigned long)g.p.gold);label(220,b,UI_GOLD);label(245,v.message,UI_RED);
  for(int i=0;i<2;++i){scenicPanel(c,14+i*110,270,102,40);c.setTextSize(1);c.setTextColor(UI_GOLD);c.setCursor(44+i*110,286);c.print(i?"Comprar":"Voltar");}return;
 }
 drawMimic(c,80,32,0,false);c.drawRect(113,72,14,15,UI_WHITE);c.drawRect(116,65,8,8,UI_WHITE);
 snprintf(b,sizeof(b),"CD %u / tentativas %u de 2",rpg::lockDifficulty(g),g.chestTries);label(108,b,UI_GOLD);
 snprintf(b,sizeof(b),"Forca %+d / chance %u%%",rpg::lockBonus(g,false),rpg::lockChance(g,false));label(132,b);snprintf(b,sizeof(b),"Destreza %+d / chance %u%%",rpg::lockBonus(g,true),rpg::lockChance(g,true));label(156,b);
 if(g.chestRoll){snprintf(b,sizeof(b),"D20 %u %+d = %d / %s",g.chestRoll,rpg::lockBonus(g,g.chestPick),int(g.chestRoll)+rpg::lockBonus(g,g.chestPick),g.chestLock==2?"ABERTO":g.chestLock==3?"TRAVOU":"FALHOU");label(180,b,g.chestLock==2?UI_GREEN:UI_RED);}
 else {snprintf(b,sizeof(b),g.chestTrap?"Fio suspeito: falha perde ate %u HP":"Sem armadilha visivel",rpg::trapCeiling(g));label(180,b,g.chestTrap?UI_RED:UI_WHITE);}
 snprintf(b,sizeof(b),"HP %u/%u / risco de %u HP",g.p.hp,g.p.maxhp,g.chestLock==1?rpg::trapCeiling(g):0);label(204,b,g.chestTrap&&g.chestLock==1?UI_RED:UI_WHITE);
 if(g.chestLock==2){button(224,"Fechadura aberta",false);button(256,"Recolher o conteudo");}
 else if(g.chestLock==3){button(224,"Fechadura inutilizada",false);button(256,"Sem novas tentativas",false);}
 else {button(224,"Forcar / falha inutiliza o bau");snprintf(b,sizeof(b),"Usar gazua / gasta 1 / tem %u",g.gazuas);button(256,b,g.gazuas>0);}
 button(288,"Deixar o bau",true,28);
}
