#include "SceneCanvas.h"
struct BagCanvas:Canvas {struct Label{int x,y,size;std::string text;};std::vector<Label> labels;void print(const char* s){labels.push_back({tx,ty,size,s});Canvas::print(s);}};
int main(int argc,char** argv){assert(argc==2);BagCanvas c;unsigned frames=0;auto bag=scenicUi::ruinsButtons[scenicUi::ruinsBag];
 for(bool fallback:{false,true})for(auto period:{worldClock::Period::Dawn,worldClock::Period::Day,worldClock::Period::Dusk,worldClock::Period::Night})for(unsigned cls=0;cls<4;++cls)for(unsigned wins=0;wins<=3;++wins)for(unsigned frame:{0u,1u,32u,255u}){
  menu.art=fallback?ArtStatus::Fallback:ArtStatus::Ready;menu.clockValid=menu.dayCycle=true;menu.worldPeriod=period;auto g=rpg::create(cls,42);g.city=1;g.ruinsWins=wins;g.guardianDefeated=wins==3;g.tutorial=true;ViewState v;v.page=Page::Ruins;v.message=frame==255?"Venca 3 encontros primeiro":"";auto original=g;c.labels.clear();drawScenicRuins(c,g,v,frame);assert(!memcmp(&g,&original,sizeof(g)));unsigned labels=0;for(auto& label:c.labels)if(label.text=="Bolsa"){++labels;assert(label.size==1&&label.x>=bag.x+4&&label.x+30<=bag.x+bag.w-4&&label.y>=bag.y+4&&label.y+8<=bag.y+bag.h-4);}assert(labels==2);
  assert(c.pixels[bag.y*240+bag.x]==scenicArt::ruinsPalette[scenicArt::ruinsPixels[246*240+52]]);
  if(!fallback&&cls==2&&!wins&&!frame){const char* name=period==worldClock::Period::Day?"ruins-bag-day":period==worldClock::Period::Night?"ruins-bag-night":period==worldClock::Period::Dawn?"ruins-bag-dawn":"ruins-bag-dusk";c.save(std::string(argv[1])+"/"+name+".ppm");}++frames;
 }
 printf("PASS: %u Ruins bag render frames, ready/fallback, all day periods/classes/progress, bold label/flash art inside shared 88x44 rectangle, no Game mutation.\n",frames);
}
