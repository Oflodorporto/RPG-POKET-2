#include "LegacySave.h"
#include "../firmware/RPG_POKET_2/Narrative.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
using namespace rpg;
struct Store{bool fail=false;std::map<std::string,std::vector<uint8_t>> data;Read read(const char* k,uint8_t* b){auto i=data.find(k);if(i==data.end())return Read::Missing;memcpy(b,i->second.data(),SAVE_SIZE);return Read::Ok;}bool write(const char* k,const uint8_t* b){if(fail)return false;data[k]=std::vector<uint8_t>(b,b+SAVE_SIZE);return true;}};
Game prepared(unsigned cls,unsigned race){auto g=create(cls,77);g.race=race;g.p.xp=265000;levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=true;g.city=3;g.dungeonClears=1;g.campaignFlags=3;g.seenEnemies=(1u<<6)|(1u<<8);assert(g.p.level==18&&valid(g));return g;}
Game reboot(const Game& g){uint8_t b[SAVE_SIZE];encode(g,11,b);Game h;uint32_t seq;assert(valid(g)&&decode(b,h,seq)==Decode::Ok&&seq==11);uint8_t c[SAVE_SIZE];encode(h,11,c);assert(!memcmp(b,c,SAVE_SIZE));return h;}
int main(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race){auto g=prepared(cls,race);auto before=g;assert(campaignMission(g)==3&&campaignContact(g)&&!campaignError(g,3));assert(!memcmp(&g,&before,sizeof(g)));assert(!startCampaign(g,3)&&g.enemyId==7&&g.campaignStage==3&&(g.seenEnemies&(1u<<7))&&!eventSafe(g));g=reboot(g);g.enemyHp=0;finish(g);g=reboot(g);auto gold=g.p.gold,xp=g.p.xp;assert(resolveCampaign(g)&&g.campaignFlags==7&&g.p.gold==gold+350&&g.p.xp==xp+3500);g=reboot(g);before=g;assert(!resolveCampaign(g)&&startCampaign(g,3)&&!campaignContact(g));assert(!memcmp(&g,&before,sizeof(g)));assert(story::knownChapter(g)==4&&strstr(story::objective(g).title,"VIGILIA"));}
 for(auto phase:{Phase::Lost,Phase::Fled}){auto g=prepared(0,0);assert(!startCampaign(g,3));g.phase=phase;if(phase==Phase::Lost)g.p.hp=0;g=reboot(g);auto gold=g.p.gold,xp=g.p.xp;assert(resolveCampaign(g)&&g.campaignFlags==3&&g.p.hp&&g.p.gold==gold&&g.p.xp==xp);g=reboot(g);assert(!startCampaign(g,3));}
 for(unsigned invalid=0;invalid<7;++invalid){auto g=prepared(2,0);if(invalid==0)g.city=2;if(invalid==1){g.p.level=17;g.p.mp=g.p.maxmp=totalMana(g);}if(invalid==2)g.campaignFlags=1;if(invalid==3)g.tutorial=false;if(invalid==4)g.dungeonClears=0;if(invalid==5)g.tripStage=1;if(invalid==6)g.phase=Phase::Hero;auto before=g;assert(startCampaign(g,3)&&!memcmp(&g,&before,sizeof(g)));}
 auto g=prepared(2,0);uint8_t b[SAVE_SIZE];encode(g,3,b);legacyFormat(b,20);put32(b,124,crc(b,124));Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&h.campaignFlags==3&&h.seenEnemies==g.seenEnemies&&!h.campaignStage);g.campaignFlags=7;encode(g,3,b);legacyFormat(b,20);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);g.campaignFlags=5;assert(!valid(g));
 Store store;Journal<Store> j(store);assert(j.load(h)==Load::Empty);g=prepared(2,0);assert(j.save(g));assert(!startCampaign(g,3));assert(j.save(g));g.enemyHp=0;finish(g);assert(j.save(g));auto won=g;auto gold=g.p.gold;assert(resolveCampaign(g));store.fail=true;assert(!j.save(g));Journal<Store> boot(store);assert(boot.load(h)==Load::Ok&&h.campaignStage==3&&h.phase==Phase::Won&&h.p.gold==won.p.gold);store.fail=false;assert(j.save(g)&&j.save(g));assert(boot.load(h)==Load::Ok&&h.campaignFlags==7&&h.p.gold==gold+350&&!resolveCampaign(h));
 // Ordinary Aurora encounters cannot manufacture archive progress.
 g=prepared(1,0);assert(begin(g,7));g.enemyHp=0;finish(g);assert(home(g)&&g.campaignFlags==3&&!g.campaignStage);
 assert(campaignScenes(3)==6);for(unsigned i=0;i<6;++i){auto scene=story::campaignScene(3,i);assert(story::speakerNpc(scene.speaker)!=story::Npc::None);for(auto line:scene.lines)assert(strlen(line)<=38);}
 puts("PASS: Aurora all classes/races, level18/prerequisites, six named portrait scenes, old20 preservation, defeat/flee, ordinary encounter isolation, reward once and failed-save retry/reboot");
}

