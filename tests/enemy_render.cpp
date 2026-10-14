#define main originalEnemyRenderRegression
#include "render.cpp"
#undef main
#include <set>
#include <filesystem>
#include <new>
#include <memory>
static bool drawing=false;
extern "C" void* __real_malloc(size_t);
extern "C" void* __real_calloc(size_t,size_t);
extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){assert(!drawing);return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t s){assert(!drawing);return __real_calloc(n,s);}
extern "C" void* __wrap_realloc(void* p,size_t n){assert(!drawing);return __real_realloc(p,n);}
void* operator new(size_t n){assert(!drawing);if(auto p=malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{free(p);}void operator delete(void* p,size_t)noexcept{free(p);}
static void frozen(Canvas& c,const rpg::Game& g,const ViewState& v){
 auto copy=g;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,7,before);auto memory=artMemory;auto workspace=enemyArt::workspace;
 drawing=true;render(c,g,v,0);drawing=false;
 rpg::encode(g,7,after);assert(!memcmp(before,after,sizeof(before))&&!memcmp(&g,&copy,sizeof(g))&&memory==artMemory&&workspace==enemyArt::workspace);
}
static uint32_t hashArea(const Canvas& c,int x,int y,int w,int h){uint32_t hash=2166136261u;for(int j=y;j<y+h;++j)for(int i=x;i<x+w;++i)hash=(hash^c.pixels[j*240+i])*16777619;return hash;}
struct ContactProbe:Canvas {
 struct Point{int x,y,w,h;};Point sparks[256]{};unsigned count=0;
 void fillRect(int x,int y,int w,int h,uint16_t color){
  if(color==UI_GOLD&&w==2&&h==1){assert(count<256);sparks[count++]={x,y,w,h};}
  Canvas::fillRect(x,y,w,h,color);
 }
 bool sparkAt(int x,int y)const{for(unsigned i=0;i<count;++i)if(sparks[i].x==x&&sparks[i].y==y)return true;return false;}
};
static unsigned verifyProjectedHeroContact(){
 unsigned checks=0;bool shortBody=false,tallBody=false,floating=false;
 for(unsigned mode=0;mode<6;++mode)for(bool lethal:{false,true})for(unsigned progress:{600u,650u,700u}){
  auto g=rpg::create(0,42);g.dungeonFlags=1;g.phase=lethal?rpg::Phase::Won:rpg::Phase::Hero;g.enemyHp=lethal?0:100;g.dungeonLoot=255;
  const unsigned spawnIndex=mode>=3?2:0;
  if(mode==0){g.city=1;g.dungeonXY=0x72;rpg::dungeonFace(g,1);}
  else if(mode==3){g.city=1;g.dungeonXY=0x47;rpg::dungeonFace(g,0);}
  else {g.eventStage=2;g.eventKind=3;g.eventTier=mode==1?0:mode==2?2:mode==4?1:3;g.city=g.eventTier;g.eventLevel=mode==1?2:10;g.dungeonXY=(mode>=4?3:1)|(2<<4);rpg::dungeonFace(g,0);if(mode>=4)rpg::dungeonSetFloor(g,1);}
  const auto spawn=rpg::dungeonSpawn(g,spawnIndex);g.enemyId=spawn.id;g.dungeonEnemies=((1u<<rpg::dungeonSpawnCount(g))-1)&~(1u<<spawnIndex);assert(rpg::dungeonEnemyAhead(g)==int(spawnIndex));
  ViewState v;v.page=Page::Dungeon;v.effect=Effect::MagicDarts;v.effectProgress=progress;v.effectElapsedMs=progress*1100/1000;v.effectFrame=progress*8/1000;
  v.combatResult={CombatFeedback::Damage,4,false,false};v.combatResult.heroAction=rpg::Action::MagicMissile;v.combatResult.hasHeroAction=true;v.combatResult.enemyDefeated=lethal;
  const auto frame=enemyArt::body(spawn.id,enemyPose(true,v.combatResult,progress,false));const auto image=enemyArt::decode(frame);
  shortBody|=spawn.id==25;tallBody|=spawn.id==2;floating|=frame.anchor==enemyArt::Anchor::Virtual;
  const auto fit=enemyScale::world(spawn.id,frame,120+combatRecoil(v.combatResult,progress),172,1.f);
  const int rootX=fit.anchorX,rootY=enemyArt::actionY(frame,fit);int targetX=120,targetY=82,best=INT_MAX;
  for(int x=std::max(0,fit.x);x<std::min(240,fit.x+fit.w);x+=2)for(int y=std::max(34,fit.y);y<std::min(172,fit.y+fit.h);y+=2){
   const unsigned ix=(x-fit.x)*80/fit.w,iy=(y-fit.y)*86/fit.h;if(image.pixels[iy*80+ix]==0xf81f)continue;
   int dx=x-rootX,dy=y-rootY,d=dx*dx+dy*dy;if(d<best){best=d;targetX=x;targetY=y;}
  }
  assert(best<INT_MAX);ContactProbe probe;auto before=g;drawing=true;drawDungeon(probe,g,v);drawing=false;assert(!memcmp(&before,&g,sizeof(g)));
  const int distance=5+(int(v.effectElapsedMs)-540)/13;assert(probe.sparkAt(targetX+distance,targetY));++checks;
  if(lethal&&progress==650)probe.save("build/screens/enemy77/contact-foe"+std::to_string(spawn.id)+"-fade.ppm");
 }
 assert(shortBody&&tallBody&&floating);return checks;
}
int main(){
 const std::string output="build/screens/enemy77";std::filesystem::create_directories(output);
 std::vector<uint8_t> memory(ART_BYTES+64,0xa5);artMemory=memory.data()+32;std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES);
 Canvas c,baseline;ViewState v;auto g=rpg::create(0,42);unsigned frames=0;std::set<uint32_t> bodies;
 for(unsigned id=0;id<29;++id){
  for(unsigned pose=0;pose<4;++pose){c.fillScreen(0);drawing=true;enemyArt::drawBody(c,id,enemyArt::Pose(pose),80,40,80,86);drawing=false;bodies.insert(hashArea(c,80,40,80,86));}
  g=rpg::create(0,42);g.enemyId=id;g.enemyHp=rpg::encounterHp(g,id);g.phase=rpg::Phase::Hero;g.seenEnemies=1u<<id;
  for(auto page:{Page::Battle,Page::EnemyInfo,Page::Bestiary}){v=ViewState{};v.page=page;v.bestiaryIndex=id;frozen(c,g,v);++frames;if(page==Page::Battle)c.save(output+"/arena-"+std::to_string(id)+".ppm");if(id==25&&page!=Page::Battle)c.save(output+"/crab-"+std::to_string(unsigned(page))+".ppm");}
  for(unsigned beat=0;beat<3;++beat)for(auto outcome:{CombatFeedback::Damage,CombatFeedback::Miss,CombatFeedback::NoDamage,CombatFeedback::Heal,CombatFeedback::Buff,CombatFeedback::Skip}){
   g.enemyBeat=beat;v=ViewState{};v.page=Page::Battle;v.effect=Effect::Slash;v.combatResult={outcome,uint16_t(outcome==CombatFeedback::Damage||outcome==CombatFeedback::Heal?4:0),false,true};
   v.combatResult.resolvedEnemyIntent=outcome==CombatFeedback::Heal?rpg::Intent::Mend:outcome==CombatFeedback::Buff?rpg::Intent::Prepare:rpg::enemyIntent(g);v.effectOnHero=v.combatResult.attack();
   bool first=true;
   for(unsigned p:{0u,199u,200u,490u,491u,650u,818u,999u}){v.effectProgress=p;v.effectFrame=p*8/1000;frozen(c,g,v);++frames;
    if(first){baseline=c;first=false;}else for(unsigned y=0;y<320;++y)if(y<86||y>=192)for(unsigned x=0;x<240;++x)assert(c.pixels[y*240+x]==baseline.pixels[y*240+x]);
    if(id==19&&outcome==CombatFeedback::Damage&&beat==2)c.save(output+"/core-drain-"+std::to_string(p)+".ppm");
   }
  }
 }
 assert(bodies.size()==116);
 // Every authored effect frame is independently drawable; misses/skips draw no contact pixels.
 for(unsigned f=0;f<7;++f)for(unsigned p=0;p<8;++p){c.fillScreen(0);drawing=true;enemyArt::drawEffect(c,enemyArt::Family(f),p,120,100,32);drawing=false;assert(hashArea(c,104,84,32,32)!=hashArea(baseline,104,84,32,32));}
 for(auto outcome:{CombatFeedback::Miss,CombatFeedback::NoDamage,CombatFeedback::Skip})for(unsigned p=0;p<=1000;p+=10){c.fillScreen(0);CombatResult r{outcome,0,false,true};drawing=true;drawEnemyAction(c,0,r,p,180,130,50,130);drawing=false;for(auto pixel:c.pixels)assert(!pixel);}
 // All real dungeon rosters, floors, directions and target outcomes use logical80x86 safely.
 unsigned dungeonFrames=0;
 for(unsigned mode=0;mode<6;++mode){
  g=rpg::create(3,42);g.dungeonFlags=1;g.city=1;g.dungeonLoot=192;
  if(mode==1){g.dungeonFlags|=16;g.campaignFlags=255;g.campaignEnding=2;}
  if(mode>=2){g.eventStage=2;g.eventKind=3;g.eventTier=mode-2;g.city=g.eventTier;g.eventLevel=mode==2?2:mode==3?5:mode==4?10:20;}
  for(unsigned i=0;i<rpg::dungeonSpawnCount(g);++i){auto spawn=rpg::dungeonSpawn(g,i);rpg::dungeonSetFloor(g,spawn.floor);
   for(unsigned heading=0;heading<4;++heading){int x=spawn.x-rpg::dungeonDx[heading],y=spawn.y-rpg::dungeonDy[heading];if(rpg::dungeonWall(g,x,y))continue;
    g.dungeonXY=x|(y<<4);rpg::dungeonFace(g,heading);g.enemyId=spawn.id;g.enemyHp=rpg::encounterHp(g,spawn.id);g.phase=rpg::Phase::Hero;
    v=ViewState{};v.page=Page::Dungeon;v.effect=Effect::Slash;v.combatResult={CombatFeedback::Damage,4,false,true};v.effectOnHero=true;
    for(unsigned p:{0u,200u,491u,650u,999u}){v.effectProgress=p;v.effectFrame=p*8/1000;frozen(c,g,v);++dungeonFrames;}
    if(heading==1)c.save(output+"/dungeon-mode"+std::to_string(mode)+"-foe"+std::to_string(spawn.id)+".ppm");
   }
  }
 }
 // Lethal dungeon effect retains only the existing spawn during its fade. No resurrected mask.
 g=rpg::create(0,31);g.city=1;g.crystals=1;assert(!rpg::enterDungeon(g,0));g.dungeonXY=0x72;rpg::dungeonFace(g,1);assert(rpg::dungeonEnemyAhead(g)==0);assert(rpg::begin(g,2));g.enemyHp=0;rpg::finish(g);assert(g.phase==rpg::Phase::Won&&!g.dungeonEnemies);
 v=ViewState{};v.page=Page::Dungeon;v.effect=Effect::MagicDarts;v.combatResult={CombatFeedback::Damage,1,false,false};v.combatResult.enemyDefeated=true;
 unsigned fullDefeatPixels=0,lateDefeatPixels=0;Canvas hiddenCanvas;
 for(unsigned p:{491u,650u,800u,950u,1000u}){v.effectProgress=p;v.effectFrame=p*8/1000;frozen(c,g,v);c.save(output+"/dungeon-defeat-"+std::to_string(p)+".ppm");assert(!g.dungeonEnemies);
  auto missing=g;missing.dungeonEnemies|=1;frozen(hiddenCanvas,missing,v);unsigned difference=0;for(unsigned y=34;y<172;++y)for(unsigned x=0;x<240;++x)difference+=c.pixels[y*240+x]!=hiddenCanvas.pixels[y*240+x];
  if(p==650)fullDefeatPixels=difference;if(p==950)lateDefeatPixels=difference;if(p==1000)assert(!difference);
 }
 assert(fullDefeatPixels>lateDefeatPixels&&lateDefeatPixels>0);
 v.effect=Effect::None;frozen(baseline,g,v);auto hidden=g;hidden.dungeonEnemies|=1;frozen(c,hidden,v);for(unsigned y=34;y<172;++y)for(unsigned x=0;x<240;++x)assert(c.pixels[y*240+x]==baseline.pixels[y*240+x]);
 // Search existing original-dungeon geometry for two separately visible actors and verify
 // each survives the following decode. This catches caching two workspace pointers.
 bool multiple=false;Canvas only0,only1,none;
 g=rpg::create(0,42);g.dungeonFlags=1;g.city=1;g.dungeonLoot=8;rpg::dungeonSetFloor(g,1);g.phase=rpg::Phase::Home;v=ViewState{};v.page=Page::Dungeon;
 for(unsigned y=1;y<8&&!multiple;++y)for(unsigned x=1;x<8&&!multiple;++x)for(unsigned d=0;d<4&&!multiple;++d){if(rpg::dungeonWall(g,x,y))continue;g.dungeonXY=x|(y<<4);rpg::dungeonFace(g,d);g.dungeonEnemies=31;frozen(c,g,v);g.dungeonEnemies=95;frozen(only0,g,v);g.dungeonEnemies=63;frozen(only1,g,v);g.dungeonEnemies=127;frozen(none,g,v);
  unsigned visible0=0,visible1=0;
  for(unsigned py=34;py<120;++py)for(unsigned px=0;px<240;++px){auto i=py*240+px;if(only0.pixels[i]!=none.pixels[i]&&only1.pixels[i]==none.pixels[i]){assert(c.pixels[i]==only0.pixels[i]);++visible0;}if(only1.pixels[i]!=none.pixels[i]&&only0.pixels[i]==none.pixels[i]){assert(c.pixels[i]==only1.pixels[i]);++visible1;}}
  if(visible0>10&&visible1>10){multiple=true;c.save(output+"/two-billboards.ppm");}
 }
 assert(multiple);
 const unsigned projectedContacts=verifyProjectedHeroContact();assert(projectedContacts==36);
 // Missing/wrong/corrupt packages are tested by actual loaders separately. Their zero
 // compact fallback remains transparent; absent workspace must not block repair UI.
 // The optional workspace can fail even when the full original art package loaded.
 auto savedWorkspace=enemyArt::workspace;enemyArt::workspace=nullptr;g=rpg::create(0,42);g.phase=rpg::Phase::Hero;g.enemyHp=15;v=ViewState{};v.page=Page::Battle;frozen(c,g,v);v.page=Page::Updates;frozen(c,g,v);enemyArt::workspace=savedWorkspace;
 makeFallback();g=rpg::create(0,42);g.enemyHp=15;g.phase=rpg::Phase::Hero;g.seenEnemies=1;
 for(bool workspace:{true,false}){enemyArt::workspace=workspace?enemyTestWorkspace:nullptr;
  for(auto page:{Page::Title,Page::Menu,Page::Wifi,Page::Updates,Page::Battle,Page::EnemyInfo,Page::Bestiary}){v=ViewState{};v.page=page;frozen(c,g,v);}
  g.dungeonFlags=1;g.dungeonXY=0x71;v.page=Page::Dungeon;frozen(c,g,v);g.dungeonFlags=0;
 }
 enemyArt::workspace=nullptr;artMemory=nullptr;v=ViewState{};v.page=Page::Updates;frozen(c,g,v);
 for(unsigned i=0;i<32;++i)assert(memory[i]==0xa5&&memory[memory.size()-1-i]==0xa5);
 printf("PASS: enemy77 %u arena/info/bestiary semantic frames, %u dungeon frames; 116 distinct poses, effect families, fixed HUD, sequential billboards, defeat fade, 36 projected hero contacts (short/tall/floating/fading), fallback/update and no render allocations/state changes.\n",frames,dungeonFrames);
}
