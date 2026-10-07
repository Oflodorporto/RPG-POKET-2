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
  bool active=false;uint8_t from=0,to=0;uint32_t started=0;unsigned progress=0;
  static constexpr uint32_t duration=3000;
  bool start(uint8_t a,uint8_t b,uint32_t now){if(a>3||b>3||a==b||active)return false;from=a;to=b;started=now;progress=0;active=true;return true;}
  bool tick(uint32_t now){if(!active)return false;progress=std::min<uint32_t>(1000u,std::min(duration,uint32_t(now-started))*1000u/duration);if(progress==1000){active=false;return true;}return false;}
  Point position()const{int a=placePoint[from],b=placePoint[to],dir=a<b?1:-1;unsigned total=0;
    for(int i=a;i!=b;i+=dir)total+=std::max(abs(roadPoints[i].x-roadPoints[i+dir].x),abs(roadPoints[i].y-roadPoints[i+dir].y));
    unsigned d=progress*total/1000;for(int i=a;i!=b;i+=dir){Point p=roadPoints[i],q=roadPoints[i+dir];unsigned len=std::max(abs(p.x-q.x),abs(p.y-q.y));if(d<=len)return {p.x+(q.x-p.x)*int(d)/int(len),p.y+(q.y-p.y)*int(d)/int(len)};d-=len;}return roadPoints[b];}
};
inline const char* placeName(uint8_t id){return places[id<4?id:0].name;}
