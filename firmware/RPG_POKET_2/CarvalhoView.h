#pragma once
#include "SceneArt.h"
// Approved two-page Carvalho layout. Geometry and action IDs are shared by input/render.
namespace carvalhoUi {
constexpr scenicUi::Rect mission={6,72,228,40};
constexpr scenicUi::Rect actions[]={{6,176,112,40},{122,176,112,40},{6,224,112,40},{122,224,112,40}};
constexpr scenicUi::Rect menu={6,272,112,40},pager={122,272,112,40};
constexpr uint8_t actionIds[2][4]={{0,1,2,4},{3,5,6,7}};
inline unsigned page(unsigned value){return value==1?1:0;}
inline bool paged(unsigned city){return city==0||city==2||city==3;}
inline int choice(unsigned value,int x,int y){if(menu.contains(x,y))return 8;for(unsigned i=0;i<4;++i)if(actions[i].contains(x,y))return actionIds[page(value)][i];return -1;}
inline uint16_t sourcePixel(unsigned x,unsigned y){if(x>=240||y>=320)return UI_INK;return panelArt::villagePalette[assetBytes(panelArt::villageAsset)[y*240+x]];}
template<class C>void crop(C& c,int sx,int sy,int sw,int sh,int x,int y,int w,int h){
 if(sx<0||sx>=240||sy<0||sy>=320||sw<=0||sw>240-sx||sh<=0||sh>320-sy||x<0||y<0||w<=0||w>240||h<=0||h>320||x>240-w||y>320-h)return;
 uint16_t row[240];for(int py=0;py<h;++py){for(int px=0;px<w;++px)row[px]=sourcePixel(sx+px*sw/w,sy+py*sh/h);c.draw16bitRGBBitmap(x,y+py,row,w,1);}
}
// Reuse the gold border only, never the original baked text/icon. No new art payload.
template<class C>void frame(C& c,const scenicUi::Rect& r){
 int x=r.x,y=r.y,w=r.w,h=r.h;if(x<0||y<0||w<8||w>240||h<8||h>320||x>240-w||y>320-h)return;
 if(::menu.art!=ArtStatus::Ready){scenicPanel(c,x,y,w,h);return;}
 c.fillRect(x,y,w,h,sourcePixel(109,191));
 for(int dx:{0,1})for(int dy:{0,1})crop(c,dx?113:26,dy?199:180,4,4,x+(dx?w-4:0),y+(dy?h-4:0),4,4);
 crop(c,30,180,83,4,x+4,y,w-8,4);crop(c,30,199,83,4,x+4,y+h-4,w-8,4);crop(c,26,184,4,15,x,y+4,4,h-8);crop(c,113,184,4,15,x+w-4,y+4,4,h-8);
}
template<class C>void icon(C& c,unsigned id,int x,int y){if(id>8||::menu.art!=ArtStatus::Ready)return;if(id==8){crop(c,83,280,27,20,x,y,24,24);return;}crop(c,id%2?126:31,182+int(id/2)*24,25,19,x,y,24,24);}
template<class C>void action(C& c,const scenicUi::Rect& r,unsigned id){
 static const char* labels[]={"Conversar","Loja","Bolsa","Heroi","Explorar","Guilda","Acampar","Mapa","Menu"};
 if(id>8)return;frame(c,r);icon(c,id,r.x+9,r.y+8);touchLabel(c,r.x+37,r.y+17,69,labels[id]);
}
}
template<class C>void drawCarvalho(C& c,const rpg::Game& g,const ViewState& v){
 const bool ready=menu.art==ArtStatus::Ready,regional=g.city==2||g.city==3;
 if(ready){if(regional)sceneArt::draw(c,g.city==2?sceneArt::Id::mares:sceneArt::Id::aurora);else PANEL_IMAGE(c,village);}else c.fillRect(0,0,240,320,UI_INK);
 // Preserve the original tree, sky and ornate title; only dynamic badge interiors change.
 char text[64];
 if(regional){
  if(ready){carvalhoUi::crop(c,58,21,125,29,58,21,125,29);c.fillRect(67,29,107,15,carvalhoUi::sourcePixel(109,191));}else scenicPanel(c,58,21,125,29);
  touchLabel(c,66,29,109,g.city==2?"Mares":"Aurora",UI_GOLD,2);
  carvalhoUi::frame(c,{6,52,112,16});carvalhoUi::frame(c,{122,52,112,16});snprintf(text,sizeof(text),"Explorar Nv%u+",rpg::cityLevel(g.city));touchLabel(c,10,56,104,text);
  snprintf(text,sizeof(text),"%lu ouro",(unsigned long)g.p.gold);touchLabel(c,126,56,104,text,UI_GOLD);
 }else{
  if(!ready){scenicPanel(c,59,24,124,24);touchLabel(c,62,28,118,"Carvalho",UI_GOLD,2);}
  c.fillRect(79,50,84,9,ready?carvalhoUi::sourcePixel(80,53):0x0843);snprintf(text,sizeof(text),"Explorar Nv%u+",rpg::cityLevel(g.city));touchLabel(c,79,51,84,text);
  c.fillRect(92,61,54,7,ready?carvalhoUi::sourcePixel(94,64):0x0843);snprintf(text,sizeof(text),g.p.gold<=9999?"%lu ouro":"%lu g",(unsigned long)g.p.gold);touchLabel(c,91,61,57,text,UI_GOLD);
 }
 carvalhoUi::frame(c,carvalhoUi::mission);if(*v.message)touchLines(c,12,80,216,v.message,UI_RED);else {auto goal=story::objective(g);touchLabel(c,12,80,216,goal.title,UI_GOLD);touchLabel(c,12,93,216,"Toque: proximo objetivo");}
 // Existing cobblestones cover old baked controls, leaving the central scene112..175 intact.
 if(ready&&!regional){uint16_t row[240];for(int y=176;y<320;++y){for(int x=0;x<240;++x)row[x]=carvalhoUi::sourcePixel(x,300+(y-176)%20);c.draw16bitRGBBitmap(0,y,row,240,1);}}
 unsigned page=carvalhoUi::page(v.cityPage);for(unsigned i=0;i<4;++i)carvalhoUi::action(c,carvalhoUi::actions[i],carvalhoUi::actionIds[page][i]);
 carvalhoUi::action(c,carvalhoUi::menu,8);carvalhoUi::frame(c,carvalhoUi::pager);touchLabel(c,125,285,106,page?"< Voltar":"Mais >");touchLabel(c,125,298,106,page?"Pagina 2/2":"Pagina 1/2",UI_GOLD);
}
