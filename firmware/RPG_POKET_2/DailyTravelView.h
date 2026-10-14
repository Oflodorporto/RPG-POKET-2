#pragma once
#include "TentMarker.h"
// Day-by-day travel presentation uses saved facts and the same personalized hero.
// No draws modify Game or read the SD. Small props are native pixel sprites.
namespace dailyUi {
inline Journey routePosition(const rpg::Game& g,unsigned animation){
 const bool leg=g.daily.phase==rpg::DailyPhase::Leg;
 const bool ended=g.daily.phase==rpg::DailyPhase::NightRoll||g.daily.phase==rpg::DailyPhase::NightBattle||g.daily.phase==rpg::DailyPhase::Rest||g.daily.phase==rpg::DailyPhase::Tent;
 unsigned time=(g.daily.day-(ended?0u:1u))*1000+(leg?std::min(1000u,animation):0);
 unsigned edges=g.daily.totalDays/2,edge=std::min(edges-1,time/2000);int direction=g.tripTo>g.daily.origin?1:-1;
 Journey segment;segment.from=uint8_t(int(g.daily.origin)+direction*int(edge));segment.to=uint8_t(int(segment.from)+direction);segment.progress=std::min(1000u,(time-edge*2000)/2);segment.escort=rpg::escortTrip(g);return segment;
}

template<class C>void wagon(C& c,int x,int y,unsigned size=1){
 auto r=[&](int a,int b,int w,int h,uint16_t color){c.fillRect(x+a*size,y+b*size,w*size,h*size,color);};
 r(2,3,22,12,0x51e3);r(4,1,18,10,0xd5f3);r(6,0,14,2,0xff19);r(11,2,2,9,0x9b86);r(3,12,20,2,0xb3c8);r(3,10,20,1,0x8b66);r(5,4,2,6,0xff19);r(19,4,2,6,0xb4b0);r(3,14,20,2,0x6a63);
 for(int wheel:{4,18}){r(wheel-2,13,8,8,UI_INK);r(wheel-1,14,6,6,0xb3c8);r(wheel,15,4,4,0x20c3);r(wheel+1,14,2,6,UI_GOLD);r(wheel-1,16,6,2,UI_GOLD);r(wheel+1,16,2,2,0xff19);}
}
template<class C>void merchant(C& c,int x,int y,unsigned frame){
 // Reuse the detailed seated camp traveller as the merchant, while the player
 // at the opposite side always uses their own race/class/clothing sprite.
 magicSprite(c,campArt::heroes[2][1][frame%2],56,64,x,y,56,64);
}
template<class C>void road(C& c,Point p,Point q){int dx=abs(q.x-p.x),sx=p.x<q.x?1:-1,dy=-abs(q.y-p.y),sy=p.y<q.y?1:-1,err=dx+dy;for(;;){c.fillRect(p.x-1,p.y-1,3,3,UI_GOLD);if(p.x==q.x&&p.y==q.y)break;int e=2*err;if(e>=dy){err+=dy;p.x+=sx;}if(e<=dx){err+=dx;p.y+=sy;}}}
}
template<class C>void drawDailyTravel(C& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 char b[64];auto label=[&](int y,const char* s,uint16_t color=UI_WHITE){touchLabel(c,12,y,216,s,color);};
 const bool escort=rpg::escortTrip(g),night=v.page==Page::CampRoll,rest=v.page==Page::CampRest;
 if(rest){
  auto regional=g;regional.city=g.daily.region;const auto previousPeriod=backdropPeriod;backdropPeriod=worldClock::Period::Night;panelCampBackdrop(c,regional,v);backdropPeriod=previousPeriod;carvalhoUi::frame(c,{12,48,216,42});
  snprintf(b,sizeof(b),"Pernoite %u / %s",g.daily.day,placeName(g.daily.region));label(58,b,UI_GOLD);label(76,"Sem cobrar outra racao");
  if(menu.art==ArtStatus::Ready){if(escort){c.fillRect(149,199,73,4,0x0842);c.fillRect(158,202,58,2,0x1083);dailyUi::wagon(c,144,137,3);}campUi::hero(c,g,1,110,100,120,g.daily.rationsPaid>0,frame);magicSprite(c,campArt::fires[frame%4],48,48,82,167,60,60);if(escort){c.fillRect(167,232,57,3,0x0842);dailyUi::merchant(c,179,176,frame);}if(g.daily.kit)magicSprite(c,campArt::sleepingbag,48,48,40,221,48,28);}
  carvalhoUi::frame(c,{12,258,216,53});snprintf(b,sizeof(b),"HP %u/%u / MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);label(267,b,UI_GREEN);label(284,"Descansando... 2 segundos");c.fillRect(21,301,198*std::min(1000u,v.campProgress)/1000,3,UI_GREEN);return;
 }
 scenicWorld(c,false,night?worldClock::Period::Night:worldClock::Period::Day);
 for(unsigned i=0;i<9;++i)dailyUi::road(c,roadPoints[i],roadPoints[i+1]);
 for(unsigned i=0;i<4;++i)c.drawRect(places[i].x-8,places[i].y-8,17,17,i==g.daily.region?UI_GREEN:UI_BLUE);
 carvalhoUi::frame(c,{12,8,216,45});label(19,night?"TESTE DO PERNOITE":v.page==Page::TravelRoll?"TESTE DE PARTIDA":escort?"ESCOLTA PELA ESTRADA":"VIAGEM PELA ESTRADA",UI_GOLD);
 snprintf(b,sizeof(b),"Dia %u de %u / %s",g.daily.day,g.daily.totalDays,placeName(g.daily.region));label(37,b);
 if(v.page==Page::TravelRoll||night){
  unsigned roll=night?g.daily.roll:g.tripRoll,total=night?g.daily.total:g.tripTotal,difficulty=night?g.daily.difficulty:g.tripDifficulty;
  bool safe=night?rpg::dailyNightSafe(g):rpg::tripSafe(g);
  d20Ui::draw(c,120,139,46,roll,menu.rollReady,uint32_t(menu.renderNow-menu.rollStarted));
  label(198,"D20",UI_GOLD);
  carvalhoUi::frame(c,{12,218,216,45});snprintf(b,sizeof(b),"D20 %u / total %u / CD %u",roll,total,difficulty);label(226,menu.rollReady?b:"Rolando o dado...");label(244,menu.rollReady?(safe?"LOCAL SEGURO":"PERIGO NA ESTRADA"):"Aguarde o resultado",safe?UI_GREEN:UI_GOLD);
  carvalhoUi::frame(c,{14,272,212,40});label(288,!menu.rollReady?"Aguarde":safe?(night?"Descansar":"Iniciar dia 1"):"Enfrentar perigo",UI_GOLD);return;
 }
 Journey path=dailyUi::routePosition(g,menu.journey.progress);Point p=path.position();if(escort){Point q=path.wagonPosition();dailyUi::wagon(c,std::max(0,std::min(214,q.x-12)),q.y-19);}
 if(menu.art==ArtStatus::Ready)for(unsigned y=0;y<24;++y)for(unsigned x=0;x<22;++x){unsigned pose=g.daily.phase==rpg::DailyPhase::Leg?frame%6:0;auto color=personalPixel(g,pose,(y*5)*112+x*5);if(color!=SPRITE_KEY)c.fillRect(p.x-11+x,p.y-27+y,1,1,color);}
 else c.fillRect(p.x-3,p.y-8,6,8,UI_GREEN);
 if(g.daily.phase==rpg::DailyPhase::Tent){
  // Road camps stay centred below the exact stop. At a city, move just the
  // tent beside the hero so its fabric/shadow cannot cover the baked-in name.
  int tentX=p.x-tentMarker::Width/2,tentY=p.y+18-tentMarker::Height;
  for(const auto& city:places)if(p.x==city.x&&p.y==city.y){
   tentX=p.x+18; if(tentX+tentMarker::Width>240)tentX=p.x-17-tentMarker::Width;
   tentY=p.y+4-tentMarker::Height;break;
  }
  tentMarker::draw(c,std::max(0,std::min(240-tentMarker::Width,tentX)),tentY);
 }
 c.fillRect(0,218,240,102,UI_INK);carvalhoUi::frame(c,{12,220,216,44});snprintf(b,sizeof(b),"%s -> %s",placeName(g.daily.origin),placeName(g.tripTo));label(229,b,UI_GOLD);
 snprintf(b,sizeof(b),"Racoes pagas: %u / bolsa: %u",g.daily.rationsPaid,g.rations);label(248,b);
 if(g.daily.phase==rpg::DailyPhase::Tent){carvalhoUi::frame(c,{14,272,212,40});label(288,"Continuar",UI_GOLD);}
 else {carvalhoUi::frame(c,{14,280,212,20});c.fillRect(18,287,204*menu.journey.progress/1000,6,UI_GREEN);}
}
