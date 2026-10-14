#include "../firmware/RPG_POKET_2/Persistence.h"
#include <cassert>
#include <cstdio>
#include <set>
#include <string>
#include <vector>
using namespace rpg;
unsigned topologyChecks=0,snapshots=0,moves=0;
std::vector<uint8_t> bytes(const Game& g){std::vector<uint8_t> out(160);encodePersistent(g,51,0,out.data());return out;}
void reload(Game& g){if(!persistentValid(g))fprintf(stderr,"INVALID snapshot%u seed%u run%u generator%u flags%u xy%u loot%u foes%u phase%u hp%u enemy%u ehp%u\n",snapshots,g.dungeonSeed,g.dungeonRun,g.dungeonGenerator,g.dungeonFlags,g.dungeonXY,g.dungeonLoot,g.dungeonEnemies,unsigned(g.phase),g.p.hp,g.enemyId,g.enemyHp);assert(persistentValid(g));auto b=bytes(g);Game other;uint32_t seq=0;assert(decodePersistent(b.data(),0,other,seq)==Decode::Ok&&seq==51);assert(bytes(other)==b);g=other;++snapshots;}
uint32_t layoutHash(const RuinsLayout& m){return crc(reinterpret_cast<const uint8_t*>(m.cells),162);}
void topology(const RuinsLayout& m){
  for(unsigned f=0;f<2;++f){auto cells=m.cells[f];unsigned walk=0;uint8_t distance[81],parent[81];ruinsDistances(cells,distance,parent);
    for(unsigned y=0;y<9;++y)for(unsigned x=0;x<9;++x){unsigned p=ruinsIndex(x,y);if(!x||x==8||!y||y==8)assert(cells[p]=='#');if(cells[p]!='#'){++walk;assert(distance[p]!=255);}}
    assert(walk==33&&distance[16]>=12&&distance[16]<=16);
    assert(cells[64]==(f?'S':'E')&&cells[16]==(f?'B':'S'));
    std::set<unsigned> objects;
    for(unsigned i=f?3:0;i<(f?7:3);++i){auto p=m.enemies[i];unsigned index=ruinsIndex(p.x,p.y);assert(cells[index]!='#'&&objects.insert(index).second);}
    for(unsigned i=f?4:0;i<(f?8:4);++i){auto p=m.pickups[i];unsigned index=ruinsIndex(p.x,p.y);assert(cells[index]!='#'&&objects.insert(index).second);}
    auto early=m.enemies[f?3:0],middle=m.enemies[f?4:1],last=m.enemies[f?5:2];
    assert(!ruinsReachableWithout(cells,ruinsIndex(early.x,early.y),16));
    assert(ruinsReachableWithout(cells,ruinsIndex(middle.x,middle.y),16)==!bool(f));
    assert(ruinsReachableWithout(cells,ruinsIndex(last.x,last.y),16)==!bool(f));
    if(f){assert(cells[43]=='D'&&cells[25]=='T');assert(!ruinsReachableWithout(cells,43,16)&&!ruinsReachableWithout(cells,43,25));
      for(unsigned i=4;i<7;++i){auto p=m.pickups[i];unsigned at=ruinsIndex(p.x,p.y);assert(ruinsReachableWithout(cells,43,at)&&!ruinsReachableWithout(cells,ruinsIndex(early.x,early.y),at));assert(ruinsReachableWithout(cells,ruinsIndex(middle.x,middle.y),at)==(i<6));}}
    else {char detour[81];memcpy(detour,cells,81);detour[ruinsIndex(middle.x,middle.y)]='#';detour[ruinsIndex(last.x,last.y)]='#';uint8_t dd[81],pp[81];ruinsDistances(detour,dd,pp);assert(dd[16]>=20&&dd[16]<=24);for(unsigned i=0;i<4;++i){auto p=m.pickups[i];unsigned at=ruinsIndex(p.x,p.y);const unsigned limit[]={10,20,22,16};assert(dd[at]<=limit[i]&&!ruinsReachableWithout(cells,ruinsIndex(early.x,early.y),at));}auto key=m.pickups[3];assert(ruinsReachableWithout(cells,ruinsIndex(last.x,last.y),ruinsIndex(key.x,key.y)));}
    ++topologyChecks;
  }
}
Game ready(unsigned cls,uint32_t seed){Game g=create(cls,seed);g.p.xp=dndXp[19];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.city=1;g.crystals=3;g.tutorial=true;return g;}
void win(Game& g){assert(g.phase==Phase::Hero);auto before=g.p.gold;g.enemyHp=0;finish(g);assert(g.phase==Phase::Won&&g.p.gold>before);reload(g);auto once=bytes(g);finish(g);assert(bytes(g)==once);assert(dungeonResolve(g));reload(g);assert(!dungeonResolve(g));}
void walkTo(Game& g,unsigned target){
  assert(g.phase==Phase::Home);unsigned safety=0;
  while(ruinsIndex(dungeonX(g),dungeonY(g))!=target){assert(++safety<100);char cells[81];for(unsigned i=0;i<81;++i)cells[i]=dungeonWall(g,i%9,i/9)?'#':'.';
    // BFS rooted at the target; entrances need not be the current position.
    uint8_t queue[81],distance[81];memset(distance,255,81);unsigned head=0,tail=0;distance[target]=0;queue[tail++]=target;
    while(head<tail){unsigned p=queue[head++];for(unsigned d=0;d<4;++d){int x=int(p%9)+dungeonDx[d],y=int(p/9)+dungeonDy[d];if(x<0||x>8||y<0||y>8)continue;unsigned n=ruinsIndex(x,y);if(cells[n]!='#'&&distance[n]==255){distance[n]=distance[p]+1;queue[tail++]=n;}}}
    unsigned here=ruinsIndex(dungeonX(g),dungeonY(g));assert(distance[here]!=255);bool stepped=false;
    for(unsigned d=0;d<4;++d){unsigned n=ruinsIndex(dungeonX(g)+dungeonDx[d],dungeonY(g)+dungeonDy[d]);if(distance[n]<distance[here]){dungeonFace(g,d);assert(!dungeonMove(g,1,0,0));reload(g);if(g.phase==Phase::Hero){win(g);}++moves;stepped=true;break;}}
    assert(stepped);
  }
}
void runs(){
  for(unsigned cls=0;cls<4;++cls)for(unsigned seed=1;seed<=24;++seed){auto g=ready(cls,seed);auto rng=g.randomState;assert(!enterDungeon(g)&&g.crystals==2&&g.randomState==rng&&g.dungeonGenerator==1);assert(!dungeonWall(g,dungeonX(g)+dungeonDx[dungeonHeading(g)],dungeonY(g)+dungeonDy[dungeonHeading(g)]));reload(g);
    auto immutable=bytes(g);for(unsigned n=0;n<50;++n){for(int y=-1;y<10;++y)for(int x=-1;x<10;++x){dungeonCell(g,x,y);dungeonWall(g,x,y);dungeonEnemyAt(g,x,y);}for(unsigned i=0;i<8;++i)dungeonPickup(g,i);ruinsLayout(seed+1000+n);}
    assert(bytes(g)==immutable&&g.randomState==rng);
    const auto firstSeed=g.dungeonSeed;const auto map=ruinsLayout(firstSeed);const auto firstHash=layoutHash(map);
    for(unsigned f=0;f<2;++f){assert(dungeonFloor(g)==f);
      for(unsigned i=f?3:0;i<(f?7:3);++i){auto p=dungeonSpawn(g,i);walkTo(g,ruinsIndex(p.x,p.y));}
      for(unsigned i=f?4:0;i<(f?8:4);++i){auto p=dungeonPickup(g,i);walkTo(g,ruinsIndex(p.x,p.y));assert(dungeonCollect(g));reload(g);auto once=bytes(g);assert(!dungeonCollect(g)&&bytes(g)==once);}
      if(!f){walkTo(g,16);assert(dungeonStairs(g));reload(g);assert(layoutHash(ruinsLayout(g.dungeonSeed))==firstHash);}
    }
    assert(g.dungeonEnemies==127&&g.dungeonLoot==255&&g.dungeonClears==1);assert(leaveDungeon(g));reload(g);assert(!g.dungeonGenerator&&g.dungeonSeed==firstSeed&&g.dungeonRun==1);
    rng=g.randomState;assert(!enterDungeon(g)&&g.crystals==1&&g.dungeonSeed!=firstSeed&&g.dungeonRun==2&&g.randomState==rng);reload(g);assert(layoutHash(ruinsLayout(g.dungeonSeed))!=firstHash);
    // Defeat ends this run without undoing its cost or collected rewards.
    auto guard=dungeonSpawn(g,0);walkTo(g,ruinsIndex(guard.x,guard.y));auto next=dungeonSpawn(g,2);
    // Find a living enemy and enter combat normally, without walking onto it.
    unsigned target=ruinsIndex(next.x,next.y);unsigned near=target-9;if(dungeonWall(g,near%9,near/9))near=target+9;
    if(!dungeonWall(g,near%9,near/9)){walkTo(g,near);dungeonFace(g,near<target?2:0);assert(!dungeonMove(g,1,0,0));}
    if(g.phase!=Phase::Hero){ // This route may already have defeated it; re-arm only test setup.
      g.dungeonEnemies&=~(1u<<2);unsigned pos=ruinsIndex(next.x,next.y);for(unsigned d=0;d<4;++d){int x=next.x-dungeonDx[d],y=next.y-dungeonDy[d];if(!dungeonWall(g,x,y)&&dungeonEnemyAt(g,x,y)<0){g.dungeonXY=x|(y<<4);dungeonFace(g,d);break;}}assert(dungeonEnemyAhead(g)==2);assert(begin(g,next.id));(void)pos;
    }
    g.p.hp=0;finish(g);reload(g);auto currentSeed=g.dungeonSeed;assert(dungeonResolve(g)&&!inDungeon(g)&&g.p.hp==1&&g.crystals==1&&g.dungeonSeed==currentSeed&&!g.dungeonGenerator);reload(g);
    assert(!enterDungeon(g)&&g.crystals==0&&g.dungeonSeed!=currentSeed&&g.dungeonRun==3);reload(g);
  }
}
void compatibility(){
  for(unsigned f=0;f<2;++f){auto legacy=ready(0,77);assert(!enterDungeon(legacy,0));legacy.dungeonLoot=8;dungeonSetFloor(legacy,f);legacy.dungeonXY=0x71;
    uint8_t b[160];encodePersistentVersion(legacy,81,2,b,29);Game g;uint32_t seq=0;assert(decodePersistent(b,2,g,seq)==Decode::Ok&&!g.dungeonGenerator&&!g.dungeonSeed&&!g.dungeonRun);
    for(unsigned y=0;y<9;++y)for(unsigned x=0;x<9;++x)assert(dungeonCell(g,x,y)==fixedRuinsCell(f,x,y));
    encodePersistent(g,82,2,b);assert(get16(b,4)==30&&decodePersistent(b,2,g,seq)==Decode::Ok&&!g.dungeonGenerator);
    g.dungeonXY=f?0x71:0x17;assert(dungeonStairs(g)&&!g.dungeonGenerator);for(unsigned y=0;y<9;++y)for(unsigned x=0;x<9;++x)assert(dungeonCell(g,x,y)==fixedRuinsCell(1-f,x,y));
    assert(leaveDungeon(g)&&!enterDungeon(g)&&g.dungeonGenerator==1);
  }
  auto g=ready(0,9);assert(!enterDungeon(g));assert(!valid(g));Game bad=g;bad.dungeonRun=0x1000000;assert(!persistentValid(bad));bad=g;bad.dungeonGenerator=2;assert(!persistentValid(bad));bad=g;bad.dungeonRun=0;assert(!persistentValid(bad));bad=g;bad.dungeonSeed=0;assert(!persistentValid(bad));
  uint8_t b[160];encodePersistent(g,1,0,b);b[143]=2;put32(b,156,crc(b,156));uint32_t seq=0;assert(decodePersistent(b,0,bad,seq)==Decode::Unsupported);
  assert(leaveDungeon(g));g.dungeonRun=0xffffff;assert(!enterDungeon(g)&&g.dungeonRun==1);reload(g);
  assert(leaveDungeon(g));g.campaignEnding=2;g.campaignFlags=255;g.dungeonClears=1;assert(!enterIsland(g)&&!g.dungeonGenerator);reload(g);assert(leaveDungeon(g));
  // Invalid legacy reserved bytes cannot be reinterpreted as generation data.
  g=ready(0,22);encodePersistentVersion(g,1,0,b,29);for(unsigned off:{143u,148u,149u,150u,151u,153u,154u,155u}){auto copy=std::vector<uint8_t>(b,b+160);copy[off]=1;put32(copy.data(),156,crc(copy.data(),156));assert(decodePersistent(copy.data(),0,bad,seq)==Decode::Unsupported);}
}
int main(int argc,char** argv){
  unsigned limit=argc>1?unsigned(strtoul(argv[1],nullptr,10)):100000;assert(limit<=100000);
  std::set<uint32_t> hashes;unsigned fallback=0,maxAttempts=0;
  for(unsigned seed=1;seed<=limit;++seed){auto a=generateRuins(seed);topology(a);auto b=generateRuins(seed);assert(!memcmp(a.cells,b.cells,sizeof(a.cells))&&!memcmp(a.enemies,b.enemies,sizeof(a.enemies))&&!memcmp(a.pickups,b.pickups,sizeof(a.pickups)));hashes.insert(layoutHash(a));for(unsigned f=0;f<2;++f){fallback+=a.fallback[f];maxAttempts=std::max(maxAttempts,unsigned(a.attempts[f]));}}
  if(limit>=1000)assert(hashes.size()>limit/2);if(limit==100000)assert(hashes.size()>80000);RuinsLayout fallbackMap;for(unsigned f=0;f<2;++f){fixedRuinsFloor(fallbackMap,f);fallbackMap.fallback[f]=true;}topology(fallbackMap);
  compatibility();runs();printf("PASS procedural6: %u seeds, %u floor topologies, %zu distinct topology pairs, %u fallback floors, max %u/256 entrance or64 lower attempts; %u reload snapshots, %u walking/combat steps; legacy, version, RNG, loot, death, next-entry checks\n",limit,topologyChecks,hashes.size(),fallback,maxAttempts,snapshots,moves);
}
