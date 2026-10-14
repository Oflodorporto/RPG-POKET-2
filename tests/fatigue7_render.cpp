#define main priorFatigue7Render
#include "render.cpp"
#undef main
#include <filesystem>
struct ReceiptCanvas:Canvas {
 struct Label{int x,y,size;std::string text;};std::vector<Label> labels;
 void print(const char* s){labels.push_back({tx,ty,size,s});Canvas::print(s);}
 bool has(const char* s)const{for(const auto& l:labels)if(l.text.find(s)!=std::string::npos)return true;return false;}
};

static void frame(ReceiptCanvas& c,const rpg::Game& g,const ViewState& v){
 auto before=g;auto state=v;c.labels.clear();render(c,g,v);
 assert(!memcmp(&g,&before,sizeof(g))&&!memcmp(&v,&state,sizeof(v)));
 for(const auto& l:c.labels)assert(l.x>=0&&l.y>=0&&l.x+int(l.text.size())*6*l.size<=240&&l.y+8*l.size<=320);
}
int main(int argc,char** argv){
 assert(argc==2);std::string out=argv[1];std::filesystem::create_directories(out);
 artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);
 std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES);
 ReceiptCanvas c;unsigned frames=0;
 for(unsigned cls=0;cls<4;++cls)for(unsigned city=0;city<4;++city)
 for(unsigned level:{1u,20u})for(unsigned fatigue:{69u,70u,71u,100u}){
  auto g=rpg::create(cls,1);g.p.xp=rpg::dndXp[level-1];rpg::levelUp(g);g.fatigue=fatigue;g.city=city;
  ViewState v;v.page=Page::Home;frame(c,g,v);++frames;
  assert(c.has(fatigue>70?"Combate: dano -":"Combate: sem penalidade ate 70%"));
  if(!cls&&city==1&&level==1)c.save(out+"/home-fatigue"+std::to_string(fatigue)+".ppm");
  g.phase=rpg::Phase::Hero;g.enemyId=2;g.enemyHp=17;
  for(Page page:{Page::Battle,Page::Dungeon}){
   v.page=page;g.dungeonFlags=page==Page::Dungeon?5:0;g.dungeonXY=0x72;
   frame(c,g,v);++frames;char badge[32];snprintf(badge,sizeof(badge),"Fadiga %u%% -%u dano",fatigue,fatigue>=90?2:1);
   assert(c.has(badge)==(fatigue>70));
   if(cls<2&&city==1&&level==1)c.save(out+"/battle"+std::to_string(unsigned(page))+"-class"+std::to_string(cls)+"-fatigue"+std::to_string(fatigue)+".ppm");
   // Skip keeps its existing hit region and never collides with a status badge.
   v.effect=Effect::Slash;frame(c,g,v);++frames;assert(c.has("Pular >")&&!c.has(badge));v.effect=Effect::None;
  }
 }
 for(unsigned cls=0;cls<4;++cls)for(unsigned fatigue:{69u,70u,71u,100u})
 for(unsigned seed:{1u,94u,374u})for(unsigned a=0;a<=unsigned(rpg::Action::DivineSmite);++a){
  auto g=rpg::create(cls,seed);g.p.xp=rpg::dndXp[19];rpg::levelUp(g);g.oath=1;g.fatigue=fatigue;
  g.p.hp=std::max(1u,unsigned(g.p.maxhp)/2);g.phase=rpg::Phase::Hero;g.enemyId=19;g.enemyHp=240;
  ViewState v;v.page=Page::Battle;if(rpg::act(g,rpg::Action(a),&v.heroTrace))continue;assert(!v.heroTrace.overflow);
  v.traceOpen=true;bool found=false;
  for(unsigned page=0;page<combatTraceUi::pageCount(v.heroTrace);++page){v.tracePage=page;frame(c,g,v);++frames;found|=c.has("Fadiga: max(1,");}
  assert(found==(fatigue>70&&v.heroTrace.damaging&&v.heroTrace.net>0));
 }
 // A real 3d6=4+2+4 receipt: save halves 10 to 5, then fatigue makes 4 / 3.
 for(unsigned fatigue:{70u,71u,100u}){auto g=rpg::create(0,1);g.p.xp=rpg::dndXp[2];rpg::levelUp(g);g.phase=rpg::Phase::Hero;g.enemyHp=17;g.fatigue=fatigue;ViewState v;v.page=Page::Battle;v.traceOpen=true;assert(!rpg::act(g,rpg::Action::BurningHands,&v.heroTrace));assert(v.heroTrace.potential==10&&v.heroTrace.net==(fatigue==70?5:fatigue==71?4:3));frame(c,g,v);++frames;c.save(out+"/real-fire-fatigue"+std::to_string(fatigue)+".ppm");}
 free(artMemory);artMemory=nullptr;printf("PASS fatigue7 render: %u frames, all classes, 69/70/71/100, home/combat/dungeon badges, bounded receipt labels and real fire/saving-throw example; pure Game/View/RNG\n",frames);
 return 0;
}
