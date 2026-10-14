#pragma once
#include <stdint.h>
#include <string.h>
#include <initializer_list>
#include <algorithm>

namespace rpg {
// Version 1 is frozen for the lifetime of any saved run. Future generators must
// keep this implementation available while its saves remain supported.
constexpr uint8_t DUNGEON_GENERATOR_VERSION=1;
struct RuinsPoint {uint8_t x=0,y=0;};
struct RuinsLayout {
  char cells[2][81]{};
  RuinsPoint enemies[7],pickups[8];
  uint16_t attempts[2]{};bool fallback[2]{};
};
inline char fixedRuinsCell(unsigned floor,int x,int y){
  static const char* maps[2][9]={
    {"#########","#...#..S#","#.#.#.#.#","#.#...#.#","#.###.#.#","#...#...#","###.#.###","#E......#","#########"},
    {"#########","#...#..B#","#.#.#..T#","#.#.#...#","#.#####D#","#...#...#","###.#.###","#S......#","#########"}};
  return floor<2&&x>=0&&x<9&&y>=0&&y<9?maps[floor][y][x]:'#';
}
inline uint32_t ruinsMix(uint32_t n){n^=n>>16;n*=0x7feb352du;n^=n>>15;n*=0x846ca68bu;n^=n>>16;return n?n:0x6d2b79f5u;}
struct RuinsRandom {
  uint32_t state;
  explicit RuinsRandom(uint32_t seed):state(seed?seed:1){}
  uint32_t next(){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
  unsigned below(unsigned bound){return next()%bound;}
};
inline unsigned ruinsIndex(unsigned x,unsigned y){return y*9+x;}
inline RuinsPoint ruinsPoint(unsigned index){return {uint8_t(index%9),uint8_t(index/9)};}
inline unsigned ruinsRoot(uint8_t* roots,unsigned node){while(roots[node]!=node)node=roots[node];return node;}
inline void ruinsDistances(const char* cells,uint8_t* distance,uint8_t* parent){
  memset(distance,255,81);memset(parent,255,81);uint8_t queue[81];unsigned head=0,tail=0;
  const unsigned start=ruinsIndex(1,7);distance[start]=0;queue[tail++]=start;
  while(head<tail){unsigned here=queue[head++];int x=here%9,y=here/9;
    const int dx[]={0,1,0,-1},dy[]={-1,0,1,0};
    for(unsigned d=0;d<4;++d){int nx=x+dx[d],ny=y+dy[d];if(nx<0||nx>=9||ny<0||ny>=9)continue;unsigned next=ruinsIndex(nx,ny);
      if(cells[next]!='#'&&distance[next]==255){distance[next]=distance[here]+1;parent[next]=here;queue[tail++]=next;}}
  }
}
inline bool ruinsReachableWithout(const char* cells,unsigned blocked,unsigned target){
  char copy[81];memcpy(copy,cells,81);copy[blocked]='#';uint8_t distances[81],parents[81];ruinsDistances(copy,distances,parents);return distances[target]!=255;
}
inline bool ruinsPlace(RuinsRandom& rng,const char* cells,const uint8_t* distance,bool* used,
                       const bool* mainPath,bool offPath,unsigned minimum,unsigned maximum,RuinsPoint& result){
  uint8_t candidates[81];unsigned count=0;
  for(unsigned i=0;i<81;++i)if(cells[i]=='.'&&!used[i]&&(!offPath||!mainPath[i])&&distance[i]>=minimum&&distance[i]<=maximum)candidates[count++]=i;
  if(!count)return false;unsigned selected=candidates[rng.below(count)];used[selected]=true;result=ruinsPoint(selected);return true;
}
inline bool buildRuinsFloor(RuinsLayout& out,unsigned floor,RuinsRandom& rng){
  if(floor!=1)return false;
  char* cells=out.cells[floor];memset(cells,'#',81);
  // A 4x4 room graph, a spanning tree and exactly two extra corridors retain
  // the old 33 traversable cells. The final chamber has exactly one entrance.
  struct Edge {uint8_t a,b,cell;};Edge edges[24];unsigned edgeCount=0;uint8_t roots[16];
  auto allowed=[floor](unsigned n){return !floor||(n!=3&&n!=7);};
  for(unsigned n=0;n<16;++n){roots[n]=n;if(!allowed(n))continue;unsigned x=(n%4)*2+1,y=(n/4)*2+1;cells[ruinsIndex(x,y)]='.';
    if(n%4<3&&allowed(n+1))edges[edgeCount++]={uint8_t(n),uint8_t(n+1),uint8_t(ruinsIndex(x+1,y))};
    if(n/4<3&&allowed(n+4))edges[edgeCount++]={uint8_t(n),uint8_t(n+4),uint8_t(ruinsIndex(x,y+1))};}
  for(unsigned i=edgeCount;i>1;--i){unsigned j=rng.below(i);Edge swap=edges[i-1];edges[i-1]=edges[j];edges[j]=swap;}
  for(unsigned i=0;i<edgeCount;++i){const auto& e=edges[i];unsigned a=ruinsRoot(roots,e.a),b=ruinsRoot(roots,e.b);if(a!=b){roots[a]=b;cells[e.cell]='.';}}
  unsigned extra=0;for(unsigned i=0;i<edgeCount&&extra<2;++i)if(cells[edges[i].cell]=='#'){cells[edges[i].cell]='.';++extra;}
  if(extra!=2)return false;
  cells[ruinsIndex(1,7)]=floor?'S':'E';cells[ruinsIndex(7,1)]=floor?'B':'S';
  if(floor){cells[ruinsIndex(7,2)]='T';cells[ruinsIndex(7,3)]='.';cells[ruinsIndex(7,4)]='D';}
  uint8_t distance[81],parent[81];ruinsDistances(cells,distance,parent);
  unsigned length=distance[ruinsIndex(7,1)];if(length<12||length>16)return false;
  bool used[81]={},mainPath[81]={};uint8_t path[81];unsigned count=0,here=ruinsIndex(7,1);
  while(here!=255){path[count++]=here;mainPath[here]=true;here=parent[here];}
  if(count!=length+1)return false;
  // All three regular guards are compulsory on the lower floor, as before.
  unsigned early=path[length-2],middle=path[length/2],late=path[2];
  for(unsigned guard:{early,middle,late})if(ruinsReachableWithout(cells,guard,ruinsIndex(7,1)))return false;
  used[early]=used[middle]=used[late]=true;out.enemies[3]=ruinsPoint(early);out.enemies[4]=ruinsPoint(middle);out.enemies[5]=ruinsPoint(late);
  out.enemies[6]={7,1};out.pickups[7]={7,2};
  if(!ruinsPlace(rng,cells,distance,used,mainPath,true,8,14,out.pickups[4]))return false;
  if(!ruinsPlace(rng,cells,distance,used,mainPath,false,4,10,out.pickups[5]))return false;
  if(!ruinsPlace(rng,cells,distance,used,mainPath,false,6,12,out.pickups[6]))return false;
  return true;
}
inline void fixedRuinsFloor(RuinsLayout& out,unsigned floor){
  for(unsigned y=0;y<9;++y)for(unsigned x=0;x<9;++x)out.cells[floor][ruinsIndex(x,y)]=fixedRuinsCell(floor,x,y);
  if(!floor){out.enemies[0]={3,7};out.enemies[1]={5,5};out.enemies[2]={7,3};out.pickups[0]={1,5};out.pickups[1]={5,3};out.pickups[2]={5,1};out.pickups[3]={3,1};}
  else{out.enemies[3]={3,7};out.enemies[4]={5,5};out.enemies[5]={7,3};out.enemies[6]={7,1};out.pickups[4]={3,1};out.pickups[5]={1,5};out.pickups[6]={7,5};out.pickups[7]={7,2};}
}
// Release-candidate generator revision R5. Separate graph and placement limits
// retain the fixed maps as validated fallbacks, never weaken resource gates.
inline void buildRuinsEntranceGraph(char* cells,RuinsRandom& rng){
 memset(cells,'#',81);struct Edge {uint8_t a,b,p;};Edge edges[24];uint8_t roots[16];unsigned count=0;
 for(unsigned n=0;n<16;++n){roots[n]=n;unsigned x=n%4*2+1,y=n/4*2+1;cells[ruinsIndex(x,y)]='.';if(x<7)edges[count++]={uint8_t(n),uint8_t(n+1),uint8_t(ruinsIndex(x+1,y))};if(y<7)edges[count++]={uint8_t(n),uint8_t(n+4),uint8_t(ruinsIndex(x,y+1))};}
 for(unsigned i=count;i>1;--i){auto j=rng.below(i);auto tmp=edges[i-1];edges[i-1]=edges[j];edges[j]=tmp;}
 for(auto e:edges){unsigned a=ruinsRoot(roots,e.a),b=ruinsRoot(roots,e.b);if(a!=b){roots[a]=b;cells[e.p]='.';}}
 unsigned extras=0;for(auto e:edges)if(extras<2&&cells[e.p]=='#'){cells[e.p]='.';++extras;}
 cells[64]='E';cells[16]='S';
}
inline bool ruinsPlaceBudgeted(RuinsRandom& rng,const char* cells,const uint8_t* distance,bool* used,const bool* mainPath,bool offPath,unsigned minimum,unsigned maximum,const uint8_t* beforeFirst,const uint8_t* avoid,unsigned limit,bool blocked,RuinsPoint& result,const uint8_t* accessible=nullptr){
 uint8_t candidates[81];unsigned count=0;for(unsigned p=0;p<81;++p)if(cells[p]=='.'&&!used[p]&&(!offPath||!mainPath[p])&&distance[p]>=minimum&&distance[p]<=maximum&&beforeFirst[p]==255&&(!accessible||accessible[p]!=255)&&(blocked?avoid[p]==255:avoid[p]<=limit))candidates[count++]=p;
 if(!count)return false;unsigned p=candidates[rng.below(count)];used[p]=true;result=ruinsPoint(p);return true;
}
inline bool buildRuinsEntrance(RuinsLayout& out,RuinsRandom& rng){
 char* cells=out.cells[0];buildRuinsEntranceGraph(cells,rng);uint8_t distance[81],parent[81];ruinsDistances(cells,distance,parent);unsigned length=distance[16];if(length<12||length>16)return false;
 uint8_t path[81];bool mainPath[81]={};unsigned here=16,count=0;while(here!=255){path[count++]=here;mainPath[here]=true;here=parent[here];}
 uint8_t firsts[5],seconds[16];unsigned fcount=0,scount=0;
 for(unsigned dist=2;dist<=6;++dist){unsigned p=path[length-dist];if(cells[p]=='.'&&!ruinsReachableWithout(cells,p,16))firsts[fcount++]=p;}
 for(unsigned dist=4;dist<length-1;++dist){unsigned p=path[length-dist];char alternative[81];memcpy(alternative,cells,81);alternative[p]='#';uint8_t dd[81],pp[81];ruinsDistances(alternative,dd,pp);if(dd[16]>=20&&dd[16]<=24)seconds[scount++]=p;}
 if(!fcount||!scount)return false;
 // Choose a valid ordered pair rather than imposing a fixed guard coordinate.
 struct Pair{uint8_t first,second;};Pair pairs[80];unsigned n=0;
 for(unsigned f=0;f<fcount;++f)for(unsigned s=0;s<scount;++s)if(distance[seconds[s]]>=distance[firsts[f]]+2)pairs[n++]={firsts[f],seconds[s]};
 if(!n)return false;auto pair=pairs[rng.below(n)];out.enemies[0]=ruinsPoint(pair.first);out.enemies[1]=ruinsPoint(pair.second);
 bool used[81]={};used[pair.first]=used[pair.second]=true;
 if(!ruinsPlace(rng,cells,distance,used,mainPath,true,6,12,out.enemies[2]))return false;
 char alternative[81];memcpy(alternative,cells,81);alternative[pair.second]='#';auto optional=out.enemies[2];alternative[ruinsIndex(optional.x,optional.y)]='#';uint8_t dd[81],pp[81];ruinsDistances(alternative,dd,pp);if(dd[16]<20||dd[16]>24)return false;
 char locked[81];memcpy(locked,cells,81);locked[pair.first]='#';uint8_t beforeFirst[81],parents[81];ruinsDistances(locked,beforeFirst,parents);
 if(beforeFirst[ruinsIndex(optional.x,optional.y)]!=255)return false;
 if(!ruinsPlaceBudgeted(rng,cells,distance,used,mainPath,true,10,14,beforeFirst,dd,16,false,out.pickups[3]))return false;
 if(!ruinsPlaceBudgeted(rng,cells,distance,used,mainPath,false,4,8,beforeFirst,dd,10,false,out.pickups[0]))return false;
 if(!ruinsPlaceBudgeted(rng,cells,distance,used,mainPath,false,4,10,beforeFirst,dd,20,false,out.pickups[1]))return false;
 if(!ruinsPlaceBudgeted(rng,cells,distance,used,mainPath,false,6,12,beforeFirst,dd,22,false,out.pickups[2]))return false;
 auto key=out.pickups[3];return ruinsReachableWithout(cells,ruinsIndex(optional.x,optional.y),ruinsIndex(key.x,key.y));
}
inline bool placeRuinsLowerEncounters(RuinsLayout& out,RuinsRandom& rng){
 const char* cells=out.cells[1];uint8_t distance[81],parent[81];ruinsDistances(cells,distance,parent);unsigned length=distance[16];uint8_t path[81];bool mainPath[81]={};unsigned n=0,here=16;
 while(here!=255){path[n++]=here;mainPath[here]=true;here=parent[here];}
 struct Pair{uint8_t first,second;};Pair pairs[81];unsigned count=0;
 for(unsigned a=2;a<=6;++a)for(unsigned b=a+2;b<=length-4;++b){unsigned first=path[length-a],second=path[length-b];if(cells[first]=='.'&&cells[second]=='.'&&!ruinsReachableWithout(cells,first,16)&&!ruinsReachableWithout(cells,second,16))pairs[count++]={uint8_t(first),uint8_t(second)};}
 if(!count)return false;auto p=pairs[rng.below(count)];bool used[81]={};used[p.first]=used[p.second]=used[34]=true;
 RuinsLayout trial=out;trial.enemies[3]=ruinsPoint(p.first);trial.enemies[4]=ruinsPoint(p.second);
 char blocked[81];memcpy(blocked,cells,81);blocked[p.first]='#';uint8_t beforeFirst[81],parents[81];ruinsDistances(blocked,beforeFirst,parents);
 memcpy(blocked,cells,81);blocked[p.second]='#';uint8_t beforeSecond[81];ruinsDistances(blocked,beforeSecond,parents);
 memcpy(blocked,cells,81);blocked[43]='#';uint8_t beforeDoor[81];ruinsDistances(blocked,beforeDoor,parents);
 if(!ruinsPlaceBudgeted(rng,cells,distance,used,mainPath,true,8,14,beforeFirst,beforeSecond,16,false,trial.pickups[4],beforeDoor))return false;
 if(!ruinsPlaceBudgeted(rng,cells,distance,used,mainPath,false,4,10,beforeFirst,beforeSecond,10,false,trial.pickups[5],beforeDoor))return false;
 if(!ruinsPlaceBudgeted(rng,cells,distance,used,mainPath,false,6,12,beforeFirst,beforeSecond,0,true,trial.pickups[6],beforeDoor))return false;
 for(unsigned i=3;i<5;++i)out.enemies[i]=trial.enemies[i];for(unsigned i=4;i<7;++i)out.pickups[i]=trial.pickups[i];return true;
}
inline RuinsLayout generateRuins(uint32_t seed){
 RuinsLayout out;RuinsRandom rng(ruinsMix(seed^0x9e3779b9u));bool done=false;
 for(unsigned i=0;i<256;++i){out.attempts[0]=uint16_t(i+1);if(buildRuinsEntrance(out,rng)){done=true;break;}}
 if(!done){fixedRuinsFloor(out,0);out.fallback[0]=true;}
 RuinsRandom lower(ruinsMix(seed^(0x9e3779b9u*2)));done=false;
 for(unsigned i=0;i<64;++i){out.attempts[1]=i+1;if(buildRuinsFloor(out,1,lower)){for(unsigned j=0;j<32;++j)if(placeRuinsLowerEncounters(out,lower)){done=true;break;}}if(done)break;}
 if(!done){fixedRuinsFloor(out,1);out.fallback[1]=true;for(unsigned i=0;i<32;++i)if(placeRuinsLowerEncounters(out,lower))break;}return out;
}
// Bounded, allocation-free derived cache. It is keyed solely by immutable run
// identity, never serialized, and never advances either generation or combat RNG.
inline const RuinsLayout& ruinsLayout(uint32_t seed){
  static uint32_t cachedSeed=0;static bool ready=false;static RuinsLayout cache;
  if(!ready||cachedSeed!=seed){cache=generateRuins(seed);cachedSeed=seed;ready=true;}return cache;
}
inline uint32_t nextRuinsSeed(uint32_t previous,uint32_t initial){
  RuinsRandom rng(previous?previous:ruinsMix(initial^0xa511e9b3u));
  if(!previous)return rng.next();
  const auto old=ruinsLayout(previous);
  for(unsigned attempt=0;attempt<64;++attempt){uint32_t seed=rng.next();const auto& candidate=ruinsLayout(seed);
    if(memcmp(old.cells,candidate.cells,sizeof(old.cells)))return seed;}
  return 0; // Refuse before charging if a future change exhausts bounded novelty.
}

}
