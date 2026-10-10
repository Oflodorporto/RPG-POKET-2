#include "LegacySave.h"
#include "TestHero.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
struct Store {uint8_t data[2][SAVE_SIZE]{};bool present[2]{},fail=false;Read read(const char* k,uint8_t* b){unsigned i=*k=='b';if(!present[i])return Read::Missing;memcpy(b,data[i],SAVE_SIZE);return Read::Ok;}bool write(const char* k,const uint8_t* b){if(fail)return false;unsigned i=*k=='b';memcpy(data[i],b,SAVE_SIZE);present[i]=true;return true;}};
Game reboot(Game g){uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,7,b);Game h;uint32_t seq;assert(valid(g)&&decode(b,h,seq)==Decode::Ok&&seq==7);encode(h,7,c);assert(!memcmp(b,c,SAVE_SIZE));return h;}
Game hero(unsigned cls,unsigned level,unsigned seed){auto g=create(cls,seed);g.p.xp=dndXp[level-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=g.guildMember=true;return g;}
Game letter(unsigned cls,unsigned level,unsigned kind,unsigned seed=42){auto g=hero(cls,level,seed);g.city=0;assert(offerEvent(g,20733,10));g.eventKind=kind;assert(!acceptEvent(g,49));return reboot(g);}
int main(){
 unsigned seen=0,pairs=0;
 for(unsigned level:{1u,5u,10u,18u})for(unsigned seed=1;seed<80;++seed){auto g=hero(0,level,seed);assert(offerEvent(g,20733,10));unsigned a=g.eventKind;seen|=1u<<a;assert(level>=5||a!=2);g=reboot(g);assert(refuseEvent(g)&&offerEvent(g,20733,22));assert(g.eventKind!=a);++pairs;assert(refuseEvent(g)&&!offerEvent(g,20733,23));}
 assert(seen==7&&pairs==316);
 for(unsigned cls=0;cls<4;++cls)for(unsigned level:{1u,5u,10u,18u}){
  auto g=letter(cls,level,1);auto gold=eventGold(g),xp=eventXp(g);unsigned attempts=0,fights=0;
  assert(!finishEvent(g)&&prepareTrip(g,0)&&!startDiscovery(g)&&startCamp(g,false,false));
  while(!eventReady(g)&&attempts++<40){unsigned before=g.eventProgress;assert(!searchEvent(g));g=reboot(g);
   if(g.phase==Phase::Hero){++fights;assert(g.eventProgress==before&&!finishEvent(g));g.enemyHp=0;finish(g);g=reboot(g);assert(!finishEvent(g)&&resumeEventBattle(g));g=reboot(g);assert(!resumeEventBattle(g));}
   else assert(g.eventProgress==before+1);
  }
  assert(eventReady(g)&&attempts<40&&g.eventProgress==2&&eventGold(g)==gold&&eventXp(g)==xp);assert(searchEvent(g)&&!abandonEvent(g));
  auto total=totalExperience(g),money=g.p.gold;assert(finishEvent(g));g=reboot(g);assert(g.eventStage==3&&g.city==0&&g.p.gold==money+gold&&totalExperience(g)==total+xp&&!finishEvent(g));
 }
 // Recovery cargo is separate from both bag supplies and a region's existing contract.
 auto g=hero(0,5,3);g.city=1;assert(!acceptQuest(g,8));assert(offerEvent(g,20733,10));g.eventKind=1;assert(!acceptEvent(g,36));auto rations=g.rations;
 while(!eventReady(g)){assert(!searchEvent(g));if(g.phase==Phase::Hero){g.enemyHp=0;finish(g);assert(resumeEventBattle(g));}assert(!g.questProgress&&g.rations==rations);g=reboot(g);}
 // Every escort follows a real route; failed D20 starts combat, then resumes the same trip.
 for(unsigned cls=0;cls<4;++cls)for(unsigned level:{5u,10u,18u})for(unsigned safe=0;safe<2;++safe){
  g=hero(cls,level,11);g.city=1;assert(!acceptQuest(g,9));auto quest=g.questId;
  assert(offerEvent(g,20733,10));g.eventKind=2;assert(!acceptEvent(g,36));auto dest=eventDestination(g);assert(prepareTrip(g,(dest+2)%4));assert(!prepareTrip(g,dest));
  g.tripRoll=safe?20:1;g.tripTotal=g.tripRoll+g.tripLuck+g.tripSurvival;g=reboot(g);auto roll=g.tripRoll;
  assert(acceptTrip(g));g=reboot(g);assert(!g.eventProgress&&!finishEvent(g));
  if(!safe){assert(g.phase==Phase::Hero);g.enemyHp=0;finish(g);g=reboot(g);assert(resumeEventBattle(g)&&g.tripStage==3&&g.tripRoll==roll);}
  assert(!g.eventProgress&&arriveTrip(g)&&g.eventProgress==1&&g.city==dest&&!g.questProgress&&g.questId==quest);g=reboot(g);
  assert(eventReady(g)&&!arriveTrip(g));auto money=g.p.gold;auto pay=eventGold(g);assert(finishEvent(g)&&g.city==1&&g.questId==quest&&!g.questProgress&&g.p.gold==money+pay);g=reboot(g);
 }
 // Loss/flee and deliberate abandonment preserve contract and personal supplies, never pay the letter.
 for(unsigned kind=1;kind<=2;++kind)for(unsigned outcome=0;outcome<3;++outcome){
  g=letter(0,5,kind);g.rations=2;auto pay=g.p.gold;
  if(outcome==2){assert(abandonEvent(g));}else {if(kind==1){assert(begin(g,2));}else {assert(!prepareTrip(g,eventDestination(g)));g.tripRoll=1;g.tripTotal=1+g.tripLuck+g.tripSurvival;assert(acceptTrip(g));}
   if(outcome==0){g.p.hp=0;finish(g);}else g.phase=Phase::Fled;
  }
  g=reboot(g);assert(finishEvent(g)&&g.p.hp&&g.p.gold==pay&&!g.tripStage&&g.city==0);g=reboot(g);assert(!finishEvent(g));
 }
 // Atomic mission completion: reboot before a failed write can pay once after recovery, never twice.
 g=letter(0,1,1);g.eventProgress=2;Store store;Journal<Store> journal(store);Game h;assert(journal.load(h)==Load::Empty&&journal.save(g));auto money=g.p.gold;auto reward=eventGold(g);store.fail=true;assert(finishEvent(g)&&!journal.save(g));Journal<Store> old(store);assert(old.load(h)==Load::Ok&&eventReady(h));assert(finishEvent(h));store.fail=false;assert(old.save(h));Journal<Store> paid(store);assert(paid.load(h)==Load::Ok&&!finishEvent(h)&&h.p.gold==money+reward);
 // Actual old26 pilot states retain their old rewards and second-slot status.
 uint8_t b[SAVE_SIZE];uint32_t seq;
 for(unsigned stage=1;stage<=6;++stage){g=testHero(0,42);g.tutorial=true;assert(offerEvent(g,20733,10));g.eventKind=g.eventLevel=0;g.eventOfferSlot=1;if(stage==4)refuseEvent(g);else if(stage!=1){assert(!acceptEvent(g,49));if(stage!=2){if(stage==3){g.enemyHp=0;finish(g);}else if(stage==5){g.p.hp=0;finish(g);}else g.phase=Phase::Fled;assert(finishEvent(g));}}
  encode(g,1,b);legacyFormat(b,26);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Ok&&!h.eventKind&&!h.eventLevel&&h.eventOfferSlot==1&&h.eventStage==stage&&eventGold(h)==eventGold(h.eventTier));
 }
 g=letter(0,5,1);encode(g,1,b);b[95]=1;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);encode(g,1,b);b[97]|=128;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);encode(g,1,b);b[97]=(b[97]&~24)|24;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);encode(g,1,b);legacyFormat(b,26);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);
 puts("PASS: three event types,316 distinct daily pairs; real search/escort, regional combat, D20 and trip resume; contract/supply isolation; loss/flee/abandon, atomic payment, save27/26 compatibility and corruption guards");
}
