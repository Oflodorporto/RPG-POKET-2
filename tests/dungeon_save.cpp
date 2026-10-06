#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <queue>
#include <cstdio>
using namespace rpg;
Game roundtrip(const Game& g){assert(valid(g));uint8_t bytes[SAVE_SIZE];encode(g,42,bytes);Game copy;uint32_t seq;assert(decode(bytes,copy,seq)==Decode::Ok&&seq==42);assert(copy.dungeonFlags==g.dungeonFlags&&copy.dungeonXY==g.dungeonXY&&copy.dungeonLoot==g.dungeonLoot&&copy.dungeonEnemies==g.dungeonEnemies);return copy;}
void reach(Game g,int sx,int sy,bool visited[9][9]){std::queue<std::pair<int,int>> q;q.push({sx,sy});visited[sy][sx]=true;while(!q.empty()){auto p=q.front();q.pop();for(int d=0;d<4;++d){int x=p.first+dungeonDx[d],y=p.second+dungeonDy[d];if(!dungeonWall(g,x,y)&&!visited[y][x]){visited[y][x]=true;q.push({x,y});}}}}
int main(){
 Game g=create(0,19);g.city=1;g.p.gold=299;assert(buyCrystal(g)&&g.p.gold==299);assert(enterDungeon(g));g.p.gold=600;assert(!buyCrystal(g)&&g.crystals==1&&g.p.gold==300);assert(!enterDungeon(g)&&g.crystals==0);assert(enterDungeon(g));g=roundtrip(g);
 bool v[9][9]={};reach(g,1,7,v);assert(v[1][7]&&v[1][3]);for(auto s:dungeonPickups)if(s.floor==0)assert(v[s.y][s.x]);
 assert(dungeonMove(g,1,0,0));assert(dungeonX(g)==1&&dungeonY(g)==7);assert(!dungeonMove(g,0,0,1));assert(!dungeonMove(g,1,0,0));assert(!dungeonMove(g,1,0,0)&&g.phase==Phase::Hero&&g.enemyId==2);g=roundtrip(g);
 unsigned gold=g.p.gold;g.enemyHp=0;finish(g);assert(g.phase==Phase::Won&&g.ruinsWins==0);g=roundtrip(g);unsigned rewarded=g.p.gold;finish(g);assert(g.p.gold==rewarded&&rewarded>gold);assert(dungeonResolve(g));g=roundtrip(g);assert(g.dungeonEnemies&1);assert(!dungeonResolve(g));assert(!dungeonMove(g,1,0,0)&&dungeonX(g)==3);
 g.dungeonXY=0x13;assert(dungeonCollect(g)&&g.dungeonLoot&8);g=roundtrip(g);g.dungeonXY=0x17;assert(dungeonStairs(g)&&dungeonFloor(g)==1);
 assert(dungeonX(g)==1&&dungeonY(g)==7);g=roundtrip(g);bool v2[9][9]={};reach(g,1,7,v2);assert(v2[1][7]);
 Game locked=g;locked.dungeonLoot=0;bool vl[9][9]={};reach(locked,1,7,vl);assert(!vl[1][7]);
 for(auto s:dungeonPickups)if(s.floor==1)assert(v2[s.y][s.x]);
 g.dungeonXY=0x47;g.dungeonFlags=3;assert(begin(g,9));g=roundtrip(g);g.enemyHp=0;finish(g);g=roundtrip(g);assert(dungeonResolve(g)&&g.dungeonEnemies&32);
 g.dungeonXY=0x27;assert(begin(g,8));g=roundtrip(g);g.enemyHp=0;finish(g);g=roundtrip(g);assert(dungeonResolve(g)&&g.dungeonClears==1);g=roundtrip(g);
 assert(dungeonCollect(g)&&g.dungeonLoot&128&&gearOwns(g.owned,gearOffer(g.p.cls,1)));auto owned=g.owned;gold=g.p.gold;assert(!dungeonCollect(g)&&g.owned==owned&&g.p.gold==gold);g=roundtrip(g);
 assert(leaveDungeon(g)&&!inDungeon(g)&&g.dungeonClears==1);g=roundtrip(g);
 Game legacy=create(0,99);legacy.city=1;legacy.p.gold=137;uint8_t bytes[SAVE_SIZE];encode(legacy,1,bytes);put16(bytes,4,8);put32(bytes,92,crc(bytes,92));Game old;uint32_t seq;assert(decode(bytes,old,seq)==Decode::Ok&&!old.crystals&&!old.dungeonFlags&&old.p.gold==137);
 bytes[70]=1;put32(bytes,92,crc(bytes,92));assert(decode(bytes,old,seq)==Decode::Corrupt);put16(bytes,4,10);assert(decode(bytes,old,seq)==Decode::Unsupported);
 g=create(1,4);g.city=1;g.ruinsWins=3;assert(begin(g,3));g.enemyHp=0;finish(g);assert(g.crystals==1);home(g);g.crystals=9;assert(begin(g,3));g.enemyHp=0;finish(g);assert(g.crystals==9);
 for(unsigned cls=0;cls<4;++cls){g=create(cls,4);g.city=1;g.crystals=1;assert(!enterDungeon(g));g.dungeonXY=0x72;g.dungeonFlags=5;assert(begin(g,2));g.p.hp=0;finish(g);g=roundtrip(g);assert(dungeonResolve(g)&&!inDungeon(g)&&g.p.hp==1);assert(valid(g));}
 puts("PASS: crystals, collision, reachability, seal gate, 2 floors, seven enemies, boss, unique loot, save9/import8, defeat, all classes");
 return 0;
}
