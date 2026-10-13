#pragma once
#include "PanelArt.h"
#include "Progression.h"
#include "TouchPanels.h"
namespace panelUi {
constexpr scenicUi::Rect skillsGuide={14,110,48,29};
constexpr scenicUi::Rect bagBack={8,276,108,44},bagUse={124,276,108,44},bagGear={12,234,216,40};
constexpr scenicUi::Rect evolution={10,112,220,26};
inline scenicUi::Rect bagSlot(unsigned i){return {34+int(i%3)*60,38+int(i/3)*56,52,52};}
constexpr scenicUi::Rect cityButtons[]={{6,84,112,40},{122,84,112,40},{6,132,112,40},{122,132,112,40},{6,180,112,40},{122,180,112,40},{6,228,112,40},{122,228,112,40},{6,272,228,40}};
constexpr scenicUi::Rect battleButtons[]={{10,240,108,29},{122,240,108,29},{10,274,108,29},{122,274,108,29}};
}
template<class C>void panelImage(C& c,const uint16_t* palette,unsigned asset,int top=0,int bottom=320){
 const auto* indices=assetBytes(asset);uint16_t row[240];
 for(int y=top;y<bottom;++y){for(int x=0;x<240;++x)row[x]=palette[indices[y*240+x]];c.draw16bitRGBBitmap(0,y,row,240,1);}
}
#define PANEL_IMAGE(c,name) panelImage(c,panelArt::name##Palette,panelArt::name##Asset)
template<class C>void panelBattleArena(C& c){const auto* source=assetBytes(panelArt::battleAsset);uint16_t row[240];for(int y=66;y<192;++y){int sy=76+(y-66)*115/126;for(int x=0;x<240;++x)row[x]=panelArt::battlePalette[source[sy*240+x*128/240]];c.draw16bitRGBBitmap(0,y,row,240,1);}}
template<class C>void panelLabel(C& c,int x,int y,int w,const char* s,uint16_t color=UI_WHITE,int size=1){int len=int(strlen(s))*6*size;if(len>w){size=1;len=int(strlen(s))*6;}c.setTextSize(size);c.setTextColor(color);c.setCursor(x+std::max(0,(w-len)/2),y+1);char clipped[64];snprintf(clipped,sizeof(clipped),"%.*s",std::min(63,w/(6*size)),s);if(y+2+8*size<=320){c.setTextColor(UI_INK);c.setCursor(x+std::max(0,(w-len)/2),y+2);c.print(clipped);}c.setTextColor(color);c.setCursor(x+std::max(0,(w-len)/2),y+1);c.print(clipped);}
template<class C>void panelBar(C& c,int x,int y,int w,unsigned value,unsigned maximum,uint16_t color){c.fillRect(x,y,w,4,0x18c6);c.fillRect(x,y,maximum?int(uint64_t(w)*std::min(value,maximum)/maximum):0,4,color);}
template<class C>void panelCrop(C& c,const uint16_t* palette,unsigned asset,int sx,int sy,int iw,int ih,int x,int y,int w,int h){const auto* source=assetBytes(asset);uint16_t row[48];for(int py=0;py<h;++py){for(int px=0;px<w;++px)row[px]=palette[source[(sy+py*ih/h)*240+sx+px*iw/w]];c.draw16bitRGBBitmap(x,y+py,row,w,1);}}
template<class C>void panelHero(C& c,const rpg::Game& g,int x,int y,int w,int h,unsigned frame=0){c.fillRect(x,y,w,h,0x0843); // only the live personalized hero is displayed
 for(int py=0;py<h;++py)for(int px=0;px<w;++px){auto color=personalPixel(g,6,(py*66/h)*80+px*80/w);if(color!=0xf81f)c.fillRect(x+px,y+py,1,1,color);} (void)frame;
}
template<class C>void drawPanelSkills(C& c,const rpg::Game& g,const ViewState& v){
 PANEL_IMAGE(c,skills);const auto guide=panelUi::skillsGuide;scenicPanel(c,guide.x,guide.y,guide.w,guide.h);panelLabel(c,guide.x+3,guide.y+10,guide.w-6,"Guia",UI_GOLD);panelHero(c,g,90,89,62,48);c.fillRect(75,140,93,18,0x0843);char b[64];
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
 bagSkin::crop(c,0,0,240,320,0,0,240,320);bagSkin::leather(c,8,4,224,270);char b[64];
 bagSkin::crop(c,48,14,144,20,12,6,216,26);
 snprintf(b,sizeof(b),"BOLSA / %lu ouro",(unsigned long)g.p.gold);touchLabel(c,16,9,208,b,UI_GOLD);
 snprintf(b,sizeof(b),"HP %u/%u MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);touchLabel(c,16,21,208,b);
 const char* names[]={"Vida","Mana","Cristal","Racao","Mapa","Sorte","Kit","Sucata","Gazuas"};
 unsigned q[]={g.p.life,g.p.mana,g.crystals,g.rations,g.charts,g.charms,unsigned(g.sleepKit),g.scrap,g.gazuas};
 for(unsigned i=0;i<9;++i){auto r=panelUi::bagSlot(i);bagSkin::frame(c,r);
  if(i==v.choice){c.drawRect(r.x,r.y,r.w,r.h,0xffff);c.drawRect(r.x+1,r.y+1,r.w-2,r.h-2,UI_GOLD);}
  if(i<7)panelCrop(c,panelArt::bagPalette,panelArt::bagAsset,53+int(i%3)*47+10,125+int(i/3)*33+2,23,18,r.x+14,r.y+4,24,22);
  else if(i==7){c.fillRect(r.x+15,r.y+10,14,10,0xbdf7);c.drawRect(r.x+15,r.y+10,14,10,UI_WHITE);c.fillRect(r.x+25,r.y+6,13,10,UI_MUTED);}
  else {c.fillRect(r.x+12,r.y+8,27,3,UI_GOLD);c.fillRect(r.x+36,r.y+8,3,16,UI_GOLD);c.fillRect(r.x+12,r.y+11,3,5,UI_GOLD);}
  touchLabel(c,r.x+2,r.y+29,r.w-4,names[i]);snprintf(b,sizeof(b),"x%u",q[i]);touchLabel(c,r.x+2,r.y+40,r.w-4,b,q[i]?UI_GREEN:UI_MUTED);
 }
 unsigned i=std::min(8u,unsigned(v.choice));bagSkin::crop(c,29,196,182,19,12,207,216,24);
 const char* desc=i==0?"Recupera 30% HP":i==1?"Recupera 50% MP":i==2?"Chave da cripta":i==3?"Comida no acampamento":i==4?"Bonus na viagem":i==5?"Bonus na viagem":i==6?"Kit de acampamento":i==7?"Ver sucata / vender na cidade":"Ver gazuas e orientacoes";
 if(*v.message)touchLines(c,16,210,208,v.message,UI_RED);else {touchLabel(c,16,210,208,names[i],UI_GOLD);touchLabel(c,16,221,208,desc);}
 bagSkin::crop(c,22,225,196,29,panelUi::bagGear.x,panelUi::bagGear.y,panelUi::bagGear.w,panelUi::bagGear.h);
 snprintf(b,sizeof(b),"Equipamentos: %u",rpg::gearOwnedCount(g.owned));touchLabel(c,16,246,208,b,0xffff,2);
 for(auto r:{panelUi::bagBack,panelUi::bagUse})bagSkin::crop(c,r.x<120?20:124,263,96,35,r.x,r.y,r.w,r.h);
 touchLabel(c,panelUi::bagBack.x+2,290,panelUi::bagBack.w-4,"Voltar",0xffff,2);
 touchLabel(c,panelUi::bagUse.x+2,290,panelUi::bagUse.w-4,i>=7?"Ver":"Usar",0xffff,2);
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
 PANEL_IMAGE(c,village);char b[64];
 // Cover baked labels only; the existing Carvalho scenery remains the backdrop.
 for(auto r:{scenicUi::Rect{0,0,240,80},scenicUi::Rect{0,220,240,8},scenicUi::Rect{0,268,240,4}})c.fillRect(r.x,r.y,r.w,r.h,UI_INK);
 scenicPanel(c,6,4,228,24);touchLabel(c,9,8,222,"Carvalho",UI_GOLD,2);
 scenicPanel(c,6,31,112,14);snprintf(b,sizeof(b),"Explorar Nv %u+",rpg::cityLevel(g.city));touchLabel(c,8,34,108,b);
 scenicPanel(c,122,31,112,14);snprintf(b,sizeof(b),"%lu ouro",(unsigned long)g.p.gold);touchLabel(c,124,34,108,b,UI_GOLD);
 const auto mission=touchUi::cityMission;scenicPanel(c,mission.x,mission.y,mission.w,mission.h);auto goal=story::objective(g);
 if(*v.message)touchLines(c,10,53,220,v.message,UI_RED);else {touchLabel(c,10,53,220,goal.title,UI_GOLD);touchLabel(c,10,65,220,"Toque: proximo objetivo");}
 const char* labels[]={"Conversar","Loja","Bolsa","Heroi","Explorar","Guilda","Acampar","Mapa","Menu"};
 for(unsigned i=0;i<9;++i){auto r=panelUi::cityButtons[i];scenicPanel(c,r.x,r.y,r.w,r.h);touchLabel(c,r.x+1,r.y+12,r.w-2,labels[i],0xffff,2);}
}

template<class C>void panelCampBackdrop(C& c,const rpg::Game& g,const ViewState& v){
 if(v.page==Page::CampSetup){PANEL_IMAGE(c,campSetup);}else if(v.page==Page::CampRoll){PANEL_IMAGE(c,campRoll);}else {PANEL_IMAGE(c,campRest);}
 if(g.city!=1){drawBackdrop(c,g.city==0?bg_explore0:g.city==2?bg_explore2:bg_explore3);scenicHeading(c,v.page==Page::CampSetup?"ACAMPAMENTO":v.page==Page::CampRoll?"TESTE DE ACAMPAR":"DESCANSANDO");}
 panelLabel(c,82,64,76,placeName(g.city),UI_GOLD);
}

template<class C>struct PanelEffectCanvas : CombatCanvas<C> {
 explicit PanelEffectCanvas(C& c):CombatCanvas<C>{c,86,192,0,34}{}
};
