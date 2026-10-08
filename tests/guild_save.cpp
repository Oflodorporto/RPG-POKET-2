#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
int main(){
 for(unsigned race=0;race<4;++race)for(unsigned cls=0;cls<4;++cls)for(unsigned color=0;color<8;++color){Game g=testHero(cls,17);g.race=race;g.shirt=color;g.trousers=7-color;uint8_t b[SAVE_SIZE];encode(g,9,b);Game copy;uint32_t seq;assert(decode(b,copy,seq)==Decode::Ok&&copy.race==race&&copy.shirt==color&&copy.trousers==7-color);}
 auto g=testHero(0,9);g.p.gold=99;assert(joinGuild(g)&&g.p.gold==99&&!g.guildMember);g.p.gold=100;assert(!joinGuild(g)&&g.p.gold==0&&g.guildMember);assert(joinGuild(g)&&g.p.gold==0);g.p.level=5;g.p.maxmp=totalMana(g);g.p.mp=g.p.maxmp;g.p.gold=25;assert(!reserveFight(g,18)&&g.p.gold==0);assert(reserveFight(g,19));assert(refundFight(g)&&g.p.gold==25);assert(!refundFight(g)&&g.p.gold==25);assert(reserveFight(g,18));assert(!reserveFight(g,19));assert(startFight(g,19)&&!startFight(g,19));assert(!refundFight(g));assert(settleFight(g,19,1)&&g.p.gold==45);assert(!settleFight(g,19,1)&&g.p.gold==45);
 for(unsigned version=1;version<=6;++version){auto old=testHero(1,123);old.p.gold=137;uint8_t b[SAVE_SIZE];encode(old,3,b);put16(b,4,version);put16(b,6,64);put32(b,60,crc(b,60));Game copy;uint32_t seq;assert(decode(b,copy,seq)==Decode::Ok&&copy.p.gold==137&&copy.race==0&&!copy.guildMember);}
 uint8_t b[SAVE_SIZE];assert(valid(g));encode(g,9,b);for(unsigned off: {64u,65u,66u,67u,68u,69u,80u,91u}){uint8_t corrupted[SAVE_SIZE];memcpy(corrupted,b,SAVE_SIZE);corrupted[off]=255;put32(corrupted,92,crc(corrupted,92));Game copy;uint32_t seq;assert(decode(corrupted,copy,seq)==Decode::Corrupt);}
 puts("PASS: 128 character styles, save1..6 migration, guild exact fee, club reservation/refund/payout duplicate protection, extension validation.");
}
