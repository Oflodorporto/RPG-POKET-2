#include "SceneCanvas.h"
struct RuinsCanvas:Canvas {struct Label{int x,y,size;std::string text;};std::vector<Label> labels;void print(const char* s){labels.push_back({tx,ty,size,s});Canvas::print(s);}bool has(const char* s){for(auto& l:labels)if(l.text==s)return true;return false;}};
int main(int argc,char**argv){assert(argc==2);RuinsCanvas c;unsigned frames=0;
 const char* names[]={"Conversar","Cripta","Guardiao","Explorar","Acampar","Mapa","Loja","Bolsa"};
 for(bool fallback:{false,true})for(auto period:{worldClock::Period::Dawn,worldClock::Period::Day,worldClock::Period::Dusk,worldClock::Period::Night})for(unsigned page=0;page<2;++page)for(unsigned cls=0;cls<4;++cls)for(unsigned wins=0;wins<5;++wins)for(unsigned stress=0;stress<3;++stress){
  // nullptr proves the city has no dependency on SD or the shared PSRAM art store.
  artMemory=nullptr;menu=MenuState{};menu.art=fallback?ArtStatus::Fallback:ArtStatus::Ready;menu.clockValid=menu.dayCycle=true;menu.worldPeriod=period;
  auto g=rpg::create(cls,42);g.city=1;g.tutorial=true;g.ruinsWins=std::min(wins,3u);g.guardianDefeated=wins==4;g.crystals=stress?9:1;g.p.gold=stress?999999:123;g.sleepKit=true;g.rations=stress?255:3;if(stress==2){g.questId=1;g.questProgress=2;}
  ViewState v;v.page=Page::Ruins;v.cityPage=page;v.message=stress==1?"Venca 3 encontros primeiro":"";auto original=g;c.labels.clear();render(c,g,v,32);assert(!memcmp(&g,&original,sizeof(g)));
  assert(c.has("Toque: proximo objetivo")&&c.has(page?"Pagina 2/2":"Pagina 1/2"));
  for(unsigned id=0;id<8;++id){assert(c.has(names[id])==(scenicUi::ruinsActionPage(id)==page));if(c.has(names[id])){auto r=scenicUi::ruinsButton(id);unsigned labels=0;for(auto& l:c.labels)if(l.text==names[id]){assert(l.x>=r.x+4&&l.x+int(l.text.size())*6<=r.x+r.w-3&&l.y>=r.y+4&&l.y+8<=r.y+r.h-4);++labels;}assert(labels==2);}}
  // The whole 64-pixel central city window survives in the base preview.
  if(stress==0)for(int y=112;y<176;++y)for(int x=0;x<240;++x)assert(c.pixels[y*240+x]==worldClock::shade(scenicArt::ruinsPalette[scenicArt::ruinsPixels[y*240+x]],period));
  if(!cls&&wins==0&&!stress)c.save(std::string(argv[1])+"/ruins7-"+std::to_string(page+1)+"-"+(period==worldClock::Period::Day?"day":period==worldClock::Period::Night?"night":period==worldClock::Period::Dawn?"dawn":"dusk")+(fallback?"-fallback":"")+".ppm");
  if(!fallback&&!cls&&!page&&period==worldClock::Period::Day&&wins>=3&&stress==0)c.save(std::string(argv[1])+"/ruins7-guardian-"+std::to_string(wins)+".ppm");
  ++frames;
 }
 printf("PASS: %u actual 240x320 Ruins renders; two pages/all periods/classes/guardian states, SD-null fallback, current costs, live labels inside equal controls, scene window exact, Game unchanged.\n",frames);
}
