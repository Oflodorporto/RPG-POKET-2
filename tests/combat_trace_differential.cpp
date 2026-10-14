// Independent combat ABI/save differential probe. The same source is compiled
// against a frozen baseline and both candidate APIs by test_combat_trace.py.
#include "Persistence.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <vector>
using namespace rpg;

static_assert(std::is_trivially_copyable<Game>::value, "Game must remain a value type");
#ifndef COMBAT_TRACE_SAVE_FORMAT
#define COMBAT_TRACE_SAVE_FORMAT 29
#endif
static_assert(PERSISTENT_SAVE_FORMAT == COMBAT_TRACE_SAVE_FORMAT && PERSISTENT_SAVE_SIZE == 160,
              "The baseline and candidate must use the explicitly selected save ABI");
namespace {
uint64_t records=0, accepted=0, rejected=0, wins=0, losses=0, flees=0;
unsigned seedCount=64;
const Action actions[]={Action::Attack,Action::Offensive,Action::Defensive,
 Action::Life,Action::Mana,Action::Flee,Action::MagicMissile,Action::BurningHands,
 Action::ShieldSpell,Action::ScorchingRay,Action::Fireball,Action::LayHands,
 Action::SacredWeapon,Action::TurnUndead,Action::SecondWind,Action::ActionSurge,
 Action::RagePower,Action::DivineSmite,Action(18),Action(255)};
const unsigned phaseCount=6;
uint32_t seedAt(unsigned i){
 const uint32_t edge[]={0,1,2,3,47,0xffffffffu,0x80000000u,0x7fffffffu,
                        0xdeadbeefu,0x55555555u,0xaaaaaaaau,0x01010101u};
 if(i<sizeof(edge)/sizeof(edge[0]))return edge[i];
 uint32_t x=0x9e3779b9u*(i+1);x^=x>>16;x*=0x85ebca6bu;x^=x>>13;return x;
}
void normalize(Game& g){
 // Padding is not game state. Clear only padding so every real member, including
 // fields omitted by Save29, participates in exact byte comparison.
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 12
 __builtin_clear_padding(&g);
#elif defined(__has_builtin)
 #if __has_builtin(__builtin_clear_padding)
 __builtin_clear_padding(&g);
 #else
 #error "The differential test requires __builtin_clear_padding"
 #endif
#else
 #error "The differential test requires GCC 12+ or equivalent"
#endif
}
void bytes(const void* p,size_t n){if(std::fwrite(p,1,n,stdout)!=n)std::abort();}
void snapshot(const Game& source,const uint32_t (&key)[10],const char* result){
 Game g=source;normalize(g);uint8_t save[PERSISTENT_SAVE_SIZE];
 encodePersistent(g,0x12345678u,2,save);
 char status[64]={};assert(!result||std::strlen(result)<sizeof status);if(result)std::snprintf(status,sizeof status,"%s",result);
 bytes(key,sizeof key);bytes(&g,sizeof g);bytes(save,sizeof save);bytes(status,sizeof status);
 ++records;wins+=g.phase==Phase::Won;losses+=g.phase==Phase::Lost;flees+=g.phase==Phase::Fled;
}
Game hero(bool modern,unsigned cls,unsigned level,uint32_t seed){
 Game g=create(cls,seed?seed:1);g.dndProgression=modern;
 if(modern){g.p.xp=dndXp[level-1];levelUp(g);}else{
  g.p.level=level;g.p.atk=6+cls+level-1;g.p.maxhp=18+2*cls+2*(level-1);
  g.p.maxmp=totalMana(g);
 }
 g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.p.life=3;g.p.mana=3;
 g.p.gold=257;g.p.xp=0;g.city=1;g.ruinsWins=3;g.randomState=seed;
 g.race=cls;g.shirt=(cls+2)%8;g.trousers=(cls+5)%8;g.fatigue=37;
 g.questId=2;g.questLevel=level;g.oath=modern&&cls==1&&level>=3;
#if COMBAT_TRACE_SAVE_FORMAT == 30
 // Retain a previous Ruins identity outside a run as well as active runs below.
 g.dungeonSeed=seed?seed:1;g.dungeonRun=1+(seed%0xffffffu);
#endif
 normalize(g);return g;
}
void battle(Game& g,unsigned id){
 // Direct setup deliberately exercises every combat branch independently from
 // the campaign/travel gates on begin(). It is not a persistence-validity test.
 g.phase=Phase::Hero;g.enemyId=id;g.enemyHp=encounterHp(g,id);
 g.enemyBeat=0;g.guard=0;g.seenEnemies|=1u<<id;
 g.damage=321;g.gainXp=123;g.gainGold=17;
 g.crit=g.dodge=g.dropLife=g.dropMana=true;
}
const char* runAction(Game& g,Action a,bool direct){
#if defined(COMBAT_TRACE_CAPTURE)
 struct Envelope{uint64_t prefix=0x0123456789abcdefULL;CombatTrace trace{};
                 uint64_t suffix=0xfedcba9876543210ULL;} box;
 const char* result=direct?usePower(g,a,&box.trace):act(g,a,&box.trace);
 assert(box.prefix==0x0123456789abcdefULL&&box.suffix==0xfedcba9876543210ULL);
#elif defined(COMBAT_TRACE_NULL)
 const char* result=direct?usePower(g,a,nullptr):act(g,a,nullptr);
#else
 const char* result=direct?usePower(g,a):act(g,a);
#endif
 accepted+=result==nullptr;rejected+=result!=nullptr;return result;
}
bool runEnemy(Game& g){
#if defined(COMBAT_TRACE_CAPTURE)
 struct Envelope{uint64_t prefix=0x0123456789abcdefULL;CombatTrace trace{};
                 uint64_t suffix=0xfedcba9876543210ULL;} box;
 bool result=enemy(g,&box.trace);
 assert(box.prefix==0x0123456789abcdefULL&&box.suffix==0xfedcba9876543210ULL);
#elif defined(COMBAT_TRACE_NULL)
 bool result=enemy(g,nullptr);
#else
 bool result=enemy(g);
#endif
 accepted+=result;rejected+=!result;return result;
}
void actionMatrix(){
 for(unsigned modern=0;modern<2;++modern)for(unsigned cls=0;cls<4;++cls)
 for(unsigned li=1;li<=(modern?20u:21u);++li){unsigned level=li==21?99:li;
  for(unsigned si=0;si<seedCount;++si)for(unsigned ai=0;ai<20;++ai)
  for(unsigned variant=0;variant<5;++variant){
   auto g=hero(modern,cls,level,seedAt(si));battle(g,(si+ai)%29);
   if(variant==0){g.p.hp=std::max(1u,unsigned(g.p.maxhp)/2);g.p.mp=g.p.maxmp/2;}
   if(variant==1){g.p.hp=1;g.p.mp=0;g.p.life=g.p.mana=0;}
   if(variant==2){g.enemyHp=1;g.p.xp=xpNeeded(g)-1;g.p.gold=999998;g.p.life=g.p.mana=98;}
   if(variant==3){
    g.enemyHp=60000;g.p.hp=std::max(1u,unsigned(g.p.maxhp)/3);
    g.surgePending=g.surgeTurnUsed=modern&&cls==2;g.rageTurns=modern&&cls==3?3:0;
    g.sacredTurns=modern&&cls==1?3:0;g.forge[0]=g.forge[1]=g.forge[2]=3;
   }
   if(variant==4){g.phase=Phase((si+ai)%phaseCount);g.windSpent=1;
    g.surgeSpent=surgeUses(g);g.rageSpent=uint8_t(rageUses(g));
    g.laySpent=uint8_t(5*std::min(20u,level));g.channelSpent=true;
   }
   const char* result=runAction(g,actions[ai],false);
   const uint32_t key[]={1,modern,cls,level,seedAt(si),variant,uint8_t(actions[ai]),g.enemyId,0,si};
   snapshot(g,key,result);
  }
 }
}
void directPowerMatrix(){
 for(unsigned modern=0;modern<2;++modern)for(unsigned cls=0;cls<4;++cls)
 for(unsigned li=1;li<=(modern?20u:21u);++li){unsigned level=li==21?99:li;
  for(unsigned si=0;si<std::min(seedCount,16u);++si)for(unsigned ai=0;ai<20;++ai)
  for(unsigned variant=0;variant<phaseCount+2;++variant){
   auto g=hero(modern,cls,level,seedAt(si));battle(g,(si+ai)%29);
   g.phase=Phase(variant<phaseCount?variant:unsigned(Phase::Hero));
   g.p.hp=std::max(1u,unsigned(g.p.maxhp)/2);
   if(variant==6){g.p.mp=0;g.oath=0;}
   if(variant==7){g.windSpent=1;g.surgeTurnUsed=true;g.rageTurns=3;g.channelSpent=true;
    g.laySpent=uint8_t(5*std::min(20u,level));}
   const char* result=runAction(g,actions[ai],true);
   const uint32_t key[]={2,modern,cls,level,seedAt(si),variant,uint8_t(actions[ai]),g.enemyId,0,si};
   snapshot(g,key,result);
  }
 }
}
void enemyMatrix(){
 const unsigned levels[]={1,2,5,11,15,18,20,99};
 for(unsigned modern=0;modern<2;++modern)for(unsigned cls=0;cls<4;++cls)
 for(unsigned level:levels){if(modern&&level==99)continue;
  for(unsigned si=0;si<std::min(seedCount,32u);++si)for(unsigned id=0;id<29;++id)
  for(unsigned beat=0;beat<3;++beat)for(unsigned variant=0;variant<6;++variant){
   auto g=hero(modern,cls,level,seedAt(si));battle(g,id);g.phase=Phase::Enemy;
   g.enemyBeat=beat;g.enemyHp=std::max(1u,unsigned(g.enemyHp)-5);
   g.surgeTurnUsed=true;g.rageTurns=modern&&cls==3?3:0;
   if(variant==1){g.guard=50;g.p.hp=1;g.p.mp=1;}
   if(variant==2){g.guard=75;g.p.hp=std::max(1u,unsigned(g.p.maxhp)/2);}
   if(variant==3){g.turnedTurns=2;g.guard=75;}
   if(variant==4){g.eventStage=2;g.eventKind=7;g.guard=75;}
   if(variant==5){g.phase=Phase((id+si)%phaseCount);g.p.mp=0;}
   const bool result=runEnemy(g);
   const uint32_t key[]={3,modern,cls,level,seedAt(si),variant,255,id,beat,si};
   snapshot(g,key,result?"true":"false");
  }
 }
}
void sessions(){
 const unsigned levels[]={1,2,3,5,9,11,15,17,18,20};
 for(unsigned modern=0;modern<2;++modern)for(unsigned cls=0;cls<4;++cls)
 for(unsigned level:levels)for(unsigned si=0;si<std::min(seedCount,32u);++si){
  auto g=hero(modern,cls,level,seedAt(si));battle(g,(si+level+cls)%29);
  for(unsigned turn=0;turn<96&&(g.phase==Phase::Hero||g.phase==Phase::Enemy);++turn){
   const uint32_t key[]={4,modern,cls,level,seedAt(si),0,255,g.enemyId,turn,si};
   if(g.phase==Phase::Enemy){snapshot(g,key,runEnemy(g)?"true":"false");continue;}
   Action a=actions[(turn+si+cls)%18];
   if(turn%3==0)a=Action::Attack;
   const char* result=runAction(g,a,false);snapshot(g,key,result);
   if(result){const uint32_t fallback[]={5,modern,cls,level,seedAt(si),0,0,g.enemyId,turn,si};
    snapshot(g,fallback,runAction(g,Action::Attack,false));}
  }
 }
}
#if COMBAT_TRACE_SAVE_FORMAT == 30
void proceduralCombatMatrix(){
 const unsigned levels[]={1,3,5,11,20};
 for(unsigned cls=0;cls<4;++cls)for(unsigned level:levels)
 for(unsigned si=0;si<std::min(seedCount,16u);++si){
  auto entered=hero(true,cls,level,seedAt(si));entered.crystals=1;
  assert(!enterDungeon(entered));assert(entered.dungeonGenerator==1);
  for(unsigned ai=0;ai<20;++ai){
   auto g=entered;battle(g,2);
   const uint32_t key[]={6,1,cls,level,seedAt(si),g.dungeonGenerator,uint8_t(actions[ai]),2,0,si};
   snapshot(g,key,runAction(g,actions[ai],false));
   if(g.phase==Phase::Enemy){const uint32_t reply[]={7,1,cls,level,seedAt(si),g.dungeonGenerator,255,2,1,si};
    snapshot(g,reply,runEnemy(g)?"true":"false");}
  }
 }
}
#endif
}
int main(int argc,char** argv){
 if(argc>1)seedCount=std::max(1,std::atoi(argv[1]));
 static char outputBuffer[1<<20];std::setvbuf(stdout,outputBuffer,_IOFBF,sizeof outputBuffer);
 const char magic[8]={'C','T','D','I','F','F','0','1'};bytes(magic,sizeof magic);
 const uint32_t sizes[]={uint32_t(sizeof(Game)),PERSISTENT_SAVE_SIZE,
                         uint32_t(10*sizeof(uint32_t)+sizeof(Game)+PERSISTENT_SAVE_SIZE+64)};
 bytes(sizes,sizeof sizes);
 actionMatrix();directPowerMatrix();enemyMatrix();sessions();
#if COMBAT_TRACE_SAVE_FORMAT == 30
 proceduralCombatMatrix();
#endif
 if(std::fflush(stdout))return 2;
 std::fprintf(stderr,"records=%llu accepted=%llu rejected=%llu won=%llu lost=%llu fled=%llu seeds=%u\n",
  (unsigned long long)records,(unsigned long long)accepted,(unsigned long long)rejected,
  (unsigned long long)wins,(unsigned long long)losses,(unsigned long long)flees,seedCount);
 return 0;
}
