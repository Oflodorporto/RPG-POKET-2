#include "LegacySave.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/Narrative.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
using namespace rpg;
Game prepared(unsigned cls,unsigned race){auto g=create(cls,77);g.race=race;g.p.xp=64000;levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=true;g.city=2;g.dungeonClears=1;assert(valid(g));return g;}
Game reboot(const Game& g){assert(valid(g));uint8_t b[SAVE_SIZE];encode(g,41,b);Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&seq==41);return h;}
std::string sceneText(unsigned mission,unsigned index){const auto& scene=story::campaignScene(mission,index);assert(story::speakerNpc(scene.speaker)!=story::Npc::None);std::string text;for(auto line:scene.lines){assert(*line&&strlen(line)<=34);for(const char* p=line;*p;++p)assert((unsigned char)*p<128);text+=line;text+=' ';}return text;}
int main(){
 // Four fixed lines per scene: the name is recovered only after the rescue.
 for(unsigned mission=1;mission<=2;++mission){assert(campaignScenes(mission)==4);for(unsigned index=0;index<4;++index){auto text=sceneText(mission,index);if(mission==1||(mission==2&&index<2))assert(text.find("Anwen")==std::string::npos);assert(text.find("fragmentos")==std::string::npos&&text.find("sorte")==std::string::npos);}}
 const auto confession=sceneText(1,2),tomas=sceneText(1,1),name=sceneText(2,2),response=sceneText(2,3);
 assert(tomas.find("Aceitei ouro")!=std::string::npos&&tomas.find("registros a Sabela")!=std::string::npos);
 assert(confession.find("Autorizei as cargas da Guilda")!=std::string::npos&&confession.find("Vou responder")!=std::string::npos);
 assert(name.find("nome apagado: Anwen")!=std::string::npos&&name.find("recusou sacrificar memorias")!=std::string::npos&&name.find("registros seguem para Aurora")!=std::string::npos);
 assert(response.find("contas da Guilda")!=std::string::npos&&response.find("Minha assinatura")!=std::string::npos);
 auto start=create(0,77);start.tutorial=true;auto goal=story::objective(start);assert(!strcmp(goal.place,"Vespera / Iria Sorel"));assert(strstr(goal.lines[0],"mesma cinza")&&strstr(goal.lines[2],"cuidam de Carvalho"));
 for(unsigned wins:{0u,1u,3u}){start.ruinsWins=wins;for(unsigned crystals:{0u,1u}){start.crystals=crystals;uint8_t before[SAVE_SIZE],after[SAVE_SIZE];encode(start,41,before);goal=story::objective(start);for(auto line:goal.lines)assert(!strstr(line,"Anwen")&&!strstr(line,"Vaelor"));if(crystals)assert(strstr(goal.lines[0],"Seu cristal"));encode(start,41,after);assert(!memcmp(before,after,SAVE_SIZE));}}

 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race){auto g=prepared(cls,race);
  assert(campaignMission(g)==1&&story::knownChapter(g)==3);
  for(unsigned mission=1;mission<=2;++mission){auto before=g;assert(campaignError(g,mission)==nullptr);assert(!memcmp(&before,&g,sizeof(g)));
   assert(!startCampaign(g,mission));g=reboot(g);assert(g.campaignStage==mission&&!eventSafe(g));
   g.enemyHp=0;finish(g);assert(g.phase==Phase::Won);g=reboot(g);uint8_t beforeScenes[SAVE_SIZE],afterScenes[SAVE_SIZE];encode(g,41,beforeScenes);for(unsigned index=0;index<4;++index)sceneText(mission,index);encode(g,41,afterScenes);assert(!memcmp(beforeScenes,afterScenes,SAVE_SIZE));auto xp=g.p.xp,gold=g.p.gold;unsigned bonus=campaignXp(g,mission);
   assert(resolveCampaign(g));assert(g.p.xp==xp+bonus&&g.p.gold==gold+campaignGold(mission)&&g.campaignFlags==((1u<<mission)-1));g=reboot(g);before=g;assert(!resolveCampaign(g)&&startCampaign(g,mission));assert(!memcmp(&before,&g,sizeof(g)));
  }
  assert(campaignMission(g)==3&&story::knownChapter(g)==4);assert(!strcmp(story::objective(g).place,"Aurora / Liora Valcer"));
 }
 auto g=prepared(0,0);for(auto phase:{Phase::Lost,Phase::Fled}){assert(!startCampaign(g,1));g.phase=phase;if(phase==Phase::Lost)g.p.hp=0;g=reboot(g);auto gold=g.p.gold,xp=g.p.xp;assert(resolveCampaign(g)&&!g.campaignFlags&&g.p.hp&&g.p.gold==gold&&g.p.xp==xp);g=reboot(g);}
 g=prepared(2,0);g.dungeonClears=0;auto before=g;assert(startCampaign(g,1)&&!memcmp(&before,&g,sizeof(g)));g=prepared(2,0);g.city=0;before=g;assert(startCampaign(g,1)&&!memcmp(&before,&g,sizeof(g)));g=prepared(2,0);g.p.level=9;g.p.mp=g.p.maxmp=totalMana(g);assert(startCampaign(g,1));g=prepared(2,0);assert(startCampaign(g,2));
 // Legacy clear is recognized, but no story flags or rewards are manufactured on load.
 uint8_t b[SAVE_SIZE];g=prepared(2,0);encode(g,42,b);legacyFormat(b,15);put32(b,124,crc(b,124));Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&!h.campaignFlags&&!h.campaignStage&&campaignMemory(h)&&h.p.gold==g.p.gold);
 for(unsigned byte=120;byte<124;++byte){encode(g,42,b);b[byte]=byte==120?2:byte==121?3:byte==122?2:9;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);}
 encode(g,42,b);legacyFormat(b,15);b[120]=1;put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);
 // A normal victory at the port never advances the campaign.
 g=prepared(2,0);assert(begin(g,6));g.enemyHp=0;finish(g);assert(home(g)&&!g.campaignFlags&&!g.campaignStage);
 puts("PASS: Mares four-line scenes/knowledge order/responsibility, bought-crystal objective; all classes/races; gated pure preview; restart/completion/reward once; loss/flee retry; legacy15 migration/reserved bytes; ordinary fights isolated");
}
