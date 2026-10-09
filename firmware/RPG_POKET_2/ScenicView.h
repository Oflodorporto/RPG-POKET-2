#pragma once
#include "ScenicArt.h"
// Geometry is shared with the controller. Drawing is read-only and SD-free.
namespace scenicUi {
constexpr int menuX=63,menuWidth=115,menuHeight=25;
constexpr int menuYs[7]={78,107,135,164,192,221,249};
constexpr int homeTop=274,homeHeight=40,homeWidth=72;
inline int menuChoice(int x,int y){if(x<menuX-3||x>=menuX+menuWidth+3)return -1;for(int n=0;n<7;++n)if(y>=menuYs[n]-1&&y<menuYs[n]+menuHeight+2)return n;return -1;}
inline int homeChoice(int x,int y){if(y<homeTop||y>=homeTop+homeHeight)return -1;for(int n=0;n<3;++n)if(x>=6+n*78&&x<6+n*78+homeWidth)return n;return -1;}
struct Rect {int x,y,w,h;bool contains(int px,int py)const{return px>=x&&px<x+w&&py>=y&&py<y+h;}};
// Conversation, crystal entrance, guardian, explore, camp, map, market.
constexpr Rect ruinsButtons[]={{69,57,110,16},{108,143,128,63},{105,210,130,30},{52,246,86,29},{144,246,90,29},{52,278,86,30},{144,278,90,30}};
constexpr Rect mapButtons[]={{23,278,96,30},{122,278,97,30}};
inline int ruinsChoice(int x,int y){for(int i=0;i<7;++i)if(ruinsButtons[i].contains(x,y))return i;return -1;}
inline bool atlasChoice(int x,int y){return x>=30&&x<75&&y>=65&&y<95;}
}
template<class C>void scenicText(C& c,int x,int y,const char* s,int size=1,uint16_t color=UI_WHITE){c.setTextWrap(false);c.setTextSize(size);c.setTextColor(color);c.setCursor(x,y);c.print(s);}
template<class C>void scenicPanel(C& c,int x,int y,int w,int h){
 c.fillRect(x,y,w,h,0x0843);c.fillRect(x+3,y+3,w-6,h-6,0x10c6);
 c.drawRect(x,y,w,h,UI_GOLD);c.drawRect(x+2,y+2,w-4,h-4,0x8bcb);c.fillRect(x+4,y+4,w-8,1,0x52ac);
 for(int dx:{0,w-4})for(int dy:{0,h-4})c.fillRect(x+dx,y+dy,4,4,0xff13);
}
template<class C>void scenicHeading(C& c,const char* s){
 scenicPanel(c,10,10,220,34);int size=strlen(s)*12<=208?2:1;
 scenicText(c,(240-int(strlen(s))*6*size)/2,10+(34-size*8)/2,s,size,UI_GOLD);
 c.fillRect(117,6,6,4,UI_GOLD);c.fillRect(119,5,2,6,UI_BLUE);c.fillRect(117,44,6,3,UI_GOLD);
}
template<class C>void scenicBackdrop(C& c,bool isMenu,unsigned frame,worldClock::Period period){
 uint16_t row[240],palette[256];auto raw=isMenu?scenicArt::menuPalette:scenicArt::shelterPalette;auto pixels=isMenu?scenicArt::menuPixels:scenicArt::shelterPixels;
 for(int i=0;i<256;++i)palette[i]=isMenu?raw[i]:worldClock::shade(raw[i],period);
 for(int y=0;y<320;++y){for(int x=0;x<240;++x)row[x]=(!isMenu&&y<78?raw:palette)[pixels[y*240+x]];c.draw16bitRGBBitmap(0,y,row,240,1);}
 // Small warm flicker stays inside the illustrated lantern/fire, with no SD polling.
 unsigned beat=(frame/4)%8;uint16_t warm=beat<4?0xff8d:0xfd83;
 if(frame){c.fillRect(isMenu?40:64,isMenu?183:173,2,4,warm);
 if(!isMenu){c.fillRect(128+int(beat%3)-1,237-int(beat%3),2,8,warm);for(unsigned i=0;i<3;++i){unsigned age=(frame/3+i*9)%24;c.fillRect(127+int(i*4),239-int(age),1,1,age<12?0xffab:0xaac4);}}
 else {if((frame/10)%2)c.fillRect(199,12,1,2,UI_WHITE);}
 }
}
template<class C>void scenicIcon(C& c,int x,int y,int id){
 const uint16_t gold=0xeeb3;
 if(id==6){c.fillRect(x+2,y+3,3,10,gold);c.fillRect(x,y+6,10,3,gold);c.fillRect(x+8,y+8,3,7,gold);c.fillRect(x+2,y+13,7,3,gold);return;}
 if(id==1||id==4){c.fillRect(x+4,y+2,6,5,gold);c.fillRect(x+2,y+7,10,7,gold);c.fillRect(x+1,y+9,2,5,gold);c.fillRect(x+11,y+9,2,5,gold);c.fillRect(x+3,y+14,3,3,gold);c.fillRect(x+8,y+14,3,3,gold);return;}
 c.fillRect(x+1,y+2,12,15,gold);c.drawRect(x+2,y+3,10,13,0x8326);
 if(id==0){c.fillRect(x+5,y+4,2,3,0x8326);c.fillRect(x+7,y+7,2,5,0x8326);c.fillRect(x+4,y+11,4,2,0x8326);}
 else if(id==5){c.fillRect(x+4,y+5,7,2,0x8326);c.fillRect(x+4,y+8,7,2,0x8326);c.fillRect(x+4,y+11,5,2,0x8326);}
 else {c.fillRect(x+6,y+3,1,13,0x8326);c.fillRect(x+3,y+6,2,1,0x8326);c.fillRect(x+8,y+6,3,1,0x8326);}
}
template<class C>void drawScenicMenu(C& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 scenicBackdrop(c,true,frame,worldClock::Period::Night);
 // Each button is an actual raster crop of the concept, with shared live hit regions.
 uint16_t row[115];for(int i=0;i<7;++i){int y=scenicUi::menuYs[i];for(int dy=0;dy<25;++dy){for(int dx=0;dx<115;++dx)row[dx]=scenicArt::menuPalette[scenicArt::menuPixels[(y+dy)*240+63+dx]];c.draw16bitRGBBitmap(63,y+dy,row,115,1);}if(i==5&&g.eventStage==1&&frame%12<6)c.fillRect(172,y+10,3,3,UI_GREEN);}
 scenicPanel(c,14,281,102,28);scenicText(c,40,290,"Guia >",1,UI_GOLD);scenicPanel(c,124,281,102,28);scenicText(c,128,290,"Bestiario >",1,UI_GOLD);
 if(*v.message){c.fillRect(2,310,236,10,UI_INK);scenicText(c,(240-int(strlen(v.message))*6)/2,311,v.message,1,UI_RED);}
}
template<class C>void drawScenicHome(C& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 auto period=menu.clockValid&&menu.dayCycle?menu.worldPeriod:worldClock::Period::Day;
 // The ruined shelter belongs to Vespera. Other settlements keep their own art.
 if(g.city==1)scenicBackdrop(c,false,frame,period);else {backdropPeriod=period;drawBackdrop(c,g.city==0?bg_camp:g.city==2?bg_port:bg_castle);}
 if(g.city!=1)scenicHeading(c,g.city==0?"REFUGIO DAS BRASAS":"ABRIGO DE VIAGEM");
 char b[64];const char* place=g.city==0?"Nara Veld / Carvalho":placeName(g.city);
 if(g.city!=1){scenicPanel(c,36,49,168,18);scenicText(c,(240-int(strlen(place))*6)/2,54,place,1,UI_GOLD);}
 scenicPanel(c,10,84,220,48);snprintf(b,sizeof(b),"%s / Nv %u / %lu ouro",rpg::className(g.p.cls),g.p.level,(unsigned long)g.p.gold);scenicText(c,17,92,b);
 snprintf(b,sizeof(b),"HP %u/%u  MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);scenicText(c,17,105,b);
 snprintf(b,sizeof(b),"XP %lu/%u",(unsigned long)g.p.xp,rpg::xpNeeded(g));scenicText(c,17,119,b,1,UI_MUTED);
 if(*v.message){scenicPanel(c,10,251,220,17);scenicText(c,(240-int(strlen(v.message))*6)/2,255,v.message,1,UI_GREEN);}
 scenicPanel(c,10,140,220,30);auto goal=story::objective(g);scenicText(c,17,146,goal.title,1,UI_GOLD);scenicText(c,17,158,"Toque: proximo objetivo");
 const char* names[]={"Mapa","Descanso","Menu"};for(int i=0;i<3;++i){int x=6+i*78;scenicPanel(c,x,274,72,40);scenicText(c,x+(72-int(strlen(names[i]))*6)/2,290,names[i]);}
}
template<class C>void scenicWorld(C& c,bool ruins,worldClock::Period period){
 const uint16_t* raw=ruins?scenicArt::ruinsPalette:scenicArt::mapPalette;const uint8_t* pixels=ruins?scenicArt::ruinsPixels:scenicArt::mapPixels;
 uint16_t palette[256],row[240];for(int i=0;i<256;++i)palette[i]=worldClock::shade(raw[i],period);
 for(int y=0;y<320;++y){bool ui=ruins?(y<76||y>=142):(y<50||y>=242);for(int x=0;x<240;++x){uint16_t color=(ui?raw:palette)[pixels[y*240+x]];
  // Normalize the concept's green Ruins pin; live presence is drawn separately.
  if(!ruins&&x>=112&&x<=132&&y>=165&&y<=186){uint16_t original=raw[pixels[y*240+x]];int r=(original>>11)&31,g=(original>>5)&63,b=original&31;if(g>r*2+12&&g>b*2+12)color=worldClock::shade(UI_BLUE,period);}
  row[x]=color;}c.draw16bitRGBBitmap(0,y,row,240,1);}
}
template<class C>void drawScenicMap(C& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 auto period=menu.clockValid&&menu.dayCycle?menu.worldPeriod:worldClock::Period::Day;scenicWorld(c,false,period);
 if(rpg::islandUnlocked(g)){magicSprite(c,islandArt::map_island(),32,32,16,136,32,32);c.drawRect(14,134,36,36,(frame&1)?UI_GOLD:UI_BLUE);scenicText(c,20,174,"ILHA",1,UI_GOLD);}
 const auto& selected=places[menu.destination<4?menu.destination:0];c.drawRect(selected.x-8,selected.y-8,17,17,UI_GOLD);c.drawRect(selected.x-10,selected.y-10,21,21,UI_INK);
 const auto& here=places[g.city<4?g.city:0];c.fillRect(here.x-2,here.y-2,5,5,UI_GREEN);
 c.fillRect(55,246,132,25,0x0843);char b[64];snprintf(b,sizeof(b),"Destino Nv %u+ / CD %u",rpg::cityLevel(menu.destination),rpg::routeDifficulty(g.city,menu.destination));scenicText(c,(240-int(strlen(b))*6)/2,249,b,1,g.p.level<rpg::cityLevel(menu.destination)?UI_RED:UI_WHITE);
 snprintf(b,sizeof(b),"Farol: %s",menu.destination==0?"Raiz":menu.destination==1?"Memoria":menu.destination==2?"Caminho":"Juramento");scenicText(c,(240-int(strlen(b))*6)/2,262,b,1,UI_GOLD);
 if(*v.message){c.fillRect(2,229,236,12,UI_INK);scenicText(c,(240-int(strlen(v.message))*6)/2,232,v.message,1,UI_RED);}
 (void)frame;
}
template<class C>void drawScenicRuins(C& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 auto period=menu.clockValid&&menu.dayCycle?menu.worldPeriod:worldClock::Period::Day;scenicWorld(c,true,period);
 scenicPanel(c,10,94,220,30);auto goal=story::objective(g);scenicText(c,17,100,goal.title,1,UI_GOLD);scenicText(c,17,112,"Toque: proximo objetivo");
 // Never display the sample 3/3 or "vencido" from the concept as real progress.
 c.fillRect(160,178,73,24,0x0843);char b[64];snprintf(b,sizeof(b),"Vitorias %u/3",g.ruinsWins);scenicText(c,160,180,b,1,UI_GOLD);
 scenicText(c,166,193,g.guardianDefeated?"Vencido!":g.ruinsWins>=3?"Liberado":"Bloqueado",1,g.guardianDefeated?UI_GREEN:g.ruinsWins>=3?UI_GOLD:UI_MUTED);
 if(g.ruinsWins<3){c.fillRect(111,213,119,12,0x0843);scenicText(c,116,216,"Guardiao bloqueado",1,UI_MUTED);}
 if(*v.message){c.fillRect(2,78,236,12,UI_INK);scenicText(c,(240-int(strlen(v.message))*6)/2,81,v.message,1,UI_RED);}
 if(g.questId){c.fillRect(3,128,104,12,UI_INK);snprintf(b,sizeof(b),"Missao %u/%u",g.questProgress,rpg::contract(g.questId).count);scenicText(c,7,131,b,1,UI_GREEN);}
 if(frame){c.fillRect(58+int((frame/4)%2),204,2,7,0xff8d);unsigned age=(frame/3)%22;c.fillRect(65,212-int(age),1,1,0xfdc4);}
}
