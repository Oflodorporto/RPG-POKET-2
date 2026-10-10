#pragma once
#include "PanelArt.h"
#include "Progression.h"
namespace panelUi {
constexpr scenicUi::Rect bagBack={40,281,82,26},bagUse={126,281,81,26},bagGear={46,263,156,17};
constexpr scenicUi::Rect evolution={10,112,220,26};
inline scenicUi::Rect bagSlot(unsigned i){return {53+int(i%3)*47,125+int(i/3)*33,43,30};}
constexpr scenicUi::Rect cityButtons[]={{26,180,91,23},{122,180,91,23},{26,204,91,23},{122,204,91,23},{26,228,91,23},{122,228,91,23},{26,252,91,23},{122,252,91,23},{70,278,112,21}};
constexpr scenicUi::Rect battleButtons[]={{10,240,108,29},{122,240,108,29},{10,274,108,29},{122,274,108,29}};
}
template<class C>void panelImage(C& c,const uint16_t* palette,unsigned asset,int top=0,int bottom=320){
 const auto* indices=assetBytes(asset);uint16_t row[240];
 for(int y=top;y<bottom;++y){for(int x=0;x<240;++x)row[x]=palette[indices[y*240+x]];c.draw16bitRGBBitmap(0,y,row,240,1);}
}
#define PANEL_IMAGE(c,name) panelImage(c,panelArt::name##Palette,panelArt::name##Asset)
template<class C>void panelBattleArena(C& c){const auto* source=assetBytes(panelArt::battleAsset);uint16_t row[240];for(int y=66;y<192;++y){int sy=76+(y-66)*115/126;for(int x=0;x<240;++x)row[x]=panelArt::battlePalette[source[sy*240+x*128/240]];c.draw16bitRGBBitmap(0,y,row,240,1);}}
template<class C>void panelLabel(C& c,int x,int y,int w,const char* s,uint16_t color=UI_WHITE,int size=1){int len=int(strlen(s))*6*size;if(len>w){size=1;len=int(strlen(s))*6;}c.setTextSize(size);c.setTextColor(color);c.setCursor(x+std::max(0,(w-len)/2),y+1);char clipped[64];snprintf(clipped,sizeof(clipped),"%.*s",std::min(63,w/(6*size)),s);c.print(clipped);}
template<class C>void panelBar(C& c,int x,int y,int w,unsigned value,unsigned maximum,uint16_t color){c.fillRect(x,y,w,4,0x18c6);c.fillRect(x,y,maximum?int(uint64_t(w)*std::min(value,maximum)/maximum):0,4,color);}
template<class C>void panelCrop(C& c,const uint16_t* palette,unsigned asset,int sx,int sy,int iw,int ih,int x,int y,int w,int h){const auto* source=assetBytes(asset);uint16_t row[48];for(int py=0;py<h;++py){for(int px=0;px<w;++px)row[px]=palette[source[(sy+py*ih/h)*240+sx+px*iw/w]];c.draw16bitRGBBitmap(x,y+py,row,w,1);}}
template<class C>void panelHero(C& c,const rpg::Game& g,int x,int y,int w,int h,unsigned frame=0){c.fillRect(x,y,w,h,0x0843); // only the live personalized hero is displayed
 for(int py=0;py<h;++py)for(int px=0;px<w;++px){auto color=personalPixel(g,6,(py*66/h)*80+px*80/w);if(color!=0xf81f)c.fillRect(x+px,y+py,1,1,color);} (void)frame;
}
template<class C>void drawPanelSkills(C& c,const rpg::Game& g,const ViewState& v){
 PANEL_IMAGE(c,skills);scenicPanel(c,14,143,48,29);panelLabel(c,17,153,42,"Guia",UI_GOLD);panelHero(c,g,90,89,62,48);c.fillRect(75,140,93,18,0x0843);char b[64];
 snprintf(b,sizeof(b),"HP%u/%u",g.p.hp,g.p.maxhp);panelLabel(c,75,142,46,b);snprintf(b,sizeof(b),"MP%u/%u",g.p.mp,g.p.maxmp);panelLabel(c,123,142,46,b);panelBar(c,78,154,37,g.p.hp,g.p.maxhp,UI_RED);panelBar(c,124,154,37,g.p.mp,g.p.maxmp,UI_BLUE);
 for(int i=0;i<2;++i){int y=180+i*48;c.fillRect(75,y,132,31,0x0843);snprintf(b,sizeof(b),"%s / %u MP",g.dndProgression&&g.p.cls==0?(i?"Escudo arcano":"Misseis magicos"):g.dndProgression&&g.p.cls==3&&!i?"Golpe brutal":rpg::skillName(g.p.cls,i),g.dndProgression&&g.p.cls==0?rpg::powerCost(g,i?rpg::Action::ShieldSpell:rpg::Action::MagicMissile):rpg::skillCost(g.p.cls,i));panelLabel(c,75,y,132,b,g.p.mp>=(g.dndProgression&&g.p.cls==0?rpg::powerCost(g,i?rpg::Action::ShieldSpell:rpg::Action::MagicMissile):rpg::skillCost(g.p.cls,i))?UI_GOLD:UI_MUTED);
 const char* desc=g.dndProgression&&g.p.cls==0?(i?"Bloqueia 75%":"3 dardos / nao erra"):i?(g.p.cls<2?"Bloqueia 75%":g.p.cls==3?"Cura 25% / guarda 50%":"Bloqueia 50%"):(g.p.cls==0?"+80% / ignora defesa":g.p.cls==3?"Dano dobrado":g.p.cls==2?"+50% / nao erra":"Dano +50%");panelLabel(c,75,y+19,132,desc);
 if(g.p.cls!=0){c.fillRect(34,y,34,29,0x0843);magicSprite(c,i?sprites_shield[0]:sprites_slash[0],64,72,35,y,32,29);}}
 if(g.dndProgression){scenicPanel(c,26,54,188,28);panelLabel(c,30,63,180,g.p.cls==0?"Grimorio >":g.p.cls==1?"Poderes / juramento >":"Poderes de classe >");}
 if(*v.message)panelLabel(c,8,161,224,v.message,UI_RED);
 else {snprintf(b,sizeof(b),"Pocao usa turno / risco %u HP",rpg::incomingCeiling(g,g.guard));panelLabel(c,8,161,224,b,UI_GOLD);}
 for(int i=0;i<2;++i)if(g.p.mp<(g.dndProgression&&g.p.cls==0?rpg::powerCost(g,i?rpg::Action::ShieldSpell:rpg::Action::MagicMissile):rpg::skillCost(g.p.cls,i))||(g.dndProgression&&g.p.cls==1&&i==0&&g.p.level<2))panelLabel(c,75,180+i*48+19,132,g.dndProgression&&g.p.cls==1&&i==0&&g.p.level<2?"Desbloqueia Nv 2":"Sem mana",UI_MUTED);
}
template<class C>void drawPanelBag(C& c,const rpg::Game& g,const ViewState& v){
 PANEL_IMAGE(c,bag);char b[64];snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);panelLabel(c,92,57,78,b,UI_GOLD);
 panelHero(c,g,55,72,51,41);c.fillRect(110,72,76,41,0x0843);snprintf(b,sizeof(b),"HP %u/%u",g.p.hp,g.p.maxhp);panelLabel(c,112,77,66,b);panelBar(c,114,88,65,g.p.hp,g.p.maxhp,UI_RED);
 snprintf(b,sizeof(b),"MP %u/%u",g.p.mp,g.p.maxmp);panelLabel(c,112,97,66,b);panelBar(c,114,106,65,g.p.mp,g.p.maxmp,UI_BLUE);
 const char* names[]={"Vida","Mana","Cristal","Racao","Mapa","Sorte","Kit"};unsigned q[]={g.p.life,g.p.mana,g.crystals,g.rations,g.charts,g.charms,unsigned(g.sleepKit)};
 for(unsigned i=0;i<7;++i){auto r=panelUi::bagSlot(i);c.fillRect(r.x+30,r.y+17,11,9,0x0843);snprintf(b,sizeof(b),"%u",q[i]);panelLabel(c,r.x+22,r.y+17,19,b,q[i]?UI_GREEN:UI_MUTED);for(int border=0;border<3;++border)c.drawRect(r.x+border,r.y+border,r.w-border*2,r.h-border*2,i==v.choice?UI_GOLD:0x3186);}
 auto scrapRect=panelUi::bagSlot(7);scenicPanel(c,scrapRect.x,scrapRect.y,scrapRect.w,scrapRect.h);char scrapText[24];panelLabel(c,scrapRect.x+2,scrapRect.y+4,scrapRect.w-4,"Sucata",UI_GOLD);snprintf(scrapText,sizeof(scrapText),"x%u",g.scrap);panelLabel(c,scrapRect.x+2,scrapRect.y+17,scrapRect.w-4,scrapText,g.scrap?UI_GREEN:UI_MUTED);
 auto pickRect=panelUi::bagSlot(8);scenicPanel(c,pickRect.x,pickRect.y,pickRect.w,pickRect.h);panelLabel(c,pickRect.x+2,pickRect.y+4,pickRect.w-4,"Gazuas",UI_GOLD);snprintf(scrapText,sizeof(scrapText),"x%u",g.gazuas);panelLabel(c,pickRect.x+2,pickRect.y+17,pickRect.w-4,scrapText,g.gazuas?UI_GREEN:UI_MUTED);
 unsigned i=std::min(6u,unsigned(v.choice));c.fillRect(84,228,110,30,0x0843);snprintf(b,sizeof(b),"%s x%u",names[i],q[i]);panelLabel(c,89,231,105,b,UI_GOLD);
 const char* desc=i==0?"Recupera 30% HP":i==1?"Recupera 50% MP":i==2?"Chave da cripta":i==3?"Comida no acampamento":i==6?"Kit de acampamento":"Bonus na viagem";panelLabel(c,89,247,105,desc);
 // The sample detail potion is replaced by the selected inventory icon.
 
 auto icon=panelUi::bagSlot(i);panelCrop(c,panelArt::bagPalette,panelArt::bagAsset,icon.x+10,icon.y+2,23,18,55,231,29,24);
 snprintf(b,sizeof(b),"Equipamentos: %u",rpg::gearOwnedCount(g.owned));panelLabel(c,92,269,101,b);
 if(*v.message)panelLabel(c,4,310,232,v.message,UI_RED);
}
template<class C>void drawPanelCharacter(C& c,const rpg::Game& g){
 PANEL_IMAGE(c,character);panelHero(c,g,84,59,71,51);scenicPanel(c,10,112,220,26);panelLabel(c,14,115,212,"Trilha de classe >",UI_GOLD);c.fillRect(54,139,132,18,0x0843);c.fillRect(55,161,63,19,0x0843);c.fillRect(124,161,63,19,0x0843);char b[64];
 unsigned next=rpg::nextBenefitLevel(g);if(next){auto gains=rpg::levelBenefits(g,next);snprintf(b,sizeof(b),"Nv %u: %s",next,gains.lines[0]);}else if(g.dndProgression&&g.p.level<20)snprintf(b,sizeof(b),"Nv %u: mais HP, MP e ataque",unsigned(g.p.level)+1);else snprintf(b,sizeof(b),g.dndProgression?"Nivel maximo: jornada continua":"Heroi antigo: regras preservadas");panelLabel(c,14,126,212,b,UI_GREEN);
 snprintf(b,sizeof(b),"Nv %u  XP %lu/%u",g.p.level,(unsigned long)g.p.xp,rpg::xpNeeded(g));panelLabel(c,40,142,160,b);panelBar(c,60,152,120,g.p.xp,rpg::xpNeeded(g),UI_GREEN);
 snprintf(b,sizeof(b),"HP %u/%u",g.p.hp,g.p.maxhp);panelLabel(c,56,164,61,b);panelBar(c,59,175,56,g.p.hp,g.p.maxhp,UI_RED);
 snprintf(b,sizeof(b),"MP %u/%u",g.p.mp,g.p.maxmp);panelLabel(c,124,164,63,b);panelBar(c,126,175,56,g.p.mp,g.p.maxmp,UI_BLUE);
 snprintf(b,sizeof(b),"ATQ %u",unsigned(rpg::effectiveAttack(g)));panelLabel(c,73,191,43,b);snprintf(b,sizeof(b),"DEF %u",unsigned(rpg::effectiveDefense(g)));panelLabel(c,142,191,42,b);
 snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);panelLabel(c,95,206,88,b,UI_GOLD);snprintf(b,sizeof(b),"Pocoes HP x%u / MP x%u",g.p.life,g.p.mana);panelLabel(c,74,225,113,b);
 // Intelligence/agility have no gameplay values yet; show only real attributes.
 scenicPanel(c,40,240,160,31);snprintf(b,sizeof(b),"Sobrevivencia %u",rpg::survival(g));panelLabel(c,45,245,150,b,UI_GREEN);snprintf(b,sizeof(b),"Sorte %u",rpg::luck(g));panelLabel(c,45,258,150,b,UI_GOLD);
}
template<class C>void drawPanelVillage(C& c,const rpg::Game& g,const ViewState& v){
 PANEL_IMAGE(c,village);scenicPanel(c,10,80,220,30);auto goal=story::objective(g);panelLabel(c,16,85,208,goal.title,UI_GOLD);panelLabel(c,16,97,208,"Toque: proximo objetivo");char b[48];snprintf(b,sizeof(b),"Explorar Nv %u+",rpg::cityLevel(g.city));panelLabel(c,77,52,89,b);snprintf(b,sizeof(b),"%lu g",(unsigned long)g.p.gold);panelLabel(c,96,61,49,b,UI_GOLD);if(*v.message)panelLabel(c,7,166,226,v.message,UI_RED);
}
template<class C>void panelCampBackdrop(C& c,const rpg::Game& g,const ViewState& v){
 if(v.page==Page::CampSetup){PANEL_IMAGE(c,campSetup);}else if(v.page==Page::CampRoll){PANEL_IMAGE(c,campRoll);}else {PANEL_IMAGE(c,campRest);}
 if(g.city!=1){drawBackdrop(c,g.city==0?bg_explore0:g.city==2?bg_explore2:bg_explore3);scenicHeading(c,v.page==Page::CampSetup?"ACAMPAMENTO":v.page==Page::CampRoll?"TESTE DE ACAMPAR":"DESCANSANDO");}
 panelLabel(c,82,64,76,placeName(g.city),UI_GOLD);
}

template<class C>struct PanelEffectCanvas {
 C& c;void fillRect(int x,int y,int w,int h,uint16_t color){if(y+34>=192)return;c.fillRect(x,y+34,w,std::min(h,192-y-34),color);}
 void drawRect(int x,int y,int w,int h,uint16_t color){c.drawRect(x,y+34,w,h,color);}
 void draw16bitRGBBitmap(int x,int y,uint16_t* pixels,int w,int h){c.draw16bitRGBBitmap(x,y+34,pixels,w,h);}
};

