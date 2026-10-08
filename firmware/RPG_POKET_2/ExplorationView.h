#pragma once
// Code-native chest creature: separate silhouette and animated jaw, no SD I/O.
template<class C>void drawMimic(C& c,int x,int y,unsigned frame,bool alive){
 int jaw=alive?int(frame%4)*3:0;
 c.fillRect(x+5,y+43,66,28,0x8b26);c.drawRect(x+5,y+43,66,28,UI_GOLD);
 c.fillRect(x+5,y+26-jaw,66,16,0xa409);c.drawRect(x+5,y+26-jaw,66,16,UI_GOLD);
 for(int i=0;i<3;++i)c.fillRect(x+9,y+48+i*7,58,1,0x51c2);
 for(int i=0;i<4;++i)c.fillRect(x+10+i*14,y+29-jaw,1,10,0x51c2);
 for(int strap:{15,55})c.fillRect(x+strap,y+43,5,27,UI_GOLD);
 if(alive){c.fillRect(x+10,y+39-jaw,56,8+jaw,UI_INK);for(int i=0;i<7;++i){c.fillRect(x+12+i*8,y+40-jaw,3,6,UI_WHITE);c.fillRect(x+16+i*8,y+43,3,5,UI_WHITE);}c.fillRect(x+27,y+25-jaw,6,5,UI_RED);c.fillRect(x+48,y+25-jaw,6,5,UI_RED);c.fillRect(x+35,y+48,10,14,UI_RED);}
 else {c.fillRect(x+34,y+39,9,14,UI_GOLD);c.fillRect(x+37,y+44,3,5,UI_INK);}
}
template<class C>void drawDiscovery(C& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 backdropPeriod=menu.clockValid&&menu.dayCycle?menu.worldPeriod:worldClock::Period::Day;
 auto bg=g.city==0?bg_explore0:g.city==1?bg_explore1:g.city==2?bg_explore2:bg_explore3;drawBackdrop(c,bg);c.setTextWrap(false);
 auto label=[&](int y,const char* s,uint16_t color=UI_WHITE,int size=1){scenicPanel(c,10,y-4,220,8*size+10);c.setTextColor(color);c.setTextSize(size);c.setCursor((240-int(strlen(s))*6*size)/2,y);c.print(s);};
 auto button=[&](int x,int y,int w,const char* s){scenicPanel(c,x,y,w,40);c.setTextSize(1);c.setTextColor(UI_WHITE);c.setCursor(x+(w-int(strlen(s))*6)/2,y+16);c.print(s);};
 char b[64];bool scrap=v.page==Page::Scrap;
 label(14,scrap?"SUCATA DA BOLSA":rpg::discoveryName(g),UI_GOLD,2);
 label(47,placeName(g.city));
 if(scrap){drawMimic(c,80,70,0,false);snprintf(b,sizeof(b),"Bota / roupa velha: %u de 9",g.scrap);label(159,b);snprintf(b,sizeof(b),"Vender tudo: %u ouro",g.scrap*2);label(188,b,UI_GOLD);button(14,216,212,"Descartar tudo");button(14,270,102,"Voltar");button(124,270,102,"Vender");}
 else {
  if(g.discovery==2)drawMimic(c,80,75,frame,false);
  else if(g.discoverLoot==0){for(int i=0;i<5;++i){c.fillRect(85+i*12,110-i%2*8,11,10,UI_GOLD);c.drawRect(85+i*12,110-i%2*8,11,10,0xffe0);}}
  else if(g.discoverLoot==3){c.fillRect(92,96,56,35,0xc4eb);c.drawRect(92,96,56,35,UI_GOLD);c.fillRect(116,96,7,35,0x51c2);c.fillRect(92,109,56,6,0x51c2);}
  else if(g.discoverLoot==4){c.fillRect(97,93,20,25,0x8b26);c.fillRect(97,115,47,15,0x8b26);c.fillRect(95,130,51,5,0x51c2);for(int i=0;i<3;++i)c.fillRect(100,98+i*6,13,2,UI_GOLD);}
  else if(g.discoverLoot==5){c.fillRect(100,95,40,36,0x9cf4);c.fillRect(91,100,9,17,0x9cf4);c.fillRect(140,100,9,17,0x9cf4);c.fillRect(112,95,16,8,UI_INK);c.fillRect(105,123,5,8,UI_INK);c.fillRect(127,126,7,5,UI_INK);}
  else if(g.discoverLoot==6){magicSprite(c,gear_icons[rpg::gearFamily(g.discoverAmount)],56,56,88,85,64,64);}
  else {unsigned prop=g.discoverLoot==1?6:7;magicSprite(c,dungeonArt::props[prop],32,32,88,85,64,64);}
  if(g.discovery==2){label(166,"Bau antigo. Pode conter surpresas.");label(191,g.discoverLoot==7?"Voce ouve algo se mexer dentro...":"A tampa esta solta. Vai abrir?",UI_GOLD);}
  else {const char* local[]={"Um viajante divide seus mantimentos.","Uma expedicao deixou provisoes.","Um marinheiro agradece sua ajuda.","Um vigia oferece suas provisoes."};label(166,g.discovery==3?local[g.city]:"Voce encontrou algo no caminho.");
   snprintf(b,sizeof(b),"%s x%u",g.discoverLoot==6?rpg::gearName(g.discoverAmount):rpg::lootName(g.discoverLoot),g.discoverLoot==6?1:g.discoverAmount);label(191,b,UI_GOLD);}
  label(235,v.message,UI_RED);
  if(g.discovery==5)button(14,270,212,"Continuar");else {button(14,270,102,"Deixar");button(124,270,102,g.discovery==2?"Abrir":"Recolher");}
 }
 if(scrap&&*v.message)label(247,v.message,UI_GREEN);
}
