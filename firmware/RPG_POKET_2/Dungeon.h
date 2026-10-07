#pragma once
#include "Rules.h"
namespace rpg {
inline bool inDungeon(const Game& g){return g.dungeonFlags&1;}
inline unsigned dungeonFloor(const Game& g){return (g.dungeonFlags>>1)&1;}
inline unsigned dungeonHeading(const Game& g){return (g.dungeonFlags>>2)&3;}
inline int dungeonX(const Game& g){return g.dungeonXY&15;}
inline int dungeonY(const Game& g){return g.dungeonXY>>4;}
constexpr int dungeonDx[]={0,1,0,-1},dungeonDy[]={-1,0,1,0};
inline char dungeonCell(unsigned floor,int x,int y){
  static const char* maps[2][9]={
    {"#########","#...#..S#","#.#.#.#.#","#.#...#.#","#.###.#.#","#...#...#","###.#.###","#E......#","#########"},
    {"#########","#...#..B#","#.#.#..T#","#.#.#...#","#.#####D#","#...#...#","###.#.###","#S......#","#########"}};
  return floor<2&&x>=0&&x<9&&y>=0&&y<9?maps[floor][y][x]:'#';
}
inline bool dungeonWall(const Game& g,int x,int y){char c=dungeonCell(dungeonFloor(g),x,y);return c=='#'||(c=='D'&&!(g.dungeonLoot&8));}
struct DungeonSpawn {uint8_t floor,x,y,id;};
constexpr DungeonSpawn dungeonSpawns[]={{0,3,7,2},{0,5,5,2},{0,7,3,5},{1,3,7,5},{1,5,5,9},{1,7,3,9},{1,7,1,8}};
constexpr DungeonSpawn dungeonPickups[]={{0,1,5,0},{0,5,3,6},{0,5,1,7},{0,3,1,5},{1,3,1,1},{1,1,5,6},{1,7,5,7},{1,7,2,2}};
inline int dungeonEnemyAt(const Game& g,int x,int y){for(unsigned i=0;i<7;++i){auto s=dungeonSpawns[i];if(!(g.dungeonEnemies&(1u<<i))&&s.floor==dungeonFloor(g)&&s.x==x&&s.y==y)return i;}return -1;}
inline int dungeonEnemyAhead(const Game& g){unsigned d=dungeonHeading(g);return dungeonEnemyAt(g,dungeonX(g)+dungeonDx[d],dungeonY(g)+dungeonDy[d]);}
inline void clearDungeon(Game& g){g.dungeonFlags=g.dungeonXY=g.dungeonLoot=g.dungeonEnemies=0;}
inline const char* buyCrystal(Game& g){if(g.phase!=Phase::Home||g.tripStage||inDungeon(g)||g.city!=1)return "Compre nas Ruinas";if(g.crystals>=9)return "Limite: 9 cristais";if(g.p.gold<300)return "Precisa de 300 ouro";g.p.gold-=300;++g.crystals;return nullptr;}
inline const char* enterDungeon(Game& g){if(g.phase!=Phase::Home||g.tripStage||inDungeon(g)||g.city!=1||g.clubStage==1||g.clubStage==2)return "Termine a acao atual";if(!g.crystals)return "Precisa de um cristal";--g.crystals;clearDungeon(g);g.dungeonFlags=5;g.dungeonXY=0x71;return nullptr;}
inline bool dungeonValid(const Game& g){
  if(g.crystals>9||g.dungeonFlags>15||g.dungeonEnemies>127)return false;
  if(!inDungeon(g))return !g.dungeonFlags&&!g.dungeonXY&&!g.dungeonLoot&&!g.dungeonEnemies;
  if(g.city!=1||g.tripStage||g.clubStage==1||g.clubStage==2||dungeonWall(g,dungeonX(g),dungeonY(g))||dungeonEnemyAt(g,dungeonX(g),dungeonY(g))>=0)return false;
  if((g.dungeonEnemies&64)&&!(g.dungeonLoot&8))return false;
  if((g.dungeonLoot&128)&&!(g.dungeonEnemies&64))return false;
  if(g.phase!=Phase::Home){int target=dungeonEnemyAhead(g);if(target<0||g.enemyId!=dungeonSpawns[target].id)return false;}
  return true;
}
inline const char* dungeonMove(Game& g,int forward,int side,int turn){
  if(!inDungeon(g)||g.phase!=Phase::Home)return "Termine o combate";
  unsigned d=(dungeonHeading(g)+turn+4)%4;
  if(turn){g.dungeonFlags=(g.dungeonFlags&3)|(d<<2);return nullptr;}
  int x=dungeonX(g)+forward*dungeonDx[d]+side*dungeonDx[(d+1)%4];
  int y=dungeonY(g)+forward*dungeonDy[d]+side*dungeonDy[(d+1)%4];
  if(dungeonWall(g,x,y))return dungeonCell(dungeonFloor(g),x,y)=='D'?"Porta: encontre o selo":"Parede de pedra";
  int target=dungeonEnemyAt(g,x,y);
  if(target>=0){if(forward<0)d=(d+2)%4;else if(side)d=(d+(side>0?1:3))%4;g.dungeonFlags=(g.dungeonFlags&3)|(d<<2);begin(g,dungeonSpawns[target].id);return nullptr;}
  g.dungeonXY=x|(y<<4);return nullptr;
}
inline const char* dungeonCollect(Game& g){
  if(!inDungeon(g)||g.phase!=Phase::Home)return "Termine o combate";
  const char* chestNotice=nullptr;
  for(unsigned i=0;i<8;++i){auto s=dungeonPickups[i];if((g.dungeonLoot&(1u<<i))||s.floor!=dungeonFloor(g)||s.x!=dungeonX(g)||s.y!=dungeonY(g))continue;
    if(i==7&&!(g.dungeonEnemies&64))return "Bau protegido pelo chefe";
    if(i==1||i==5){if(g.p.life>=99)return "Bolsa de vida cheia";++g.p.life;}
    else if(i==2||i==6){if(g.p.mana>=99)return "Bolsa de mana cheia";++g.p.mana;}
    else if(i==7){uint8_t id=gearOffer(g.p.cls,1);if(gearOwns(g.owned,id)){g.p.gold=std::min<uint32_t>(999999u,g.p.gold+50);chestNotice="Duplicado: +50 ouro";}else {g.owned|=1u<<(id-1);chestNotice=gearName(id);}}
    else if(i!=3)g.p.gold=std::min<uint32_t>(999999u,g.p.gold+(i==0?25u:60u));
    g.dungeonLoot|=1u<<i;return i==3?"Selo obtido! Porta liberada":i==7?chestNotice:i==1||i==5?"Pocao de vida coletada":i==2||i==6?"Pocao de mana coletada":"Ouro coletado!";
  }
  return nullptr;
}
inline bool dungeonStairs(Game& g){if(!inDungeon(g)||g.phase!=Phase::Home||dungeonCell(dungeonFloor(g),dungeonX(g),dungeonY(g))!='S')return false;bool down=!dungeonFloor(g);g.dungeonFlags=down?7:9;g.dungeonXY=down?0x71:0x17;return true;}
inline bool dungeonResolve(Game& g){if(!inDungeon(g)||g.phase==Phase::Home||g.phase==Phase::Hero||g.phase==Phase::Enemy)return false;int e=dungeonEnemyAhead(g);bool won=g.phase==Phase::Won,lost=g.phase==Phase::Lost;if(won&&e>=0){g.dungeonEnemies|=1u<<e;if(e==6&&g.dungeonClears<255)++g.dungeonClears;}home(g);if(lost)clearDungeon(g);return true;}
inline bool leaveDungeon(Game& g){if(!inDungeon(g)||g.phase!=Phase::Home)return false;clearDungeon(g);return true;}
}
