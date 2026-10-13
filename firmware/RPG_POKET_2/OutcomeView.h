#pragma once
#include "FallenArt.h"
namespace outcomeUi {
constexpr scenicUi::Rect lossContinue={14,180,212,40},lossDungeonMenu={14,180,102,40},lossDungeonContinue={124,180,102,40};
constexpr scenicUi::Rect attributes={14,272,102,40},levelContinue={124,272,102,40};
inline bool lossPage(Page p,const rpg::Game& g){return g.phase==rpg::Phase::Lost&&(p==Page::Result||p==Page::CampaignResult||p==Page::EventResult||p==Page::Dungeon);}
inline bool canImprove(const rpg::Game& g){return rpg::advancementPoints(g)&&g.phase==rpg::Phase::Home&&!g.tripStage&&!g.campStage&&!g.dungeonFlags;}
template<class C>void defeatTitle(C& c,int dx,int dy,uint16_t color){
 // The bundled legacy font's circumflex glyph is wrong. Draw this one glyph locally.
 constexpr uint8_t circumflexE[5]={56,86,85,86,24};
 const int left=71+dx,y=35+dy;c.setTextWrap(false);c.setTextSize(2);c.setTextColor(color);
 for(int stroke:{0,1}){c.setCursor(left+stroke,y);c.print("Voc");for(unsigned col=0;col<5;++col)for(unsigned row=0;row<8;++row)if(circumflexE[col]&(1<<row))c.fillRect(left+36+col*2+stroke,y+row*2,2,2,color);c.setCursor(left+48+stroke,y);c.print(" foi");}
 touchLabel(c,14+dx,62+dy,212,"derrotado",color,2);
}
}
template<class C>void drawDefeat(C& c,const rpg::Game& g,const ViewState& v){
 bool ready=menu.art==ArtStatus::Ready;if(ready)sceneArt::draw(c,rpg::escortTrip(g)?sceneArt::Id::defeat_escort:sceneArt::Id::defeat_solo);else c.fillScreen(UI_INK);
 for(int x:{-1,0,1})for(int y:{-1,0,1})if(x||y)outcomeUi::defeatTitle(c,x,y,UI_INK);outcomeUi::defeatTitle(c,0,0,0xf964);
 carvalhoUi::frame(c,{16,91,208,82});char b[64];touchLabel(c,20,100,200,"Retorno com 1 HP",UI_GOLD);
 snprintf(b,sizeof(b),"Penalidade: %u XP",g.gainXp);touchLabel(c,20,117,200,b);
 snprintf(b,sizeof(b),"Nv %u / XP %lu/%u",g.p.level,(unsigned long)g.p.xp,rpg::xpNeeded(g));touchLabel(c,20,134,200,b);
 touchLabel(c,20,153,200,"Progresso salvo.",UI_GREEN);
 if(v.page==Page::Dungeon){for(auto r:{outcomeUi::lossDungeonMenu,outcomeUi::lossDungeonContinue})carvalhoUi::frame(c,r);touchLabel(c,18,196,94,"Menu",UI_GOLD);touchLabel(c,128,196,94,"Continuar",UI_GOLD);}
 else {carvalhoUi::frame(c,outcomeUi::lossContinue);touchLabel(c,18,196,204,"Recuperar forcas",UI_GOLD);}
 if(ready){fallenArt::draw(c,g,40,224);c.fillRect(58,284,5,2,0x7800);c.fillRect(67,290,3,1,0x6800);}
}
template<class C>void drawLevelNotice(C& c,const rpg::Game& g,const ViewState&,unsigned frame){
 bool ready=menu.art==ArtStatus::Ready;if(ready)sceneArt::draw(c,sceneArt::Id::levelup);else c.fillScreen(UI_INK);
 unsigned t=frame%20,pulse=t<=10?t:20-t;unsigned red=60+4*pulse,green=165+9*pulse,blue=70+6*pulse;uint16_t glow=uint16_t(((red>>3)<<11)|((green>>2)<<5)|(blue>>3));
 for(int x:{-2,0,2})for(int y:{-2,0,2})if(x||y){touchLabel(c,22+x,31+y,196,"Subiu de",UI_INK,2);touchLabel(c,22+x,58+y,196,"n\xa1" "vel",UI_INK,2);}
 touchLabel(c,22,31,196,"Subiu de",glow,2);touchLabel(c,22,58,196,"n\xa1" "vel",glow,2);
 carvalhoUi::frame(c,{72,86,96,20});char b[48];snprintf(b,sizeof(b),"Nivel %u",g.p.level);touchLabel(c,76,92,88,b,UI_GOLD);
 if(ready)personalSprite(c,g,64,112,0,false);
 for(unsigned i=0;i<10;++i){int x=43+(i*47)%156,y=219-((i*23+(frame%20)*2)%115);if(x>=68&&x<=172&&y>=114)continue;uint16_t color=i%2?glow:UI_GOLD;c.fillRect(x,y,2,2,color);if(i%3==0){c.fillRect(x-1,y+1,4,1,color);c.fillRect(x+1,y-1,1,4,color);}}
 carvalhoUi::frame(c,{26,242,188,23});snprintf(b,sizeof(b),"%u pontos disponiveis",rpg::advancementPoints(g));touchLabel(c,30,250,180,b,UI_GOLD);
 for(auto r:{outcomeUi::attributes,outcomeUi::levelContinue})carvalhoUi::frame(c,r);
 touchLabel(c,18,288,94,"Atributos",outcomeUi::canImprove(g)?UI_GOLD:UI_MUTED);touchLabel(c,128,288,94,"Continuar",UI_GOLD);
 if(rpg::advancementPoints(g)&&!outcomeUi::canImprove(g))touchLabel(c,14,230,212,"Distribua ao terminar a atividade",UI_GOLD);
}
