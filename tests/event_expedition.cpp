#include "LegacySave.h"
#include "TestHero.h"
#include <queue>
#include <vector>
#include <cassert>
#include <cstdio>
using namespace rpg;
Game checkpoint(Game g){uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,12,b);Game h;uint32_t seq;if(!valid(g))fprintf(stderr,"EXP invalid: level%u flags%u xy%u enemies%u phase%u id%u progress%u eventvalid%d dungeonvalid%d\n",g.p.level,g.dungeonFlags,g.dungeonXY,g.dungeonEnemies,unsigned(g.phase),g.enemyId,g.eventProgress,eventValid(g),dungeonValid(g));assert(valid(g));assert(decode(b,h,seq)==Decode::Ok&&seq==12);encode(h,12,c);assert(!memcmp(b,c,SAVE_SIZE));return h;}
Game expedition(unsigned cls,unsigned level){auto g=create(cls,17);g.p.xp=dndXp[level-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=true;g.city=2;assert(offerEvent(g,20733,10));g.eventKind=3;assert(!acceptEvent(g,27));g.crystals=3;g.dungeonClears=2;g.questId=8;g.questLevel=std::min(5u,level);g.questProgress=1;assert(!eventAct(g));return checkpoint(g);}
void walk(Game& g,int goalx,int goaly){int parent[25];for(int& p:parent)p=-1;int start=dungeonY(g)*5+dungeonX(g),goal=goaly*5+goalx;std::queue<int> todo;todo.push(start);parent[start]=start;while(!todo.empty()){int node=todo.front();todo.pop();for(unsigned d=0;d<4;++d){int x=node%5+dungeonDx[d],y=node/5+dungeonDy[d];if(dungeonWall(g,x,y))continue;int n=y*5+x;if(parent[n]>=0)continue;parent[n]=node;todo.push(n);}}assert(parent[goal]>=0);std::vector<int> path;for(int n=goal;n!=start;n=parent[n])path.push_back(n);for(auto it=path.rbegin();it!=path.rend();++it){int x=*it%5,y=*it/5,d=0;while(d<4&&(dungeonX(g)+dungeonDx[d]!=x||dungeonY(g)+dungeonDy[d]!=y))++d;assert(d<4);while(dungeonHeading(g)!=unsigned(d))assert(!dungeonMove(g,0,0,1));assert(!dungeonMove(g,1,0,0));g=checkpoint(g);if(g.phase==Phase::Hero)return;}}
int main(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned level:{1u,4u,5u,10u,18u}){
  auto g=expedition(cls,level);auto clears=g.dungeonClears,crystals=g.crystals;auto campaign=g.campaignFlags;auto money=g.p.gold;unsigned count=dungeonSpawnCount(g);assert(count==(level<5?2u:3u));assert(!eventReady(g)&&!dungeonCollect(g)&&!dungeonUseLever(g));
  for(unsigned i=0;i<count;++i){auto spawn=dungeonSpawn(g,i);if(spawn.floor!=dungeonFloor(g)){walk(g,3,1);assert(dungeonStairs(g));g=checkpoint(g);}
   walk(g,spawn.x,spawn.y);assert(g.phase==Phase::Hero&&dungeonEnemyAhead(g)==int(i));assert(!dungeonBossWon(g));g.enemyHp=0;auto xp=enemySpec(g.enemyId).xp*10;auto total=totalExperience(g);finish(g);assert(totalExperience(g)==total+xp&&g.gainXp==xp);g=checkpoint(g);assert(resumeEventBattle(g)&&!resumeEventBattle(g));g=checkpoint(g);assert(g.dungeonEnemies&(1u<<i));assert(!eventReady(g));
  }
  walk(g,3,1);while(dungeonHeading(g)!=0)assert(!dungeonMove(g,0,0,1));assert(dungeonLeverAhead(g)&&dungeonUseLever(g)&&!dungeonUseLever(g));g=checkpoint(g);assert(eventReady(g)&&g.eventProgress==1);auto pay=eventGold(g),xp=eventXp(g);auto total=totalExperience(g),gold=g.p.gold;assert(finishEvent(g)&&g.p.gold==gold+pay&&totalExperience(g)==total+xp&&!inDungeon(g)&&g.city==2&&g.dungeonClears==clears&&g.crystals==crystals&&g.questProgress==1&&g.campaignFlags==campaign);g=checkpoint(g);assert(!finishEvent(g)&&!islandUnlocked(g)&&g.p.gold>=money);
 }
 // An early staircase does not skip the finite enemies; corridor and exit are reachable.
 auto g=expedition(0,5);walk(g,3,3);while(dungeonHeading(g)!=0)assert(!dungeonMove(g,0,0,1));assert(dungeonEnemyAhead(g)==1);assert(!dungeonStairs(g));assert(abandonEvent(g)&&finishEvent(g)&&!g.dungeonFlags&&g.crystals==3&&g.dungeonClears==2);g=checkpoint(g);
 // Low-level event never grants Cripta completion or a campaign memory by accident.
 g=expedition(0,1);g.dungeonClears=0;assert(!campaignMemory(g)&&!islandUnlocked(g));uint8_t b[SAVE_SIZE];encode(g,1,b);b[70]|=16;put32(b,124,crc(b,124));Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Corrupt);
 puts("PASS:20 real raycast expeditions,finite2/3 enemies,stairs/cord/XP,save every move and fight,crystal/ordinary dungeon/campaign/contract isolation,arrival/abandon/surprise island");
}
