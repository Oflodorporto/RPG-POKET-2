#define main previousRenderRegression
#include "render.cpp"
#undef main
#include <climits>
#include <array>
struct CityCanvas:Canvas {std::vector<std::string> labels;void print(const char* s){labels.push_back(s);Canvas::print(s);}bool has(const char* s)const{return std::find(labels.begin(),labels.end(),s)!=labels.end();}};
int main(int argc,char** argv){assert(argc==2);std::string out=argv[1];artMemory=(uint8_t*)malloc(ART_BYTES);assert(artMemory);CityCanvas c;unsigned frames=0;
 for(bool fallback:{false,true}){menu.art=fallback?ArtStatus::Fallback:ArtStatus::Ready;if(fallback)makeFallback();else {std::ifstream f("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(f);f.seekg(16);f.read((char*)artMemory,ART_BYTES);assert(f.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);}
  for(unsigned slot=0;slot<3;++slot)for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned page=0;page<2;++page)for(unsigned stress=0;stress<3;++stress){auto g=modern?rpg::create(cls,42+slot):rpg::testHero(cls,42+slot);g.tutorial=true;g.p.gold=stress==0?24:stress==1?999999:0;ViewState v;v.page=Page::Village;v.cityPage=page;v.message=stress==2?"ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890123":"";auto before=g;uint8_t a[rpg::SAVE_SIZE],b[rpg::SAVE_SIZE];rpg::encode(g,77,a);c.labels.clear();render(c,g,v);rpg::encode(g,77,b);assert(!memcmp(&g,&before,sizeof(g))&&!memcmp(a,b,sizeof(a)));assert(c.has("Menu")&&c.has(page?"Pagina 2/2":"Pagina 1/2")&&c.has(page?"< Voltar":"Mais >"));assert(c.has(page?"Heroi":"Conversar"));if(fallback)assert(c.has("Carvalho"));if(stress==1)assert(c.has("999999 g"));
   if(!fallback){for(int y=112;y<176;++y)for(int x=0;x<240;++x)assert(c.pixels[y*240+x]==carvalhoUi::sourcePixel(x,y));for(int y=0;y<50;++y)for(int x=0;x<240;++x)assert(c.pixels[y*240+x]==carvalhoUi::sourcePixel(x,y));}
   if(!slot&&!modern&&!cls){c.save(out+"/city-"+std::to_string(page+1)+(fallback?"-fallback":stress?"-stress":"-approved")+".ppm");if(!fallback&&!stress){const uint32_t approved[]={0x2e9ef52d,0xfd0ddf90};assert(rpg::crc((uint8_t*)c.pixels,sizeof(c.pixels))==approved[page]);}}
   ++frames;
  }
 }
 Canvas original=c;for(auto args:{std::array<int,8>{-1,0,1,1,0,0,1,1},{0,0,INT_MAX,1,0,0,1,1},{0,0,1,1,INT_MAX,0,1,1},{0,0,1,1,0,0,241,1}})carvalhoUi::crop(c,args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);assert(!memcmp(original.pixels,c.pixels,sizeof(c.pixels)));assert(carvalhoUi::sourcePixel(240,0)==UI_INK&&carvalhoUi::sourcePixel(0,320)==UI_INK);
 free(artMemory);artMemory=nullptr;printf("PASS: %u city frames; pixel-exact approved references, scene/title untouched, values/text/fallback bounds,3slots/legacy-new, no Game/Save mutation.\n",frames);
}
