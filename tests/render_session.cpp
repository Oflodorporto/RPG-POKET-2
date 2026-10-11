#define main isolatedRenderRegression
#include "render.cpp"
#undef main
int main(){
 artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);makeFallback();
 Canvas c;ViewState v;rpg::Game g;unsigned frames=0;
 const Page pages[]={Page::Title,Page::Home,Page::Menu,Page::Village,Page::Explore,Page::Map,Page::TownBag,Page::Character,Page::Progression,Page::Guide,Page::Settings,Page::Wifi,Page::Tests,Page::Battle,Page::Skills,Page::Result,Page::SaveError,Page::FirstTip};
 for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city){
  g=rpg::create(cls,71);g.p.xp=rpg::dndXp[19];rpg::levelUp(g);rpg::rest(g);g.city=city;g.tutorial=true;g.p.gold=999999;g.p.life=g.p.mana=99;
  for(unsigned cycle=0;cycle<12;++cycle)for(Page page:pages){v=ViewState{};v.page=page;v.hintIndex=cycle%10;v.evolutionLevel=20;menu=MenuState{};menu.destination=(city+1)%4;menu.touches=cycle*2000;menu.touchErrors=cycle*100;menu.frameMs=120;menu.pollMs=15;
   uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);auto memory=artMemory;
   render(c,g,v,cycle);rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before))&&artMemory==memory);++frames;
  }
 }
 free(artMemory);artMemory=nullptr;
 printf("PASS: %u repeated render frames / 18 screens / 4 classes / 4 regions; text bounds, immutable saves, fixed art buffer\n",frames);
}
