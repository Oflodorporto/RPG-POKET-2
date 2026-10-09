#include "LegacySave.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/Narrative.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace rpg;
Game prepared(unsigned cls,unsigned race){auto g=create(cls,77);g.race=race;g.p.xp=64000;levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=true;g.city=2;g.dungeonClears=1;assert(valid(g));return g;}
Game reboot(const Game& g){assert(valid(g));uint8_t b[SAVE_SIZE];encode(g,41,b);Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&seq==41);return h;}
int main(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race){auto g=prepared(cls,race);
  assert(campaignMission(g)==1&&story::knownChapter(g)==3);
  for(unsigned mission=1;mission<=2;++mission){auto before=g;assert(campaignError(g,mission)==nullptr);assert(!memcmp(&before,&g,sizeof(g)));
   assert(!startCampaign(g,mission));g=reboot(g);assert(g.campaignStage==mission&&!eventSafe(g));
   g.enemyHp=0;finish(g);assert(g.phase==Phase::Won);g=reboot(g);auto xp=g.p.xp,gold=g.p.gold;unsigned bonus=campaignXp(g,mission);
   assert(resolveCampaign(g));assert(g.p.xp==xp+bonus&&g.p.gold==gold+campaignGold(mission)&&g.campaignFlags==((1u<<mission)-1));g=reboot(g);before=g;assert(!resolveCampaign(g)&&startCampaign(g,mission));assert(!memcmp(&before,&g,sizeof(g)));
  }
  assert(!campaignMission(g)&&story::knownChapter(g)==4);assert(!strcmp(story::objective(g).place,"Aurora / Liora Valcer"));
 }
 auto g=prepared(0,0);for(auto phase:{Phase::Lost,Phase::Fled}){assert(!startCampaign(g,1));g.phase=phase;if(phase==Phase::Lost)g.p.hp=0;g=reboot(g);auto gold=g.p.gold,xp=g.p.xp;assert(resolveCampaign(g)&&!g.campaignFlags&&g.p.hp&&g.p.gold==gold&&g.p.xp==xp);g=reboot(g);}
 g=prepared(2,0);g.dungeonClears=0;auto before=g;assert(startCampaign(g,1)&&!memcmp(&before,&g,sizeof(g)));g=prepared(2,0);g.city=0;before=g;assert(startCampaign(g,1)&&!memcmp(&before,&g,sizeof(g)));g=prepared(2,0);g.p.level=9;g.p.mp=g.p.maxmp=totalMana(g);assert(startCampaign(g,1));g=prepared(2,0);assert(startCampaign(g,2));
 // Legacy clear is recognized, but no story flags or rewards are manufactured on load.
 uint8_t b[SAVE_SIZE];g=prepared(2,0);encode(g,42,b);legacyFormat(b,15);put32(b,124,crc(b,124));Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&!h.campaignFlags&&!h.campaignStage&&campaignMemory(h)&&h.p.gold==g.p.gold);
 for(unsigned byte=120;byte<124;++byte){encode(g,42,b);b[byte]=byte==120?2:byte==121?3:byte==122?2:9;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);}
 encode(g,42,b);legacyFormat(b,15);b[120]=1;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);
 // A normal victory at the port never advances the campaign.
 g=prepared(2,0);assert(begin(g,6));g.enemyHp=0;finish(g);assert(home(g)&&!g.campaignFlags&&!g.campaignStage);
 puts("PASS: Mares campaign/all classes/races; gated pure preview; restart/completion/reward once; loss/flee retry; legacy15 migration/reserved bytes; ordinary fights isolated");
}
