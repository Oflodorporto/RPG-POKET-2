#pragma once
#include "D20Mesh.h"
#include <algorithm>
#include <cstdlib>
// A small opaque, shaded 3-D die. Fixed stack work, integer math and scanlines;
// no texture, heap allocation, trigonometry, SD read or gameplay RNG/state.
namespace d20Ui {
struct Pixel {int x,y;};
inline int edge(Pixel a,Pixel b,Pixel p){return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);}
template<class C> void line(C& c,Pixel a,Pixel b,uint16_t color){
 int dx=abs(b.x-a.x),sx=a.x<b.x?1:-1,dy=-abs(b.y-a.y),sy=a.y<b.y?1:-1,err=dx+dy;
 for(unsigned n=0;n<200;++n){c.fillRect(a.x,a.y,1,1,color);if(a.x==b.x&&a.y==b.y)break;int e=2*err;if(e>=dy){err+=dy;a.x+=sx;}if(e<=dx){err+=dx;a.y+=sy;}}
}
template<class C> void triangle(C& c,Pixel a,Pixel b,Pixel d,uint16_t color){
 if(a.y>b.y)std::swap(a,b);
 if(b.y>d.y)std::swap(b,d);
 if(a.y>b.y)std::swap(a,b);
 if(a.y==d.y)return;
 // Two edges per scanline, at most 96 rows for the maximum allowed radius.
 for(int y=a.y;y<=d.y;++y){int x=a.x+(d.x-a.x)*(y-a.y)/(d.y-a.y),q;
  if(y<b.y)q=a.x+(b.x-a.x)*(y-a.y)/(b.y-a.y);
  else q=b.y==d.y?b.x:b.x+(d.x-b.x)*(y-b.y)/(d.y-b.y);
  if(x>q)std::swap(x,q);
  c.fillRect(x,y,q-x+1,1,color);
 }
}
inline Vertex rotate(Vertex p,unsigned x,unsigned y,unsigned z){
 auto axis=[](int16_t& a,int16_t& b,unsigned angle){int s=sine[angle&63],co=sine[(angle+16)&63],old=a;a=int16_t((old*co-b*s)/256);b=int16_t((old*s+b*co)/256);};
 axis(p.y,p.z,x);axis(p.z,p.x,y);axis(p.x,p.y,z);return p;
}
inline unsigned stage(uint32_t elapsed){return std::min(14u,unsigned(elapsed/120));}
inline Vertex vertex(unsigned result,unsigned id,bool ready,uint32_t elapsed){
 const auto& p=settled[std::max(1u,std::min(20u,result))-1][id];Vertex v{p[0],p[1],p[2]};
 if(ready)return v;
 static constexpr uint8_t x[]={27,49,70,88,104,117,128,137,144,149,151,150,146,140,133};
 static constexpr uint8_t y[]={9,25,39,51,60,66,70,72,72,71,70,68,66,65,64};
 static constexpr uint8_t z[]={11,18,25,32,39,45,50,54,58,60,62,63,64,64,64};
 unsigned k=stage(elapsed);return rotate(v,x[k],y[k],z[k]);
}
inline uint16_t shade(Vertex a,Vertex b,Vertex c){
 int ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z,vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
 int nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
 // Face winding is outward in mesh coordinates, which have screen-down Y.
 // Fixed upper-left key light; flat-shaded blue enamel with a warm pale rim.
 int light=std::max(0,std::min(15000,(-nx-ny+nz*2)/3));
 int r=14+light*34/15000,g=30+light*66/15000,bv=51+light*82/15000;
 return uint16_t(((r>>3)<<11)|((g>>2)<<5)|(bv>>3));
}
// Compact engraved digits, mapped onto each face's own 3-D local plane.
constexpr uint8_t digits[10][5]={{7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},{7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}};
inline Pixel onFace(Pixel a,Pixel b,Pixel c,int u,int v){
 // a is triangle's top and b/c its baseline. Signed u/v in glyph units.
 // Barycentric placement keeps the engraving within its actual facet.
 return {(a.x+b.x+c.x)/3+(b.x-c.x)*u/72+(b.x+c.x-2*a.x)*v/80,
         (a.y+b.y+c.y)/3+(b.y-c.y)*u/72+(b.y+c.y-2*a.y)*v/80};
}
template<class C>void number(C& c,Pixel a,Pixel b,Pixel d,unsigned value,uint16_t color){
 bool two=value>=10;int width=two?7:3;
 for(int row=0;row<5;++row)for(int col=0;col<width;++col){if(two&&col==3)continue;unsigned digit=two?(col<3?value/10:value%10):value;int bit=two&&col>3?col-4:col;
  if(!(digits[digit][row]&(1<<(2-bit))))continue;
  int u=(col*2-width)*2,v=(row*2-5)*2;
  auto p=onFace(a,b,d,u,v),q=onFace(a,b,d,u+4,v),r=onFace(a,b,d,u+4,v+4),s=onFace(a,b,d,u,v+4);
  triangle(c,p,q,r,color);triangle(c,p,r,s,color);
 }
 if(value==6||value==9)line(c,onFace(a,b,d,-5,13),onFace(a,b,d,5,13),color);
}
template<class C>void draw(C& c,int cx,int cy,int radius,unsigned result,bool ready,uint32_t elapsed){
 radius=std::max(20,std::min(46,radius));Vertex vertices[12];Pixel points[12];
 static constexpr int8_t bounce[]={0,-3,-6,-3,0,-4,-6,-3,0,-2,-3,-1,0,-1,0};
 int bob=ready?0:bounce[stage(elapsed)];
 // Floating die shadow, several short spans rather than a square container.
 c.fillRect(cx-radius*3/5,cy+radius+2,radius*6/5,3,0x0822);c.fillRect(cx-radius/2,cy+radius+5,radius,2,0x1083);
 for(unsigned i=0;i<12;++i){vertices[i]=vertex(result,i,ready,elapsed);points[i]={cx+vertices[i].x*radius/120,cy+bob+vertices[i].y*radius/120};}
 for(const auto& f:faces){Pixel a=points[f.a],b=points[f.b],d=points[f.c];int area=edge(a,b,d);if(area<=0)continue;
  triangle(c,a,b,d,shade(vertices[f.a],vertices[f.b],vertices[f.c]));
  // Numerals turn, shrink and foreshorten with their facets, never float above.
  if(area>radius*radius/9)number(c,a,b,d,f.value,area>radius*radius?0xff39:0xb5b0);
  line(c,a,b,0xce11);line(c,b,d,0xce11);line(c,d,a,0xce11);
 }
}
}
