#include "Persistence.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdio>
using namespace rpg;
std::vector<uint8_t> output;unsigned count=0,active=0;
unsigned geometry(const Game&g){std::vector<uint8_t> bytes;if(inDungeon(g)){auto x=g;for(unsigned f=0;f<(eventExpedition(g)?1u+unsigned(g.eventTier>0):islandDungeon(g)?3u:2u);++f){dungeonSetFloor(x,f);for(int y=0;y<9;++y)for(int z=0;z<9;++z)bytes.push_back(dungeonCell(x,z,y));}for(unsigned n=0;n<dungeonSpawnCount(g);++n){auto p=dungeonSpawn(g,n);bytes.insert(bytes.end(),{p.floor,p.x,p.y,p.id});}for(unsigned n=0;n<dungeonPickupCount(g);++n){auto p=dungeonPickup(g,n);bytes.insert(bytes.end(),{p.floor,p.x,p.y,p.id});}}return crc(bytes.data(),bytes.size());}
void add(const Game&g){if(!persistentValid(g)){fprintf(stderr,"count=%u cls=%u level=%u city=%u phase=%u flags=%u loot=%u enemies=%u xy=%u trip=%u camp=%u event=%u daily=%u dungeonValid=%d dailyValid=%d campaign=%d eventvalid=%d\n",count,g.p.cls,g.p.level,g.city,unsigned(g.phase),g.dungeonFlags,g.dungeonLoot,g.dungeonEnemies,g.dungeonXY,g.tripStage,g.campStage,g.eventStage,unsigned(g.daily.phase),dungeonValid(g),dailyTravelValid(g),campaignValid(g),eventValid(g));}assert(persistentValid(g));unsigned slot=count%3;uint8_t b[172]{};encodePersistent(g,100+count,slot,b);auto next=g;put32(b,160,random(next));put32(b,164,geometry(g));put32(b,168,g.phase==Phase::Enemy?1:0);output.insert(output.end(),b,b+172);++count;active+=inDungeon(g);}
int main(){
 for(unsigned cls=0;cls<4;++cls){
  auto g=create(cls,5123+cls);g.tutorial=g.originStory=true;g.originPage=8;g.race=3-cls;g.shirt=cls+2;g.trousers=7-cls;g.rations=9;g.charms=5;g.charts=3;g.fatigue=37+cls;g.p.gold=123456;
  for(unsigned lv=1;lv<=20;++lv){auto h=g;h.p.xp=dndXp[lv-1];levelUp(h);assert(h.p.level==lv);h.p.hp=h.p.maxhp;h.p.mp=h.p.maxmp;add(h);}
  g.p.xp=dndXp[19];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.p.life=17;g.p.mana=13;g.guildMember=true;g.scrap=9;g.gazuas=7;g.sleepKit=true;g.crystals=9;g.dungeonClears=14;g.ruinsWins=3;g.guardianDefeated=true;g.campaignFlags=255;g.campaignEnding=2;g.islandCleared=true;g.seenEnemies=(1u<<29)-1;g.questId=8;g.questLevel=5;g.questProgress=1;
  g.city=3;for(unsigned gear:{cls*3+3,15u,18u}){assert(!buyGear(g,gear));assert(!equipGear(g,gear));}assert(!learnTough(g));for(unsigned a=0;a<3;++a)g.forge[a]=3;g.p.maxmp=totalMana(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;
  if(cls==1){assert(!swearDevotion(g));g.laySpent=17;g.channelSpent=1;}if(cls==2){g.windSpent=1;g.surgeSpent=1;}add(g);
  auto d=g;d.city=1;assert(!enterDungeon(d));add(d);
  for(unsigned floor=0;floor<2;++floor)for(unsigned mask:{0u,63u,127u})for(unsigned loot:{0u,8u,127u,255u})for(unsigned y=1;y<8;++y)for(unsigned x=1;x<8;++x)for(unsigned heading=0;heading<4;++heading){auto h=d;dungeonSetFloor(h,floor);dungeonFace(h,heading);h.dungeonXY=x|(y<<4);h.dungeonEnemies=mask;h.dungeonLoot=loot;if(persistentValid(h))add(h);}
  assert(!dungeonMove(d,1,0,0));assert(!dungeonMove(d,1,0,0));assert(d.phase==Phase::Hero);add(d);assert(!act(d,Action::Defensive));add(d);enemy(d);add(d);d.enemyHp=0;finish(d);add(d);assert(dungeonResolve(d));add(d);
  auto island=g;island.city=3;assert(!enterIsland(island));add(island);island.dungeonLoot=192;island.dungeonEnemies=255;island.islandTraps=7;dungeonSetFloor(island,2);add(island);
  auto travel=g;travel.city=0;assert(!prepareDailyTrip(travel,3));add(travel);assert(acceptDailyRoad(travel));add(travel);
  auto event=g;event.city=2;assert(offerEvent(event,20733,10));event.eventKind=3;assert(!acceptEvent(event,27));assert(!eventAct(event));add(event);
 }
 std::ofstream f("save29-fixtures.bin",std::ios::binary);assert(f);f.write((const char*)output.data(),output.size());assert(f);printf("Generated %u canonical exact baseline Save29 records (%u active dungeon), 4classes20levels/fullprogression/oldRuins/island/event/daily; %zu bytes.\n",count,active,output.size());
}
