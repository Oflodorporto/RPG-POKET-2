#include "../firmware/RPG_POKET_2/View.h"
#include <fstream>
#include <iostream>
#include <array>
#include <bitset>
#include <vector>
#include <string>
#include <cassert>
#include <cmath>
using Mask=std::bitset<240*172>;
struct Canvas{Mask marked;void fillRect(int x,int y,int w,int h,uint16_t){for(int yy=std::max(0,y);yy<std::min(172,y+h);yy++)for(int xx=std::max(0,x);xx<std::min(240,x+w);xx++)marked.set(yy*240+xx);}};
struct Body {int minx=80,maxx=0,miny=86;enemyArt::Frame frame;std::array<uint16_t,80*86> px;};
Body bodies[29][4];std::vector<Mask> hands;Mask all;
struct Audit {bool ok=true;double pct=0;unsigned core=0,far=0;int top=999,left=999,right=-1;};
Audit check(unsigned id,int canonicalH,int center){Audit a;for(unsigned pose=0;pose<4&&a.ok;pose++)for(unsigned distance:{1u,2u,3u,4u,6u})for(int recoil=0;recoil<=(distance==1?6:0);recoil++){
const auto& b=bodies[id][pose];int h=canonicalH/distance,w=h*80/86;const auto f=enemyArt::fitWorld(b.frame,120+(center-120)/int(distance)+recoil,int(86+86.f/distance),w,h);auto production=enemyScale::world(id,b.frame,120+recoil,int(86+86.f/distance),float(distance));
assert(!memcmp(&f,&production,sizeof(f)));Mask body,core;int top=999,left=999,right=-1;
int actualLeft=f.x+(b.minx*f.w+79)/80,actualRight=f.x+((b.maxx+1)*f.w+79)/80-1,actualTop=f.y+(b.miny*f.h+85)/86; if(actualLeft<2||actualRight>237||actualTop<36){a.ok=false;break;}
for(int x=std::max(0,f.x);x<std::min(240,f.x+f.w);x+=2)for(int y=std::max(34,f.y);y<std::min(172,f.y+f.h);y+=2){int ix=(x-f.x)*80/f.w,iy=(y-f.y)*86/f.h;bool opaque=b.px[iy*80+ix]!=0xf81f;
auto sourceOpaque=[&](int xx,int yy){return xx>=0&&xx<80&&yy>=0&&yy<86&&b.px[yy*80+xx]!=0xf81f;};
bool rim=!opaque&&(sourceOpaque((x-2-f.x)*80/f.w,iy)||sourceOpaque((x+2-f.x)*80/f.w,iy)||sourceOpaque(ix,(y-2-f.y)*86/f.h)||sourceOpaque(ix,(y+2-f.y)*86/f.h));
if(!opaque&&!rim)continue;top=std::min(top,y);left=std::min(left,x);right=std::max(right,x+1);bool protectedCore=opaque&&ix>=20&&ix<60&&iy<62;for(int dy=0;dy<2&&y+dy<172;dy++)for(int dx=0;dx<2&&x+dx<240;dx++){body.set((y+dy)*240+x+dx);if(protectedCore)core.set((y+dy)*240+x+dx);}}
if(!body.any()){a.ok=false;break;}a.top=std::min(a.top,top);a.left=std::min(a.left,left);a.right=std::max(a.right,right);if(top<34){a.ok=false;break;}
unsigned covered=(body&all).count();unsigned cores=(core&all).count();a.core=std::max(a.core,cores);if(cores){a.ok=false;break;}if(distance>1){a.far=std::max(a.far,covered);if(covered){a.ok=false;break;}}
else {double pct=0;if(covered*100<=body.count()*4)pct=covered*100./body.count();else {for(auto& hand:hands){unsigned n=(body&hand).count();pct=std::max(pct,n*100./body.count());if(n*100>body.count()*4)break;}}a.pct=std::max(a.pct,pct);if(pct>4){a.ok=false;break;}}
}return a;}
int main(){std::vector<uint8_t> memory(ART_BYTES);std::array<uint16_t,80*86> work;artMemory=memory.data();enemyArt::workspace=work.data();std::ifstream p("cartao/RPGPOKET/artes.pak",std::ios::binary);p.seekg(16);p.read((char*)artMemory,ART_BYTES);assert(p.gcount()==ART_BYTES);
for(unsigned id=0;id<29;id++)for(unsigned pose=0;pose<4;pose++){auto&b=bodies[id][pose];b.frame=enemyArt::body(id,enemyArt::Pose(pose));auto im=enemyArt::decode(b.frame);std::copy(im.pixels,im.pixels+6880,b.px.begin());for(int y=0;y<86;y++)for(int x=0;x<80;x++)if(b.px[y*80+x]!=0xf81f){b.minx=std::min(b.minx,x);b.maxx=std::max(b.maxx,x);b.miny=std::min(b.miny,y);}}
for(unsigned cls=0;cls<4;cls++)for(unsigned action=0;action<=unsigned(rpg::Action::DivineSmite);action++)for(unsigned phase:{0u,60u,180u,199u,200u,350u,490u,491u,550u,649u,650u,749u,750u,849u,850u,900u,999u,1000u}){auto g=rpg::create(cls,42);ViewState v;v.effect=Effect::Slash;v.effectProgress=phase;v.effectElapsedMs=phase*1100/1000;v.combatResult.hasHeroAction=true;v.combatResult.heroAction=rpg::Action(action);Canvas c;drawDungeonHero(c,g,v);bool exists=false;for(auto&h:hands)if(h==c.marked){exists=true;break;}if(!exists){hands.push_back(c.marked);all|=c.marked;}}
std::cerr<<hands.size()<<" unique hand masks, including conservative cross-class actions\n";std::cout<<"id,name,near_center,canonical_h,pass,near_worst_overlap_upper_bound,core_pixels,far_pixels,top,left,right\n";
for(unsigned id=0;id<29;id++){
const auto& profile=enemyScale::profile(id);int center=120+profile.worldOffset,h=profile.worldHeight;auto result=check(id,h,center);assert(result.ok);

std::cout<<id<<','<<rpg::enemySpec(id).name<<','<<center<<','<<h<<','<<result.ok<<','<<result.pct<<','<<result.core<<','<<result.far<<','<<result.top<<','<<result.left<<','<<result.right<<'\n'<<std::flush;}
std::cerr<<"PASS: scale77 all 29 species x 4 poses x distances 1/2/3/4/6, recoil 0..6; 50 actual hand masks; <=4% body+rim near overlap, zero far/core overlap and full side/top silhouette margins.\n";
}
