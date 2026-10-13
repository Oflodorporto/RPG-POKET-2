#define main priorRenderRegression
#include "render.cpp"
#undef main
#include <climits>
struct TouchCanvas:Canvas {struct Label{int x,y,size;uint16_t color;std::string text;};std::vector<Label> labels;
 void print(const char* s){labels.push_back({tx,ty,size,color,s});Canvas::print(s);}
 bool has(const char* s)const{for(auto& l:labels)if(l.text==s)return true;return false;}
};
static bool overlap(scenicUi::Rect a,scenicUi::Rect b){return a.x<b.x+b.w&&a.x+a.w>b.x&&a.y<b.y+b.h&&a.y+a.h>b.y;}
int main(int argc,char** argv){assert(argc==2);std::string out=argv[1];artMemory=(uint8_t*)malloc(ART_BYTES);assert(artMemory);TouchCanvas c;
 std::vector<scenicUi::Rect> bag{panelUi::bagBack,panelUi::bagUse,panelUi::bagGear};for(unsigned i=0;i<9;++i){bag.push_back(panelUi::bagSlot(i));assert(bag.back().w==52&&bag.back().h==52);}
 std::vector<scenicUi::Rect> city{touchUi::cityMission};for(auto r:panelUi::cityButtons)city.push_back(r);
 std::vector<scenicUi::Rect> gear{touchUi::gearPrevious,touchUi::gearNext,touchUi::gearBack,touchUi::gearEquip,touchUi::gearSell};for(unsigned i=0;i<6;++i)gear.push_back({10+int(i%3)*76,74+int(i/3)*64,68,58});
 for(auto rects:{bag,city,gear})for(unsigned i=0;i<rects.size();++i){auto r=rects[i];assert(r.x>=0&&r.y>=0&&r.x+r.w<=240&&r.y+r.h<=320);for(unsigned j=0;j<i;++j)assert(!overlap(r,rects[j]));}
 assert(sizeof(bagSkin::plane)==38400&&sizeof(bagSkin::palette)==32);for(unsigned y=0;y<320;++y)for(unsigned x=0;x<240;++x){unsigned i=y*240+x;assert(bagSkin::pixel(x,y)==bagSkin::palette[(bagSkin::plane[i/2]>>(i&1?0:4))&15]);}assert(bagSkin::pixel(240,0)==0&&bagSkin::pixel(0,320)==0);
 Canvas before=c;bagSkin::crop(c,-1,0,240,320,0,0,240,320);bagSkin::crop(c,0,0,240,320,1,0,240,320);bagSkin::crop(c,0,0,240,320,0,0,241,320);bagSkin::crop(c,INT_MAX,0,1,1,0,0,1,1);bagSkin::crop(c,0,0,INT_MAX,1,0,0,1,1);bagSkin::crop(c,0,0,1,1,INT_MAX,0,1,1);assert(!memcmp(before.pixels,c.pixels,sizeof(c.pixels)));
 Canvas emptyBag;unsigned frames=0;
 for(bool fallback:{false,true}){if(fallback)makeFallback();else{std::ifstream f("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(f);f.seekg(16);f.read((char*)artMemory,ART_BYTES);assert(f.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);}
 for(unsigned slot=0;slot<3;++slot)for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(bool full:{false,true}){
  auto g=modern?rpg::create(cls,42+slot):rpg::testHero(cls,42+slot);g.tutorial=true;g.p.gold=full?999999:0;g.p.hp=g.p.maxhp=full?65535:18;g.p.mp=g.p.maxmp=full?65535:14;g.p.life=g.p.mana=full?99:0;g.crystals=g.rations=g.charts=g.charms=full?9:0;g.scrap=full?99:0;g.gazuas=full?9:0;g.sleepKit=full;g.owned=full?(1u<<rpg::GEAR_COUNT)-1:0;
  ViewState v;auto renderSafe=[&](){auto original=g;uint8_t a[rpg::SAVE_SIZE],b[rpg::SAVE_SIZE];rpg::encode(g,5,a);c.labels.clear();render(c,g,v);rpg::encode(g,5,b);assert(!memcmp(&g,&original,sizeof(g))&&!memcmp(a,b,sizeof(a)));++frames;};
  v.page=Page::TownBag;for(unsigned i=0;i<9;++i){v.choice=i;v.message=i==8?"ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890123":"";renderSafe();assert(c.has(i>=7?"Ver":"Usar")&&c.has("Voltar"));if(!fallback&&slot==0&&!modern&&!cls&&i==0){c.save(out+(full?"/bag-full.ppm":"/bag-empty.ppm"));if(!full)emptyBag=c;else for(unsigned icon=0;icon<7;++icon){auto r=panelUi::bagSlot(icon);for(int py=4;py<26;++py)for(int px=14;px<38;++px)assert(c.pixels[(r.y+py)*240+r.x+px]==emptyBag.pixels[(r.y+py)*240+r.x+px]);}}}if(fallback&&slot==0&&!modern&&!cls&&full)c.save(out+"/bag-fallback.ppm");
  v.page=Page::BagGear;v.message="ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890123";unsigned pages=std::max(1u,(unsigned(rpg::gearOwnedCount(g.owned))+5)/6);for(unsigned p=0;p<pages;++p)for(unsigned i=0;i<6;++i){v.gearIndex=p;v.choice=i;renderSafe();assert(c.has("Voltar")&&c.has("Equipar")&&c.has("Vender"));for(auto& l:c.labels)if(l.y==266)assert(l.x>=10&&l.x+int(l.text.size())*6*l.size<=230);if(!fallback&&slot==0&&!modern&&!cls&&full&&!p&&!i)c.save(out+"/equipment-full.ppm");}
  v.page=Page::Village;g.city=0;v.message="";renderSafe();assert(c.has("Conversar")&&c.has("Carvalho")&&c.has("Menu"));if(!fallback&&slot==0&&!modern&&!cls&&full)c.save(out+"/carvalho-max.ppm");if(fallback&&slot==0&&!modern&&!cls&&full)c.save(out+"/carvalho-fallback.ppm");
 }
 }
 free(artMemory);artMemory=nullptr;printf("PASS: %u bag/equipment/Carvalho240x320 frames, three slots, all selections/pages, old/new classes, empty/full/max values, missing-art fallback; fixed flash decoder and disjoint geometry; Game/Save28 unchanged.\n",frames);
}
