#include "LegacySave.h"
#include "../firmware/RPG_POKET_2/Narrative.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using namespace rpg;
Game ready(unsigned cls,unsigned flags){auto g=create(cls,77);g.p.xp=265000;levelUp(g);g.tutorial=true;g.city=3;g.dungeonClears=1;g.campaignFlags=flags;return g;}
Game reload(Game g){assert(valid(g));uint8_t b[SAVE_SIZE],c[SAVE_SIZE];encode(g,1,b);Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok);encode(h,1,c);assert(!memcmp(b,c,SAVE_SIZE));return h;}
void win(Game& g,unsigned mission){assert(!startCampaign(g,mission));g=reload(g);g.enemyHp=0;finish(g);g=reload(g);assert(resolveCampaign(g));g=reload(g);auto before=g;assert(!resolveCampaign(g)&&!memcmp(&g,&before,sizeof(g)));}
struct FinalStore{bool fail=false;std::map<std::string,std::vector<uint8_t>> data;Read read(const char* k,uint8_t* b){auto it=data.find(k);if(it==data.end())return Read::Missing;memcpy(b,it->second.data(),SAVE_SIZE);return Read::Ok;}bool write(const char* k,const uint8_t* b){if(fail)return false;data[k]=std::vector<uint8_t>(b,b+SAVE_SIZE);return true;}};
// Exercise the exact blocks/pages consumed by first presentation and re-reading.
std::vector<std::string> readEnding(const Game& g,unsigned mission){
 uint8_t before[SAVE_SIZE],after[SAVE_SIZE];encode(g,1,before);
 std::vector<std::string> pages;bool origin=false,letter=false;
 for(unsigned page=0;page<story::finalePages(g,mission);++page){const auto frame=story::finalePage(g,mission,page);std::string rendered;unsigned rows=0;
  assert(strlen(frame.title)<=26);story::wrapStory(frame.text,18,[&](unsigned row,const char* line){assert(strlen(line)<=18);for(const char* p=line;*p;++p)assert((unsigned char)*p<128);if(row/7==frame.page){++rows;rendered+=line;rendered+='\n';}});assert(rows>0&&rows<=7);pages.push_back(std::string(frame.title)+"\n"+rendered);
  if(!strcmp(frame.title,"SUA JORNADA")){assert(story::finaleFull(g,mission)&&g.originStory&&!letter&&frame.npc==story::Npc::None);origin=true;}
  else if(!strcmp(frame.title,"CARTA DE SABELA")){assert(story::finaleFull(g,mission)&&frame.npc==story::Npc::Sabela);assert(!g.originStory||origin);assert(!strcmp(frame.text,story::finaleLetter()));letter=true;}
  else {assert(!origin&&!letter&&frame.npc==story::Npc::None);assert(!strstr(frame.text,"Talassar")&&!strstr(frame.text,"Neris")&&!strstr(frame.text,"Seu personagem"));}
 }
 assert(letter==story::finaleFull(g,mission));assert(origin==(story::finaleFull(g,mission)&&g.originStory));
 auto last=story::finalePage(g,mission,story::finalePages(g,mission)-1),clamped=story::finalePage(g,mission,999);assert(last.text==clamped.text&&last.page==clamped.page&&last.npc==clamped.npc);
 encode(g,1,after);assert(!memcmp(before,after,SAVE_SIZE));return pages;
}
int main(){
 assert(strstr(story::finaleLetter(),"carta de Neris, de Talassar")&&strstr(story::finaleLetter(),"ilha onde os capitaes so encontraram mar")&&strstr(story::finaleLetter(),"marco parecido com os nossos")&&strstr(story::finaleLetter(),"Guardei os relatos no porto"));
 const char* originWords[]={"seu mestre","juramento digno","sua familia","sua comunidade"};
 for(unsigned cls=0;cls<4;++cls)for(bool recorded:{false,true}){
  auto hero=ready(cls,127);hero.originStory=recorded;hero.originPage=recorded?8:0;assert(valid(hero));
  assert((bool)*story::finaleOrigin(hero)==recorded);if(recorded)assert(strstr(story::finaleOrigin(hero),originWords[cls]));
  assert(!startCampaign(hero,8));hero.enemyHp=0;finish(hero);readEnding(hero,8);assert(resolveCampaign(hero));hero=reload(hero);
  assert(!startCampaign(hero,9));hero.enemyHp=0;finish(hero);const auto first=readEnding(hero,9);assert(resolveCampaign(hero));hero=reload(hero);
  assert(first==readEnding(hero,9));assert(!strcmp(story::objective(hero).title,"AURORA LIVRE"));
 }
 for(unsigned mask=0;mask<15;++mask){auto hero=ready(0,7|(mask<<3));hero.originStory=true;hero.originPage=8;win(hero,8);assert(!startCampaign(hero,9));hero.enemyHp=0;finish(hero);const auto first=readEnding(hero,9);assert(strstr(story::finaleSpeech(hero,9),"memorias aprisionadas")&&strstr(story::finaleSpeech(hero,9),"Ninguem recolhera novas lembrancas"));assert(resolveCampaign(hero));hero=reload(hero);assert(first==readEnding(hero,9));auto goal=story::objective(hero);assert(!strcmp(goal.title,"O SELO TEMPORARIO")&&strstr(goal.lines[3],"pendentes"));
  hero.campaignFlags=255;hero=reload(hero);readEnding(hero,9);assert(strstr(story::finaleSpeech(hero,9),"quatro ajudas estao prontas")&&!strstr(story::finaleSpeech(hero,9),"faltam"));assert(!strcmp(story::objective(hero).title,"RESTAURAR OS FAROIS"));assert(finaleMission(hero)==9&&!finaleError(hero,9));
 }
 for(unsigned cls=0;cls<4;++cls)for(unsigned mask=0;mask<16;++mask){auto g=ready(cls,7|(mask<<3));win(g,8);assert(g.campaignFlags&128);win(g,9);assert(g.campaignEnding==(mask==15?2:1));auto before=g;assert(startCampaign(g,9)&&!memcmp(&before,&g,sizeof(g)));if(mask<15){g.campaignFlags=255;assert(g.campaignEnding==1&&valid(g));win(g,9);assert(g.campaignEnding==2);}assert(finaleError(g,9));assert((g.seenEnemies&(3u<<18))==(3u<<18));}
 for(unsigned mission:{8u,9u})for(auto outcome:{Phase::Lost,Phase::Fled}){auto g=ready(2,mission==8?127:255);assert(!startCampaign(g,mission));g.phase=outcome;if(outcome==Phase::Lost)g.p.hp=0;g=reload(g);unsigned flags=g.campaignFlags,gold=g.p.gold;assert(resolveCampaign(g)&&g.campaignFlags==flags&&g.p.gold==gold&&!g.campaignEnding);g=reload(g);assert(!startCampaign(g,mission));}
 auto g=ready(0,127);uint8_t b[SAVE_SIZE];encode(g,1,b);legacyFormat(b,22);put32(b,124,crc(b,124));Game h;uint32_t seq;assert(decode(b,h,seq)==Decode::Ok&&!h.campaignEnding);g.campaignFlags=255;g.campaignEnding=2;g.seenEnemies=3u<<18;encode(g,1,b);legacyFormat(b,22);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Corrupt);g=ready(0,7);g.campaignEnding=2;assert(!valid(g));g=ready(0,127);auto before=g;assert(!begin(g,18)&&!memcmp(&before,&g,sizeof(g)));for(unsigned m:{8u,9u})assert(story::finalePages(g,m)>2);
 for(unsigned mission:{8u,9u}){auto trial=ready(2,mission==8?127:255);assert(!startCampaign(trial,mission));trial.enemyHp=0;finish(trial);FinalStore store;Journal<FinalStore> j(store);assert(j.load(h)==Load::Empty&&j.save(trial));auto gold=trial.p.gold;assert(resolveCampaign(trial));store.fail=true;assert(!j.save(trial));Journal<FinalStore> boot(store);assert(boot.load(h)==Load::Ok&&h.campaignStage==mission&&!h.campaignEnding&&h.p.gold==gold);store.fail=false;assert(j.save(trial)&&j.save(trial));assert(boot.load(h)==Load::Ok&&!h.campaignStage&&h.p.gold==gold+(mission==9?500:0)&&!resolveCampaign(h));}
 for(unsigned cls=0;cls<4;++cls)for(unsigned seed=1;seed<=100;++seed){auto trial=ready(cls,127);trial.randomState=seed;trial.p.gold=5000;for(unsigned id:{cls*3+3,15u,18u}){assert(!buyGear(trial,id));assert(!equipGear(trial,id));}for(unsigned mission:{8u,9u}){trial.p.hp=trial.p.maxhp;trial.p.mp=trial.p.maxmp;trial.p.life=12;trial.p.mana=8;assert(!startCampaign(trial,mission));unsigned turns=0;while(trial.phase==Phase::Hero||trial.phase==Phase::Enemy){assert(++turns<200);if(trial.phase==Phase::Enemy)enemy(trial);else {auto a=trial.p.hp*2<trial.p.maxhp&&trial.p.life?Action::Life:cls==0&&!powerError(trial,Action::ScorchingRay)?Action::ScorchingRay:Action::Attack;assert(!act(trial,a));}assert(valid(trial));}if(trial.phase!=Phase::Won)fprintf(stderr,"balance cls=%u seed=%u mission=%u lv=%u hp=%u/%u enemyhp=%u turns=%u potions=%u atk=%d def=%d\n",cls,seed,mission,trial.p.level,trial.p.hp,trial.p.maxhp,trial.enemyHp,turns,trial.p.life,effectiveAttack(trial),effectiveDefense(trial));assert(trial.phase==Phase::Won);assert(resolveCampaign(trial));}assert(trial.campaignEnding==2);}
 puts("PASS: origin/legacy first-read and replay, separate Sabela/Neris letter, 18x7 ASCII page contract and save purity; 64 finale paths/all classes; Odran/Heart resume, temporary->full restoration without lost progress, no replay payouts, loss/flee retries, historical Save28/22 codec protection and20 enemy discovery bits");}
