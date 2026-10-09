#pragma once
#include "Rules.h"
namespace rpg {
inline bool islandUnlocked(const Game& g){return g.campaignEnding==2&&(g.campaignFlags&127)==127;}
inline bool islandDungeon(const Game& g){return g.dungeonFlags&16;}
inline bool inDungeon(const Game& g){return g.dungeonFlags&1;}
inline unsigned dungeonFloor(const Game& g){return ((g.dungeonFlags>>1)&1)|((g.dungeonFlags>>4)&2);}
inline unsigned dungeonHeading(const Game& g){return (g.dungeonFlags>>2)&3;}
inline int dungeonX(const Game& g){return g.dungeonXY&15;}
inline int dungeonY(const Game& g){return g.dungeonXY>>4;}
constexpr int dungeonDx[]={0,1,0,-1},dungeonDy[]={-1,0,1,0};
inline void dungeonFace(Game& g,unsigned d){g.dungeonFlags=(g.dungeonFlags&~12u)|((d&3)<<2);}
inline void dungeonSetFloor(Game& g,unsigned n){g.dungeonFlags=(g.dungeonFlags&~34u)|((n&1)<<1)|((n&2)<<4);}
inline char dungeonCell(unsigned floor,int x,int y){
 static const char* maps[2][9]={
 {"#########","#...#..S#","#.#.#.#.#","#.#...#.#","#.###.#.#","#...#...#","###.#.###","#E......#","#########"},
 {"#########","#...#..B#","#.#.#..T#","#.#.#...#","#.#####D#","#...#...#","###.#.###","#S......#","#########"}};
 return floor<2&&x>=0&&x<9&&y>=0&&y<9?maps[floor][y][x]:'#';
}
inline char dungeonCell(const Game& g,int x,int y){
 if(!islandDungeon(g))return dungeonCell(dungeonFloor(g),x,y);
 static const char* maps[3][9]={
 {"#########","#...#..S#","#.#.#.#.#","#.#...#.#","#.###.#.#","#...#...#","###.#.###","#E......#","#########"},
 {"#########","#...#..S#","#.#.#...#","#.#.#...#","#.#####D#","#...#...#","###.#.###","#U......#","#########"},
 {"#########","#...#..B#","#.#.#..T#","#.#.#...#","#.#####D#","#...#...#","###.#.###","#U......#","#########"}};
 unsigned n=dungeonFloor(g);return n<3&&x>=0&&x<9&&y>=0&&y<9?maps[n][y][x]:'#';
}
inline bool dungeonWall(const Game& g,int x,int y){char c=dungeonCell(g,x,y);return c=='#'||(c=='D'&&!(g.dungeonLoot&(islandDungeon(g)?128:8)));}
struct DungeonSpawn {uint8_t floor,x,y,id;};
constexpr DungeonSpawn dungeonSpawns[]={{0,3,7,2},{0,5,5,2},{0,7,3,5},{1,3,7,5},{1,5,5,9},{1,7,3,9},{1,7,1,8}};
constexpr DungeonSpawn dungeonPickups[]={{0,1,5,0},{0,5,3,6},{0,5,1,7},{0,3,1,5},{1,3,1,1},{1,1,5,6},{1,7,5,7},{1,7,2,2}};
constexpr DungeonSpawn islandSpawns[]={{0,3,7,20},{0,5,5,21},{0,7,3,22},{1,3,7,22},{1,1,3,23},{1,7,3,20},{2,5,5,23},{2,7,1,24}};
constexpr DungeonSpawn islandPickups[]={{0,1,5,0},{0,5,3,6},{1,3,1,7},{1,7,5,1},{2,3,1,7},{2,7,2,2}};
constexpr DungeonSpawn islandTrapCells[]={{1,3,5,0},{1,5,7,1},{1,7,5,2}};
inline unsigned dungeonSpawnCount(const Game& g){return islandDungeon(g)?8:7;}
inline unsigned dungeonPickupCount(const Game& g){return islandDungeon(g)?6:8;}
inline DungeonSpawn dungeonSpawn(const Game& g,unsigned i){return islandDungeon(g)?islandSpawns[i%8]:dungeonSpawns[i%7];}
inline DungeonSpawn dungeonPickup(const Game& g,unsigned i){return islandDungeon(g)?islandPickups[i%6]:dungeonPickups[i%8];}
inline unsigned dungeonBossIndex(const Game& g){return dungeonSpawnCount(g)-1;}
inline unsigned dungeonChestBit(const Game& g){return islandDungeon(g)?32:128;}
inline bool dungeonBossWon(const Game& g){return inDungeon(g)&&g.phase==Phase::Won&&g.enemyId==(islandDungeon(g)?24:8);}
inline int dungeonEnemyAt(const Game& g,int x,int y){for(unsigned i=0;i<dungeonSpawnCount(g);++i){auto s=dungeonSpawn(g,i);if(!(g.dungeonEnemies&(1u<<i))&&s.floor==dungeonFloor(g)&&s.x==x&&s.y==y)return i;}return -1;}
inline int dungeonEnemyAhead(const Game& g){unsigned d=dungeonHeading(g);return dungeonEnemyAt(g,dungeonX(g)+dungeonDx[d],dungeonY(g)+dungeonDy[d]);}
inline void clearDungeon(Game& g){g.dungeonFlags=g.dungeonXY=g.dungeonLoot=g.dungeonEnemies=g.islandTraps=0;}
inline const char* buyCrystal(Game& g){if(g.phase!=Phase::Home||g.campStage||g.tripStage||inDungeon(g)||g.city!=1)return "Compre nas Ruinas";if(g.crystals>=9)return "Limite: 9 cristais";if(g.p.gold<300)return "Precisa de 300 ouro";g.p.gold-=300;++g.crystals;return nullptr;}
inline const char* enterDungeon(Game& g){if(g.phase!=Phase::Home||g.campStage||g.tripStage||inDungeon(g)||g.city!=1||g.discovery||g.campaignStage||g.eventStage==2||g.clubStage==1||g.clubStage==2)return "Termine a acao atual";if(!g.crystals)return "Precisa de um cristal";--g.crystals;clearDungeon(g);g.dungeonFlags=5;g.dungeonXY=0x71;return nullptr;}
inline const char* enterIsland(Game& g){if(!islandUnlocked(g))return "Entrada indisponivel";if(g.phase!=Phase::Home||g.campStage||g.tripStage||inDungeon(g)||g.discovery||g.campaignStage||g.eventStage==2||g.clubStage==1||g.clubStage==2)return "Termine a acao atual";clearDungeon(g);g.dungeonFlags=21;g.dungeonXY=0x71;return nullptr;}
inline bool dungeonValid(const Game& g){
 if(g.crystals>9||g.dungeonFlags>63||g.islandTraps>7||(g.islandCleared&&!islandUnlocked(g)))return false;
 if(!inDungeon(g))return !g.dungeonFlags&&!g.dungeonXY&&!g.dungeonLoot&&!g.dungeonEnemies&&!g.islandTraps;
 bool secret=islandDungeon(g);if(secret?!islandUnlocked(g)||dungeonFloor(g)>2: g.city!=1||dungeonFloor(g)>1||g.dungeonEnemies>127||g.islandTraps)return false;
 if(g.tripStage||g.campStage||g.campaignStage||g.eventStage==2||g.clubStage==1||g.clubStage==2||dungeonWall(g,dungeonX(g),dungeonY(g))||dungeonEnemyAt(g,dungeonX(g),dungeonY(g))>=0)return false;
 unsigned boss=1u<<dungeonBossIndex(g);if((g.dungeonEnemies&boss)&&!(g.dungeonLoot&(secret?192:8)))return false;
 if(secret&&dungeonFloor(g)>=1&&!(g.dungeonLoot&64))return false;
 if(secret&&dungeonFloor(g)==2&&(g.dungeonLoot&192)!=192)return false;
 if((g.dungeonLoot&dungeonChestBit(g))&&!(g.dungeonEnemies&boss))return false;
 if(g.phase!=Phase::Home){int target=dungeonEnemyAhead(g);if(target<0||g.enemyId!=dungeonSpawn(g,target).id)return false;}
 return true;
}
inline const char* dungeonMove(Game& g,int forward,int side,int turn){
 if(!inDungeon(g)||g.phase!=Phase::Home)return "Termine o combate";unsigned d=(dungeonHeading(g)+turn+4)%4;
 if(turn){dungeonFace(g,d);return nullptr;}
 int x=dungeonX(g)+forward*dungeonDx[d]+side*dungeonDx[(d+1)%4],y=dungeonY(g)+forward*dungeonDy[d]+side*dungeonDy[(d+1)%4];
 if(dungeonWall(g,x,y))return dungeonCell(g,x,y)=='D'?(islandDungeon(g)?"Porta: encontre a alavanca":"Porta: encontre o selo"):"Parede de pedra";
 int target=dungeonEnemyAt(g,x,y);if(target>=0){if(forward<0)d=(d+2)%4;else if(side)d=(d+(side>0?1:3))%4;dungeonFace(g,d);begin(g,dungeonSpawn(g,target).id);return nullptr;}
 g.dungeonXY=x|(y<<4);
 if(islandDungeon(g)&&dungeonFloor(g)==1)for(unsigned i=0;i<3;++i){auto t=islandTrapCells[i];if(x==t.x&&y==t.y&&!(g.islandTraps&(1u<<i))){g.islandTraps|=1u<<i;unsigned roll=1+random(g)%20;int bonus=g.dndProgression?abilityMod(g.attributes[1]):int(luck(g));bool safe=roll==20||(roll!=1&&int(roll)+bonus>=14);g.damage=safe?0:std::min<unsigned>(g.p.hp-1,std::max(1u,unsigned(g.p.maxhp)/8));g.p.hp-=g.damage;}}
 return nullptr;
}
inline bool dungeonLeverVisible(const Game& g,int x,int y){return islandDungeon(g)&&dungeonFloor(g)<2&&x==3&&y==0;}
inline bool dungeonLeverAhead(const Game& g){unsigned d=dungeonHeading(g);return dungeonLeverVisible(g,dungeonX(g)+dungeonDx[d],dungeonY(g)+dungeonDy[d]);}
inline bool dungeonUseLever(Game& g){if(!inDungeon(g)||g.phase!=Phase::Home||!dungeonLeverAhead(g))return false;unsigned bit=64u<<dungeonFloor(g);if(g.dungeonLoot&bit)return false;g.dungeonLoot|=bit;return true;}
inline const char* dungeonCollect(Game& g){
 if(!inDungeon(g)||g.phase!=Phase::Home)return "Termine o combate";const char* notice=nullptr;bool secret=islandDungeon(g);
 for(unsigned i=0;i<dungeonPickupCount(g);++i){auto s=dungeonPickup(g,i);if((g.dungeonLoot&(1u<<i))||s.floor!=dungeonFloor(g)||s.x!=dungeonX(g)||s.y!=dungeonY(g))continue;
  bool chest=i==dungeonPickupCount(g)-1;
  if(chest&&!(g.dungeonEnemies&(1u<<dungeonBossIndex(g))))return "Bau protegido pelo chefe";
  if(s.id==6){if(g.p.life>=99)return "Bolsa de vida cheia";++g.p.life;notice="Pocao de vida coletada";}
  else if(s.id==7){if(g.p.mana>=99)return "Bolsa de mana cheia";++g.p.mana;notice="Pocao de mana coletada";}
  else if(chest){uint8_t pool[4]={0};unsigned count=1;for(unsigned k=1;k<4;++k){uint8_t id=gearOffer((g.p.cls+k)%4,secret?2:1);if(!gearOwns(g.owned,id))pool[count++]=id;}uint8_t id=pool[random(g)%count];if(id){g.owned|=1u<<(id-1);notice=gearName(id);}else {g.p.gold=std::min<uint32_t>(999999u,g.p.gold+(secret?600:150));notice=secret?"Tesouro: +600 ouro":"Tesouro: +150 ouro";}}
  else if(!secret&&i==3)notice="Selo obtido! Porta liberada";
  else {g.p.gold=std::min<uint32_t>(999999u,g.p.gold+(secret?(s.id==0?40:100):(i==0?25:60)));notice="Ouro coletado!";}
  g.dungeonLoot|=1u<<i;return notice;
 }
 return nullptr;
}
inline bool dungeonStairs(Game& g){if(!inDungeon(g)||g.phase!=Phase::Home)return false;char cell=dungeonCell(g,dungeonX(g),dungeonY(g));unsigned floor=dungeonFloor(g);
 if(islandDungeon(g)){if(cell=='S'&&floor<2&&(g.dungeonLoot&(64u<<floor))){dungeonSetFloor(g,floor+1);dungeonFace(g,0);g.dungeonXY=0x71;return true;}if(cell=='U'&&floor){dungeonSetFloor(g,floor-1);dungeonFace(g,2);g.dungeonXY=0x17;return true;}return false;}
 if(cell!='S')return false;bool down=!floor;g.dungeonFlags=down?7:9;g.dungeonXY=down?0x71:0x17;return true;}
inline bool dungeonResolve(Game& g){if(!inDungeon(g)||g.phase==Phase::Home||g.phase==Phase::Hero||g.phase==Phase::Enemy)return false;int e=dungeonEnemyAhead(g);bool won=g.phase==Phase::Won,lost=g.phase==Phase::Lost;if(won&&e>=0){g.dungeonEnemies|=1u<<e;if(unsigned(e)==dungeonBossIndex(g)){if(islandDungeon(g))g.islandCleared=true;else if(g.dungeonClears<255)++g.dungeonClears;}}home(g);if(lost)clearDungeon(g);return true;}
inline bool leaveDungeon(Game& g){if(!inDungeon(g)||g.phase!=Phase::Home)return false;clearDungeon(g);return true;}
}
