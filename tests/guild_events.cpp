#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
struct Mem {uint8_t b[2][SAVE_SIZE]{};bool has[2]={},fail=false;Read read(const char* k,uint8_t* out){unsigned i=*k=='b';if(!has[i])return Read::Missing;memcpy(out,b[i],SAVE_SIZE);return Read::Ok;}bool write(const char* k,const uint8_t* in){if(fail)return false;unsigned i=*k=='b';memcpy(b[i],in,SAVE_SIZE);has[i]=true;return true;}};
Game reboot(Game g){uint8_t b[SAVE_SIZE];encode(g,7,b);Game copy;uint32_t seq;assert(valid(g)&&decode(b,copy,seq)==Decode::Ok&&seq==7);return copy;}
int main(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city)for(unsigned tier=0;tier<4;++tier)for(unsigned outcome=0;outcome<3;++outcome){auto g=testHero(cls,42);g.tutorial=true;g.city=city;g.p.level=tier==0?3:tier==1?5:tier==2?10:18;g.p.maxmp=totalMana(g);g.p.mp=g.p.maxmp;g.p.gold=123;acceptQuest(g,1);auto quest=g.questProgress;auto ruin=g.ruinsWins;auto ration=g.rations;auto crystal=g.crystals;
 assert(!offerEvent(g,20733,8));assert(offerEvent(g,20733,9));auto offered=reboot(g);assert(!offerEvent(g,20733,22)&&!offerEvent(g,20732,10));assert(!acceptEvent(g,49)&&g.city==tier&&g.enemyId==10+tier);g=reboot(g);assert(!offerEvent(g,20734,10));assert(g.eventTier==tier);assert(g.questProgress==quest&&g.ruinsWins==ruin&&g.rations==ration&&g.crystals==crystal);
 if(outcome==0){g.enemyHp=0;finish(g);}else if(outcome==1){g.p.hp=0;finish(g);}else g.phase=Phase::Fled;
 g=reboot(g);auto gold=g.p.gold,xp=g.p.xp;assert(g.p.life==0&&g.p.mana==2&&g.questProgress==quest&&g.ruinsWins==ruin);
 Mem mem;Journal<Mem> j(mem);Game loaded;assert(j.load(loaded)==Load::Empty&&j.save(g));mem.fail=true;assert(finishEvent(g)&&!j.save(g));assert(g.eventStage==3+0||g.eventStage==5||g.eventStage==6);assert(g.city==city&&g.eventOriginPage==49&&g.p.hp>=1&&g.p.gold==gold+(outcome==0?eventGold(tier):0)&&g.p.xp==xp+(outcome==0?eventXp(tier):0));assert(!finishEvent(g));
 // Failed commit reboots to the result; successful commit reboots to a paid terminal state.
 Journal<Mem> oldBoot(mem);assert(oldBoot.load(loaded)==Load::Ok&&loaded.eventStage==2);assert(finishEvent(loaded));mem.fail=false;assert(oldBoot.save(loaded));Journal<Mem> finalBoot(mem);assert(finalBoot.load(loaded)==Load::Ok&&!finishEvent(loaded));assert(loaded.p.gold==g.p.gold&&loaded.p.xp==g.p.xp);assert(!offerEvent(loaded,20732,10));assert(offerEvent(loaded,20734,10));assert(refuseEvent(loaded)&&!refuseEvent(loaded)&&!offerEvent(loaded,20734,22));reboot(loaded);
 }
 auto g=testHero(0,42);g.tutorial=true;g.tripStage=1;assert(!offerEvent(g,20733,10));g=testHero(0,42);g.tutorial=true;g.campStage=1;assert(!offerEvent(g,20733,10));g=testHero(0,42);assert(!offerEvent(g,20733,10));g.tutorial=true;offerEvent(g,20733,10);begin(g,2);assert(acceptEvent(g,2)&&g.eventStage==1&&g.enemyId==2);
 // Genuine save10 size/CRC, including occupied camp bytes, migrates without an event.
 g=testHero(0,55);g.sleepKit=true;g.rations=3;g.city=1;g.p.hp=1;startCamp(g,true,true);uint8_t b[SAVE_SIZE];encode(g,19,b);put16(b,4,10);put16(b,6,96);put32(b,92,crc(b,92));Game copy;uint32_t seq;assert(decode(b,copy,seq)==Decode::Ok&&!copy.eventDay&&copy.campStage==g.campStage&&copy.campRoll==g.campRoll&&copy.sleepKit&&copy.rations==2);
 g=testHero(0,42);g.tutorial=true;offerEvent(g,20733,10);acceptEvent(g,9);encode(g,1,b);b[124]^=1;assert(decode(b,copy,seq)==Decode::Corrupt);encode(g,1,b);b[99]=255;put32(b,124,crc(b,124));assert(decode(b,copy,seq)==Decode::Corrupt);encode(g,1,b);b[100]=1;put32(b,124,crc(b,124));assert(decode(b,copy,seq)==Decode::Corrupt);encode(g,1,b);put16(b,4,16);assert(decode(b,copy,seq)==Decode::Unsupported);
 puts("PASS: 192 mission outcomes, all classes/cities/tiers; daily limits and clock rollback; safe accept, refusal, resume, atomic reward/return and failed-save reboot; save10 camp migration, CRC and future protection");
}
