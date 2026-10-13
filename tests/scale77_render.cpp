#include "SceneCanvas.h"
#include <filesystem>
#include <new>
#include <chrono>
#include <set>
static bool drawing=false;
extern "C" void* __real_malloc(size_t);
extern "C" void* __real_calloc(size_t,size_t);
extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){assert(!drawing);return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t s){assert(!drawing);return __real_calloc(n,s);}
extern "C" void* __wrap_realloc(void* p,size_t n){assert(!drawing);return __real_realloc(p,n);}
void* operator new(size_t n){assert(!drawing);if(auto p=malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{free(p);}void operator delete(void* p,size_t)noexcept{free(p);}
constexpr uint16_t backdrop=0x1234;
template<class C>static void immutable(C& c,const rpg::Game& g,const ViewState& v){
 auto copy=g;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,7,before);auto memory=artMemory;auto workspace=enemyArt::workspace;
 drawing=true;render(c,g,v,0);drawing=false;rpg::encode(g,7,after);
 assert(!memcmp(before,after,sizeof(before))&&!memcmp(&g,&copy,sizeof(g))&&memory==artMemory&&workspace==enemyArt::workspace);
}
struct ContactProbe:Canvas {
 int x=-1,y=-1;bool found=false;
 void fillRect(int px,int py,int w,int h,uint16_t color){if(px==x&&py==y&&w==2&&h==1&&color==UI_GOLD)found=true;Canvas::fillRect(px,py,w,h,color);}
};
int main(){
 std::filesystem::create_directories("build/screens/scale77");
 std::vector<uint8_t> memory(ART_BYTES+64,0xa5);artMemory=memory.data()+32;
 std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES);
 unsigned frameChecks=0,contacts=0,edgePixels=0;std::ofstream report("build/scale77-profiles.csv");
 report<<"id,arena_width,arena_height,world_canonical_height,world_offset,old_idle_opaque,new_idle_opaque,edge_pixels\n";
 for(unsigned id=0;id<29;++id){
  const auto& profile=enemyScale::profile(id);int left=80,top=86,right=0,bottom=0;
  for(unsigned pose=0;pose<4;++pose){auto f=enemyArt::body(id,enemyArt::Pose(pose));auto image=enemyArt::decode(f);
   for(unsigned y=0;y<image.h;++y)for(unsigned x=0;x<image.w;++x)if(enemyScale::opaque(image,x,y)){left=std::min(left,int(x));top=std::min(top,int(y));right=std::max(right,int(x+1));bottom=std::max(bottom,int(y+1));}
  }
  assert(left==profile.left&&top==profile.top&&right==profile.right&&bottom==profile.bottom);
  unsigned oldIdle=0,newIdle=0,idEdges=0;
  for(unsigned pose=0;pose<4;++pose)for(int recoil:{0,6,7}){
   const auto f=enemyArt::body(id,enemyArt::Pose(pose));auto image=enemyArt::decode(f);auto p=enemyScale::arena(id,f,recoil);
   auto rest=enemyScale::arena(id,enemyArt::body(id,enemyArt::Pose::Idle),recoil);assert(!memcmp(&p,&rest,sizeof(p)));
   assert(abs(p.w*86-p.h*80)<=86);
   // The full fixed pose union, not just already-clipped output, fits all recoil.
   assert(p.x+profile.left*p.w/80>=128);
   assert(p.x+(profile.right*p.w+79)/80<=239);
   assert(p.y+profile.top*p.h/86>=86);
   assert(p.y+(profile.bottom*p.h+85)/86<=180);
   Canvas body,legacy;body.fillScreen(backdrop);legacy.fillScreen(backdrop);drawing=true;const auto target=enemyScale::drawArena(body,id,f,image,p,1000);drawing=false;
   enemyArt::draw(legacy,image,enemyArt::fit(f,145,99,75,80));
   unsigned pixels=0,edges=0,oldPixels=0;
   for(int y=0;y<320;++y)for(int x=0;x<240;++x){oldPixels+=legacy.pixels[y*240+x]!=backdrop;const auto color=body.pixels[y*240+x];if(color==backdrop)continue;
    assert(y>=86&&y<181&&x>=128);const int ix=(x-p.x)*80/p.w,iy=(y-p.y)*86/p.h;
    if(enemyScale::opaque(image,ix,iy)){assert(color==image.pixels[iy*80+ix]);++pixels;}else {assert(color==enemyScale::edgeColor(f));++edges;}
   }
   assert(pixels>oldPixels);assert(edges);edgePixels+=edges;
   assert(target.x>=0&&target.x<240&&target.y>=86&&target.y<181);
   assert(enemyScale::opaque(image,(target.x-p.x)*80/p.w,(target.y-p.y)*86/p.h));
   if(!pose&&!recoil){oldIdle=oldPixels;newIdle=pixels;idEdges=edges;}
   for(unsigned opacity:{0u,100u,500u,1000u}){Canvas faded;faded.fillScreen(backdrop);drawing=true;enemyScale::drawArena(faded,id,f,image,p,opacity);drawing=false;if(!opacity)for(auto color:faded.pixels)assert(color==backdrop);}
   ++frameChecks;
  }
  assert(newIdle>oldIdle);if(id==3||id==5||id==9)assert(newIdle>oldIdle*2);report<<id<<','<<unsigned(profile.arenaWidth)<<','<<unsigned(profile.arenaHeight)<<','<<unsigned(profile.worldHeight)<<','<<int(profile.worldOffset)<<','<<oldIdle<<','<<newIdle<<','<<idEdges<<'\n';
  // Actual outdoor missile impact must track a sampled silhouette pixel,
  // including virtual roots, short animals and defeat fade.
  for(bool lethal:{false,true}){auto g=rpg::create(0,42);g.enemyId=id;g.enemyHp=lethal?0:100;g.phase=lethal?rpg::Phase::Won:rpg::Phase::Hero;
   ViewState v;v.page=Page::Battle;v.effect=Effect::MagicDarts;v.effectProgress=650;v.effectElapsedMs=715;v.effectFrame=5;v.combatResult={CombatFeedback::Damage,4,false,false};v.combatResult.hasHeroAction=true;v.combatResult.heroAction=rpg::Action::MagicMissile;v.combatResult.enemyDefeated=lethal;
   const auto f=enemyArt::body(id,enemyPose(true,v.combatResult,650,false));auto pilot=magePose(715);auto p=enemyScale::arena(id,f,pilot.enemyX,pilot.enemyY);Canvas blank;auto target=enemyScale::drawArena(blank,id,f,enemyArt::decode(f),p,enemyOpacity(true,v.combatResult,650,lethal));
   ContactProbe c;c.x=target.x+5+(715-540)/13;c.y=target.y;immutable(c,g,v);assert(c.found);++contacts;
  }
  // Render all poses and fade phases with each class; comparisons preserve the
  // actual outdoor HUD and every control pixel outside its existing scene band.
  for(unsigned cls=0;cls<4;++cls){auto g=rpg::create(cls,42);g.city=id%4;g.enemyId=id;g.enemyHp=rpg::encounterHp(g,id);g.phase=rpg::Phase::Hero;Canvas c,first;ViewState v;v.page=Page::Battle;v.effect=Effect::Slash;v.combatResult={CombatFeedback::Damage,4,false,true};v.effectOnHero=true;
   bool begin=true;for(unsigned progress:{0u,199u,200u,490u,491u,650u,818u,999u}){v.effectProgress=progress;v.effectFrame=progress*8/1000;immutable(c,g,v);if(begin){first=c;begin=false;}else for(int y=0;y<320;++y)if(y<86||y>=192)for(int x=0;x<240;++x)assert(c.pixels[y*240+x]==first.pixels[y*240+x]);++frameChecks;}
  }
 }
 // Missing art/decode memory keeps both scene paths transparent and repair UI usable.
 makeFallback();Canvas empty;empty.fillScreen(backdrop);
 for(unsigned id=0;id<29;++id){auto f=enemyArt::body(id,enemyArt::Pose::Idle);drawing=true;enemyScale::drawArena(empty,id,f,enemyArt::decode(f),enemyScale::arena(id,f),1000);drawing=false;}
 for(auto pixel:empty.pixels)assert(pixel==backdrop);
 for(unsigned i=0;i<32;++i)assert(memory[i]==0xa5&&memory[memory.size()-1-i]==0xa5);
 printf("PASS: scale77 %u frozen-state/pose/clip frames, 29 exact union profiles, increased outdoor opaque area for every species (>2x for the three reported foes), %u anchored outdoor impacts/fades, %u checked rim pixels; exact interior art, stable HUD/controls, empty fallback and no allocations.\n",frameChecks,contacts,edgePixels);
}
