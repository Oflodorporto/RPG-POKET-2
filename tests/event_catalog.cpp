#include "LegacySave.h"
#include "TestHero.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
Game fresh(unsigned cls,unsigned level,unsigned kind,unsigned seed=42){Game g=create(cls,seed);g.p.xp=dndXp[level-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=true;g.city=3;assert(offerEvent(g,20733,21));g.eventKind=kind;assert(!acceptEvent(g,49));return g;}
Game rebootEvent(Game g){uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,91,b);Game h;uint32_t seq;if(!valid(g))fprintf(stderr,"Invalid event kind%u step%u flags%u phase%u city%u roll%u\n",g.eventKind,g.eventProgress,g.eventFlags,unsigned(g.phase),g.city,g.eventRoll);assert(valid(g));assert(decode(b,h,seq)==Decode::Ok&&seq==91);encode(h,91,c);assert(!memcmp(b,c,SAVE_SIZE));return h;}
void winEvent(Game& g){assert(g.phase==Phase::Hero);g.enemyHp=0;finish(g);g=rebootEvent(g);assert(!finishEvent(g));assert(resumeEventBattle(g));g=rebootEvent(g);}
struct Disk{uint8_t b[2][SAVE_SIZE]{};bool present[2]{},fail=false;Read read(const char* k,uint8_t* out){unsigned i=*k=='b';if(!present[i])return Read::Missing;memcpy(out,b[i],SAVE_SIZE);return Read::Ok;}bool write(const char* k,const uint8_t* in){if(fail)return false;unsigned i=*k=='b';present[i]=true;memcpy(b[i],in,SAVE_SIZE);return true;}};
int main(){
 unsigned mask=0;
 for(unsigned lvl:{1u,5u,10u,18u})for(unsigned seed=1;seed<500;++seed){auto g=create(0,seed);g.p.xp=dndXp[lvl-1];levelUp(g);g.tutorial=true;assert(offerEvent(g,20733,10));auto first=g.eventKind;mask|=1u<<first;g=rebootEvent(g);assert(refuseEvent(g)&&offerEvent(g,20733,21)&&g.eventKind!=first);g=rebootEvent(g);assert(eventNight(g)&&refuseEvent(g)&&!offerEvent(g,20733,23));assert(lvl>=5||g.eventKind!=2);}
 assert(mask==255);
 for(unsigned cls=0;cls<4;++cls)for(unsigned lvl:{1u,5u,10u,18u})for(unsigned kind=4;kind<8;++kind)for(unsigned seed=1;seed<=20;++seed){
  auto g=fresh(cls,lvl,kind,seed);g.rations=3;g.p.life=3;g=rebootEvent(g);auto pay=eventGold(g),xp=eventXp(g);unsigned actions=0;
  while(!eventReady(g)&&actions++<24){
   unsigned choice=0;
   if(kind==4&&g.eventProgress==1)choice=seed%2;
   if(kind==5&&g.eventProgress==2)choice=seed%2;
   if(kind==6){if(!g.eventProgress){for(choice=0;choice<3;++choice)if(!(g.eventFlags&(1u<<choice)))break;}else if(g.eventProgress==1)choice=(g.eventTier+(g.eventFlags&8?0:1))%3;}
   if(kind==7&&!g.eventProgress)choice=seed%2;
   assert(!eventAct(g,choice));g=rebootEvent(g);if(g.phase==Phase::Hero)winEvent(g);
  }
  assert(eventReady(g)&&actions<24&&eventGold(g)==pay&&eventXp(g)==xp);auto money=g.p.gold,total=totalExperience(g);auto food=g.rations;assert(eventAct(g)&&!abandonEvent(g));
  Disk disk;Journal<Disk> j(disk);Game old;assert(j.load(old)==Load::Empty&&j.save(g));disk.fail=true;assert(finishEvent(g)&&!j.save(g));Journal<Disk> loaded(disk);assert(loaded.load(old)==Load::Ok&&eventReady(old));assert(finishEvent(old));disk.fail=false;assert(loaded.save(old));Journal<Disk> paid(disk);assert(paid.load(old)==Load::Ok&&!finishEvent(old)&&old.p.gold==money+pay&&totalExperience(old)==total+xp&&old.city==3);assert(old.rations==food+unsigned(kind==7&&food<9));
 }
 // Cargo always advances within eight searches, including worst consecutive battles.
 for(unsigned seed=1;seed<500;++seed){auto cargo=fresh(0,5,1,seed);unsigned searches=0;while(!eventReady(cargo)&&searches++<8){assert(!searchEvent(cargo,eventNight(cargo)));cargo=rebootEvent(cargo);if(cargo.phase==Phase::Hero)winEvent(cargo);}assert(eventReady(cargo)&&searches<=8);}
 // Available decisions reject missing resources without changing RNG, progress or inventory.
 auto g=fresh(0,5,5);g.eventProgress=2;g.p.life=0;auto snapshot=g;assert(eventAct(g,1)&&!memcmp(&snapshot,&g,sizeof(g)));g=fresh(0,5,7);g.rations=0;snapshot=g;assert(eventAct(g,1)&&!memcmp(&snapshot,&g,sizeof(g)));
 // Repeated wrong rune orders cannot farm fights/XP; rereading is free, never completes by itself.
 g=fresh(0,5,6);for(unsigned i=0;i<3;++i){assert(!eventAct(g,i));g=rebootEvent(g);}assert(g.eventProgress==1);unsigned wrong=(g.eventTier+1)%3;assert(!eventAct(g,wrong));winEvent(g);assert(g.eventProgress==1);snapshot=g;assert(eventAct(g,wrong)&&!memcmp(&snapshot,&g,sizeof(g)));assert(!eventAct(g,g.eventTier%3)&&g.eventProgress==2);
 // Every task can fail or be abandoned. No event bonus and no unrelated progression.
 for(unsigned kind=3;kind<8;++kind)for(unsigned outcome=0;outcome<3;++outcome){g=fresh(1,10,kind);g.crystals=4;g.dungeonClears=2;g.questId=8;g.questLevel=5;g.questProgress=1;auto money=g.p.gold;auto clears=g.dungeonClears;
  if(outcome==2)assert(abandonEvent(g));else {if(kind==3){assert(!eventAct(g));assert(!dungeonMove(g,1,0,0)&&!dungeonMove(g,1,0,0)&&g.phase==Phase::Hero);}else assert(begin(g,eventFoe(g)));if(outcome==0){g.p.hp=0;finish(g);}else g.phase=Phase::Fled;}
  g=rebootEvent(g);assert(finishEvent(g)&&g.p.gold==money&&g.p.hp&&g.city==3&&!g.dungeonFlags&&g.crystals==4&&g.dungeonClears==clears&&g.questProgress==1);g=rebootEvent(g);assert(!finishEvent(g));
 }
 // Genuine save27 bit layout: all kinds0..2, six stages and intermediate cargo/route survive.
 uint8_t b[SAVE_SIZE];uint32_t seq;Game h;
 for(unsigned kind=0;kind<3;++kind)for(unsigned stage=1;stage<=6;++stage){g=fresh(0,5,kind);if(kind)g.eventProgress=kind==1?1:0;if(stage==1||stage==4){if(!kind)home(g);g.eventStage=stage;g.eventProgress=0;g.eventOriginCity=g.eventOriginPage=0;g.enemyId=2;g.enemyHp=17;}else if(stage!=2){if(!kind){g.enemyHp=0;finish(g);home(g);}g.eventStage=stage;g.enemyId=2;g.enemyHp=17;}g.eventFlags=g.eventRoll=0;encode(g,9,b);legacyFormat(b,27);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Ok&&h.eventKind==kind&&h.eventStage==stage&&h.eventProgress==g.eventProgress&&eventGold(h)==eventGold(g));h=rebootEvent(h);}
 g=fresh(0,5,7);encode(g,1,b);b[97]|=64;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);encode(g,1,b);b[96]=uint8_t(21<<3)|2;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);encode(g,1,b);b[95]=7;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);
 puts("PASS:8 event types,1996 distinct daily pairs,1280 task paths/all classes/tiers; decisions, bounded repair, rune clues/no-farm, food, atomic payout, failure/abandon, save28/27 migration");
}
