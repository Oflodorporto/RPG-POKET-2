#define main existingRendererRegression
#include "render.cpp"
#undef main
#include <climits>
// This matrix is new validation on the exact accessible baseline, not a reuse
// of the unavailable leather-v2 prototype's historical test results.
static bool auditAllocations=false;
static unsigned allocations=0;
extern "C" void* __real_malloc(size_t);
extern "C" void* __real_calloc(size_t,size_t);
extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){if(auditAllocations)++allocations;return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t s){if(auditAllocations)++allocations;return __real_calloc(n,s);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(auditAllocations)++allocations;return __real_realloc(p,n);}
int main(int argc,char** argv){
 assert(argc==2);std::string out=argv[1];Canvas c;artMemory=(uint8_t*)malloc(ART_BYTES);assert(artMemory);
 unsigned frames=0;assert(sizeof(bagSkin::plane)+sizeof(bagSkin::palette)==38432);
 for(bool fallback:{false,true}){
  menu.art=fallback?ArtStatus::Fallback:ArtStatus::Ready;
  if(fallback)makeFallback();else{std::ifstream f("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(f);f.seekg(16);f.read((char*)artMemory,ART_BYTES);assert(f.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);}
  for(unsigned slot=0;slot<3;++slot)for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(bool full:{false,true}){
   auto g=modern?rpg::create(cls,42+slot):rpg::testHero(cls,42+slot);g.tutorial=true;
   g.p.gold=full?999999:0;g.p.hp=g.p.maxhp=full?65535:18;g.p.mp=g.p.maxmp=full?65535:14;
   g.p.life=g.p.mana=full?99:0;g.crystals=g.charts=g.charms=full?9:0;g.rations=full?10:0;g.scrap=full?99:0;g.gazuas=full?9:0;g.sleepKit=full;g.owned=full?(1u<<rpg::GEAR_COUNT)-1:0;
   for(Page page:{Page::Bag,Page::TownBag})for(unsigned choice=0;choice<9;++choice){
    ViewState v{};v.page=page;v.choice=choice;v.message=choice==8?"ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890123":"";
    auto original=g;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,5,before);
    unsigned n=allocations;auditAllocations=true;render(c,g,v);auditAllocations=false;
    assert(allocations==n);rpg::encode(g,5,after);assert(!memcmp(&g,&original,sizeof(g))&&!memcmp(before,after,sizeof(before)));
    auto r=panelUi::bagSlot(choice);assert(r.x==34+int(choice%3)*60&&r.y==38+int(choice/3)*56&&r.w==52&&r.h==52);
    // Existing double selection outline must remain visibly distinct.
    assert(c.pixels[r.y*240+r.x]==0xffff&&c.pixels[(r.y+1)*240+r.x+1]==UI_GOLD);
    if(slot==0&&!modern&&!cls&&page==Page::TownBag){
     std::string name=std::string(fallback?"fallback":"ready")+(full?"-full-":"-empty-")+std::to_string(choice);c.save(out+"/"+name+".ppm");
    }
    ++frames;
   }
  }
 }
 // Non-bag rendering must stay byte-identical in the before/after comparison.
 menu.art=ArtStatus::Ready;{std::ifstream f("cartao/RPGPOKET/artes.pak",std::ios::binary);f.seekg(16);f.read((char*)artMemory,ART_BYTES);assert(f.gcount()==ART_BYTES);}
 auto g=rpg::create(0,42);g.tutorial=true;g.owned=(1u<<rpg::GEAR_COUNT)-1;ViewState v{};
 for(Page page:{Page::BagGear,Page::Village,Page::Character,Page::Menu}){v.page=page;render(c,g,v);c.save(out+"/unchanged-"+std::to_string(int(page))+".ppm");}
 free(artMemory);artMemory=nullptr;
 printf("PASS: %u fresh bag frames; both bag pages; all 9 selections; 3 slots; 4 classes; legacy/new; empty/max incl 10 rations; ready/fallback; unchanged Game/Save; zero render heap allocations; 38432-byte existing flash texture.\n",frames);
}
