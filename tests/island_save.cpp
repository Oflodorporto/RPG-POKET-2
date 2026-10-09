#include "../firmware/RPG_POKET_2/Save.h"
#include "TestHero.h"
#include "LegacySave.h"
#include <cassert>
#include <queue>
#include <cstdio>
using namespace rpg;
Game copy(Game g){if(!valid(g))fprintf(stderr,"invalid flags=%u xy=%u loot=%u foes=%u phase=%u enemy=%u\n",g.dungeonFlags,g.dungeonXY,g.dungeonLoot,g.dungeonEnemies,unsigned(g.phase),g.enemyId);assert(valid(g));uint8_t b[SAVE_SIZE];encode(g,17,b);Game c;uint32_t seq;assert(decode(b,c,seq)==Decode::Ok&&seq==17);assert(c.dungeonFlags==g.dungeonFlags&&c.islandTraps==g.islandTraps&&c.islandCleared==g.islandCleared&&c.seenEnemies==g.seenEnemies);return c;}
Game ready(unsigned cls){auto g=testHero(cls,133);g.campaignFlags=255;g.campaignEnding=2;g.dungeonClears=1;return g;}
void reach(const Game& g,bool v[9][9]){std::queue<std::pair<int,int>> q;q.push({1,7});v[7][1]=true;while(!q.empty()){auto a=q.front();q.pop();for(unsigned d=0;d<4;++d){int x=a.first+dungeonDx[d],y=a.second+dungeonDy[d];if(!dungeonWall(g,x,y)&&!v[y][x]){v[y][x]=true;q.push({x,y});}}}}
int main(){
 auto g=testHero(0,12);for(unsigned e=0;e<2;++e){g.campaignFlags=255;g.campaignEnding=e;assert(!islandUnlocked(g)&&enterIsland(g));}g=ready(0);assert(islandUnlocked(g));assert(!enterIsland(g));g=copy(g);
 for(unsigned f=0;f<3;++f){dungeonSetFloor(g,f);g.dungeonLoot=192;bool v[9][9]={};reach(g,v);assert(v[1][3]&&v[1][7]);for(auto s:islandSpawns)if(s.floor==f)assert(v[s.y][s.x]);for(auto s:islandPickups)if(s.floor==f)assert(v[s.y][s.x]);}
 g=ready(0);assert(!enterIsland(g));g.dungeonXY=0x17;assert(!dungeonStairs(g));g.dungeonXY=0x13;dungeonFace(g,1);assert(!dungeonUseLever(g));dungeonFace(g,0);assert(dungeonUseLever(g)&&!dungeonUseLever(g));g=copy(g);g.dungeonXY=0x17;assert(dungeonStairs(g)&&dungeonFloor(g)==1);g=copy(g);assert(dungeonStairs(g)&&dungeonFloor(g)==0);g=copy(g);
 for(unsigned cls=0;cls<4;++cls){g=ready(cls);assert(!enterIsland(g));g.dungeonLoot=192;
  for(unsigned i=0;i<8;++i){auto s=islandSpawns[i];dungeonSetFloor(g,s.floor);for(unsigned d=0;d<4;++d){int x=s.x-dungeonDx[d],y=s.y-dungeonDy[d];if(!dungeonWall(g,x,y)&&dungeonEnemyAt(g,x,y)<0){g.dungeonXY=x|(y<<4);dungeonFace(g,d);break;}}assert(begin(g,s.id));g=copy(g);unsigned oldGold=g.p.gold;g.enemyHp=0;finish(g);assert(g.phase==Phase::Won&&g.gainXp==enemySpec(s.id).xp&&g.p.gold==oldGold+g.gainGold);auto xp=g.p.xp;auto gold=g.p.gold;finish(g);assert(g.p.xp==xp&&g.p.gold==gold);g=copy(g);assert(dungeonResolve(g)&&!dungeonResolve(g));g=copy(g);}
  assert(g.islandCleared&&g.dungeonEnemies==255&&g.dungeonClears==1);g.dungeonXY=0x27;assert(dungeonCollect(g));g=copy(g);auto gold=g.p.gold;auto owned=g.owned;assert(!dungeonCollect(g)&&gold==g.p.gold&&owned==g.owned);assert(leaveDungeon(g)&&g.islandCleared);g=copy(g);
 }
 for(unsigned seed=1;seed<=50;++seed){g=ready(0);g.randomState=seed;assert(!enterIsland(g));dungeonSetFloor(g,1);g.dungeonLoot=64;g.dungeonXY=0x63;dungeonFace(g,0);g.p.hp=1;assert(!dungeonMove(g,1,0,0));assert(g.islandTraps==1&&g.p.hp==1);g=copy(g);auto state=g.randomState;assert(!dungeonMove(g,-1,0,0)&&!dungeonMove(g,1,0,0)&&g.randomState==state);}
 g=ready(1);g.guildMember=true;g.seenEnemies=(1u<<25)-1;g=copy(g);assert(g.guildMember&&g.seenEnemies==(1u<<25)-1);
 uint8_t b[SAVE_SIZE];encode(g,1,b);legacyFormat(b,23);put32(b,124,crc(b,124));Game c;uint32_t seq;assert(decode(b,c,seq)==Decode::Corrupt);g=testHero(0,2);encode(g,1,b);legacyFormat(b,23);put32(b,124,crc(b,124));assert(decode(b,c,seq)==Decode::Ok);g=copy(g);
 for(unsigned cls=0;cls<4;++cls)for(unsigned id=20;id<=24;++id)for(unsigned seed=1;seed<=20;++seed){auto t=create(cls,seed);t.p.xp=355000;levelUp(t);t.tutorial=true;t.dungeonClears=1;t.campaignFlags=255;t.campaignEnding=2;t.city=3;t.p.gold=5000;for(unsigned gear:{cls*3+3,15u,18u}){assert(!buyGear(t,gear));assert(!equipGear(t,gear));}t.p.hp=t.p.maxhp;t.p.mp=t.p.maxmp;t.p.life=15;t.p.mana=10;assert(!enterIsland(t));assert(begin(t,id));unsigned turns=0;while(t.phase==Phase::Hero||t.phase==Phase::Enemy){assert(++turns<250);if(t.phase==Phase::Enemy)enemy(t);else {auto a=t.p.hp*2<t.p.maxhp&&t.p.life?Action::Life:cls==0&&!powerError(t,Action::ScorchingRay)?Action::ScorchingRay:Action::Attack;assert(!act(t,a));}}if(t.phase!=Phase::Won)fprintf(stderr,"balance cls=%u id=%u seed=%u hp=%u foes=%u\n",cls,id,seed,t.p.hp,t.enemyHp);assert(t.phase==Phase::Won&&t.gainXp==enemySpec(id).xp*10);}
 puts("PASS: island hidden until full restoration,3 connected floors,wall levers,all5 new foes,XP once,save24/old23,trap resume,all classes,boss and unique treasure");
}
