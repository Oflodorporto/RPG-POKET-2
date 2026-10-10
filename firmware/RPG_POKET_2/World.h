#pragma once
#include <stdint.h>
#include <algorithm>
#include <stdlib.h>
struct Place {const char* name;int x,y;};
constexpr Place places[]={{"Carvalho",209,200},{"Ruinas",122,176},{"Mares",180,115},{"Aurora",106,98}};
// Connected chain, with bends between landmarks on the user's mapa1 image.
struct Point {int x,y;};
constexpr Point roadPoints[]={{209,200},{180,197},{151,185},{122,176},{135,149},{160,128},{180,115},{157,105},{131,103},{106,98}};
constexpr unsigned placePoint[]={0,3,6,9};
struct Journey {
  bool active=false,escort=false;uint8_t from=0,to=0;uint32_t started=0;unsigned progress=0;
  static constexpr uint32_t duration=3000;
  static constexpr uint32_t escortDuration=7000;
  bool start(uint8_t a,uint8_t b,uint32_t now,bool escorted=false){if(a>3||b>3||a==b||active)return false;from=a;to=b;started=now;progress=0;escort=escorted;active=true;return true;}
  bool tick(uint32_t now){if(!active)return false;uint32_t limit=escort?escortDuration:duration;progress=std::min<uint32_t>(1000u,std::min(limit,uint32_t(now-started))*1000u/limit);if(progress==1000){active=false;return true;}return false;}
  Point positionAt(unsigned fraction)const{int a=placePoint[from],b=placePoint[to],dir=a<b?1:-1;unsigned total=0;
    for(int i=a;i!=b;i+=dir)total+=std::max(abs(roadPoints[i].x-roadPoints[i+dir].x),abs(roadPoints[i].y-roadPoints[i+dir].y));
    unsigned d=std::min(1000u,fraction)*total/1000;for(int i=a;i!=b;i+=dir){Point p=roadPoints[i],q=roadPoints[i+dir];unsigned len=std::max(abs(p.x-q.x),abs(p.y-q.y));if(d<=len)return {p.x+(q.x-p.x)*int(d)/int(len),p.y+(q.y-p.y)*int(d)/int(len)};d-=len;}return roadPoints[b];}
  Point position()const{return positionAt(progress);}
  Point wagonPosition()const{Point p=position(),q=positionAt(progress>180?progress-180:0);if(abs(p.x-q.x)+abs(p.y-q.y)<18){int a=placePoint[from],dir=a<int(placePoint[to])?1:-1;Point next=roadPoints[a+dir];int dx=next.x-roadPoints[a].x,dy=next.y-roadPoints[a].y,span=std::max(abs(dx),abs(dy));q={p.x-dx*22/span,p.y-dy*22/span};}return q;}
};
inline const char* placeName(uint8_t id){return places[id<4?id:0].name;}
