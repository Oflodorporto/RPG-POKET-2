#include "../firmware/RPG_POKET_2/DialogueStory.h"
#include "../firmware/RPG_POKET_2/Persistence.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <set>
#include <string>
#include <vector>
using namespace rpg;

struct DialogueState {std::string label;Game hero;};
static unsigned conversations=0,pages=0,maxRows=0;
static std::set<std::string> speeches;
static bool has(const char* text,const char* phrase){return strstr(text,phrase)!=nullptr;}
static Game reboot(const Game& g,unsigned format=PERSISTENT_SAVE_FORMAT){
 assert(persistentValid(g));
 uint8_t bytes[PERSISTENT_SAVE_SIZE];encodePersistentVersion(g,17,0,bytes,format);
 Game restored;uint32_t seq=0;
 assert(decodePersistent(bytes,0,restored,seq)==Decode::Ok&&seq==17);
 assert(persistentValid(restored));return restored;
}
static Game prepared(unsigned cls,unsigned race,bool origin){
 auto g=create(cls,77);g.race=race;g.originStory=origin;g.tutorial=true;
 g.p.xp=265000;levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;
 g.p.gold=50000;g.rations=9;g.dungeonClears=1;g.city=2;
 return reboot(g);
}
static void win(Game& g,unsigned mission){
 g.city=campaignCity(mission);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;
 assert(!startCampaign(g,mission));
 if(!campaignDonation(mission)){g.enemyHp=0;finish(g);assert(resolveCampaign(g));}
 g=reboot(g);
}
static void inspect(const Game& g,unsigned person,const char* label){
 assert(persistentValid(g));
 uint8_t before[PERSISTENT_SAVE_SIZE],after[PERSISTENT_SAVE_SIZE];
 encodePersistent(g,31,1,before);
 const char* text=story::conversation(g,person);
 assert(text&&*text&&strchr(text,' '));
 for(const unsigned char* p=(const unsigned char*)text;*p;++p)assert(*p>=32&&*p<127);
 unsigned rows=story::wrapStory(text,18,[](unsigned,const char* line){assert(strlen(line)<=18);});
 if(rows>42)fprintf(stderr,"Dialogue overflow: %s city=%u person=%u rows=%u\n%s\n",label,g.city,person,rows,text);
 assert(rows&&rows<=42&&story::conversationPages(text)==(rows+6)/7);
 assert(!strcmp(text,story::conversation(g,person+3)));
 assert(!has(text,"atualizacao")&&!has(text,"proxima etapa")&&!has(text,"sorte num saque"));
 assert(!has(text,"nao vou tomar sua arma")&&!has(text,"Borin mostrou o desenho"));
 if(!campaignMemory(g)){
  for(const char* spoiler:{"Vaelor","Anwen","Livro","Pacto","Vigilia","memorias roubadas"})assert(!has(text,spoiler));
 }
 if(!(g.campaignFlags&2))assert(!has(text,"Anwen"));
 if(g.campaignEnding!=2){
  if(g.campaignFlags&128){
   for(const char* stale:{"enfrente Odran","enfrente um rei","impediremos Odran","impedir a Vigilia","Odran quer"})assert(!has(text,stale));
   if((g.city==0&&person==0)||(g.city==3&&person<=1))assert(has(text,"custodia"));
  }
  if((g.campaignFlags&7)==7&&person==campaignPerson(4+g.city)){
   assert(bool(!strcmp(text,story::contributionSpeech(4+g.city)))==contribution(g,g.city));
   if(!contribution(g,g.city)){
    const char* request[]={"Traga tres racoes","mantenha-o longe","Escolte-nos","cento e cinquenta moedas"};
    assert(has(text,request[g.city]));
   }
  }
  if(g.city==3&&person==0&&g.campaignEnding==1){
   assert(has(text,"Coracao do Veu")&&(has(text,"nao recolheremos outras")||has(text,"Nao recolheremos outras")));
   if(anwenReady(g))assert(has(text,"quatro ajudas estao prontas")&&!has(text,"faltam"));
   else assert(has(text,"ajudas faltam")&&has(text,"lembrancas aprisionadas"));
  }
  if(g.city==0&&person==2){
   if(g.campaignFlags&1)assert(!has(text,"Leve as provas a Sabela"));
   if((g.campaignFlags&3)==3&&!(g.campaignFlags&4))assert(has(text,"Liora"));
   if(g.campaignFlags&4)assert(has(text,"estrutura")||has(text,"restauracao"));
  }
  if(g.city==1&&person==0&&!campaignMemory(g)){
   assert(has(text,"cristal"));
   if(g.crystals)assert(has(text,"ja tem um cristal")&&!has(text,"a venda")&&!has(text,"conseguir um cristal"));
   else assert(has(text,"300 moedas"));
   if(g.guardianDefeated)assert(has(text,"defesa externa")&&has(text,"nao o Arconte"));
  }
  if(g.city==1&&person==2&&!campaignMemory(g)){
   assert(has(text,"Uma voz chega de alem dos selos")&&has(text,"Iria talvez saiba"));
   if(g.crystals)assert(has(text,"ja tem um cristal")&&!has(text,"a venda")&&!has(text,"Guardiao"));
   else assert(has(text,"Guardiao carrega um")&&has(text,"a venda")&&has(text,"300 moedas"));
  }
 }
 encodePersistent(g,31,1,after);assert(!memcmp(before,after,sizeof(before)));
 speeches.insert(text);++conversations;pages+=story::conversationPages(text);maxRows=std::max(maxRows,rows);
}
static void allPeople(const Game& hero,const char* label){
 for(unsigned city=0;city<4;++city)for(unsigned person=0;person<3;++person){auto g=hero;g.city=city;inspect(g,person,label);}
}
static std::vector<DialogueState> auditMatrix(unsigned cls,unsigned race,bool origin){
 // These are the audit's 56 persistent-valid states, with campaign flags earned
 // through the real start/resolve functions, including every contribution mask.
 std::vector<DialogueState> states;
 auto g=create(cls,77);g.race=race;g.originStory=origin;g.tutorial=true;
 states.push_back({"begin",g});g.ruinsWins=1;states.push_back({"ruins_1",g});
 g.ruinsWins=3;states.push_back({"ruins_3",g});g.guardianDefeated=true;states.push_back({"guardian",g});
 auto base=prepared(cls,race,origin);states.push_back({"book",base});
 win(base,1);states.push_back({"cargo",base});win(base,2);states.push_back({"rescue",base});win(base,3);
 for(unsigned mask=0;mask<16;++mask){
  auto branch=base;for(unsigned city=0;city<4;++city)if(mask&(1u<<city))win(branch,4+city);
  states.push_back({"allies_"+std::to_string(mask),branch});
  win(branch,8);states.push_back({"odran_"+std::to_string(mask),branch});
  win(branch,9);states.push_back({"ending_"+std::to_string(mask),branch});
  if(!mask){
   for(unsigned city=0;city<4;++city)win(branch,4+city);
   assert(branch.campaignEnding==1&&anwenReady(branch));states.push_back({"temporary_ready",branch});
   win(branch,9);assert(branch.campaignEnding==2);allPeople(branch,"full_after_temporary");
  }
 }
 assert(states.size()==56);return states;
}
static void crystalAndFirstDiscovery(unsigned cls,unsigned race,bool origin){
 auto g=create(cls,77);g.race=race;g.originStory=origin;allPeople(g,"guide_pending");
 g.tutorial=true;g.city=1;g.p.gold=900;
 // Buying is a genuine entry route even with zero wins and an unbeaten Guardian.
 assert(!g.guardianDefeated&&!g.ruinsWins&&!buyCrystal(g)&&g.p.gold==600&&g.crystals==1);
 allPeople(g,"bought_crystal");
 for(unsigned wins:{1u,3u}){auto withCrystal=g;withCrystal.ruinsWins=wins;allPeople(withCrystal,"bought_with_wins");}
 auto guardian=g;guardian.ruinsWins=3;guardian.guardianDefeated=true;allPeople(guardian,"guardian_with_crystal");
 assert(!enterDungeon(g,0)&&!g.crystals&&!g.guardianDefeated);
 assert(persistentValid(g));for(unsigned p=0;p<3;++p)inspect(g,p,"entered_without_guardian");
 // A valid fixed-layout checkpoint just before the boss. Discovery is visible
 // on the current Won screen before dungeonResolve increments dungeonClears.
 dungeonSetFloor(g,1);g.dungeonLoot=8;g.dungeonXY=0x27;dungeonFace(g,0);
 assert(dungeonEnemyAhead(g)==6&&begin(g,8));assert(!campaignMemory(g)&&persistentValid(g));
 for(unsigned p=0;p<3;++p)inspect(g,p,"before_first_boss_win");
 g.enemyHp=0;finish(g);assert(campaignMemory(g)&&!g.dungeonClears);g=reboot(g);
 for(unsigned p=0;p<3;++p)inspect(g,p,"first_boss_won_unresolved");
 assert(has(story::conversation(g,2),"recusasse o sacrificio"));
 assert(!has(story::conversation(g,2),"Anwen"));
 assert(dungeonResolve(g)&&g.dungeonClears==1);g=reboot(g);
 for(unsigned p=0;p<3;++p)inspect(g,p,"first_boss_recorded");
 assert(leaveDungeon(g));allPeople(reboot(g),"book_bought_without_guardian");
 // Save29 remains a supported migration, with no origin inferred for a legacy hero.
 g.originStory=false;allPeople(reboot(g,29),"legacy29_without_origin");
}
int main(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race)for(bool origin:{false,true}){
  for(const auto& state:auditMatrix(cls,race,origin))allPeople(state.hero,state.label.c_str());
  crystalAndFirstDiscovery(cls,race,origin);
 }
 auto g=prepared(0,0,true);for(unsigned mission:{1u,2u,3u,4u,5u,6u,7u,8u,9u})win(g,mission);
 const char* signatures[4][3]={{"fui eu que toquei","As raizes","Dargan fez"},{"Os nomes chegaram","A expedicao","O selo permanece"},{"Os voluntarios","As rotas","Minha equipe"},{"Meu pai","Os juramentos","Pode conferir"}};
 std::set<std::string> full;
 for(unsigned city=0;city<4;++city)for(unsigned person=0;person<3;++person){
  g.city=city;const char* text=story::conversation(g,person);
  assert(has(text,signatures[city][person]));assert(strcmp(text,story::contributionSpeech(4+city)));
  full.insert(text);assert(!has(text,"Traga")&&!has(text,"Escolte-nos")&&!has(text,"faltam"));
 }
 assert(full.size()==12);
 printf("PASS: %u conversations/%u pages/%zu distinct speeches; 56-state audit matrix x 12 NPCs x 4 classes x 4 races x both origins; 12 final voices, all masks/deposition/endings, bought crystal without Guardian, current first discovery, Save29/30 purity; max %u rows at 18 columns/7 rows per page\n",conversations,pages,speeches.size(),maxRows);
}
