#define main priorRenderRegression
#include "render.cpp"
#undef main
struct ReceiptCanvas:Canvas {
 struct Label{int x,y,size;std::string text;};std::vector<Label> labels;
 void print(const char* s){labels.push_back({tx,ty,size,s});Canvas::print(s);}
 bool has(const char* s)const{for(const auto& l:labels)if(l.text.find(s)!=std::string::npos)return true;return false;}
};
int main(int argc,char** argv){
 assert(argc==2);std::string out=argv[1];artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES);
 ReceiptCanvas c;unsigned frames=0;
 for(unsigned cls=0;cls<4;++cls)for(unsigned a=0;a<18;++a)for(unsigned seed:{1u,94u,374u}){
  auto g=rpg::create(cls,seed);g.p.xp=rpg::dndXp[19];rpg::levelUp(g);g.oath=1;g.p.hp=std::max(1u,unsigned(g.p.maxhp)/2);g.p.life=g.p.mana=2;g.phase=rpg::Phase::Hero;g.enemyId=19;g.enemyHp=240;--g.p.mp;
  ViewState v;v.page=Page::Battle;if(rpg::act(g,rpg::Action(a),&v.heroTrace))continue;v.traceOpen=true;
  for(unsigned page=0;page<combatTraceUi::pageCount(v.heroTrace);++page){v.tracePage=page;auto before=g;auto vv=v;c.labels.clear();render(c,g,v);assert(!memcmp(&g,&before,sizeof(g))&&!memcmp(&v,&vv,sizeof(v)));assert(c.has("Resultado ja aplicado"));for(auto l:c.labels){assert(l.x>=0&&l.y>=0&&l.x+int(l.text.size())*6*l.size<=240&&l.y+8*l.size<=320);}++frames;
   if(seed==1&&(a==unsigned(rpg::Action::Fireball)||a==unsigned(rpg::Action::Attack)||a==unsigned(rpg::Action::DivineSmite)))c.save(out+"/receipt-"+std::to_string(cls)+"-"+std::to_string(a)+"-"+std::to_string(page)+".ppm");
  }
 }
 // Receipt controls reserve the top strip, including terminal results.
 for(auto page:{Page::Result,Page::CampaignResult,Page::EventResult,Page::DungeonVictory})for(auto phase:{rpg::Phase::Won,rpg::Phase::Lost,rpg::Phase::Fled})for(unsigned stage:{1u,4u,8u}){
  auto g=rpg::create(0,1);g.phase=phase;g.enemyHp=phase==rpg::Phase::Won?0:10;g.campaignStage=stage;g.eventKind=3;g.eventStage=2;ViewState v;v.page=page;v.paperAnimate=false;c.labels.clear();render(c,g,v);
  for(auto l:c.labels)if(l.text!="Dados >"&&l.text!="Pular >"&&l.text.size()){int right=l.x+int(l.text.size())*6*l.size,bottom=l.y+8*l.size;assert(!(l.y<22&&bottom>0&&l.x<110&&right>4));}
  if(stage==1)c.save(out+"/terminal-"+std::to_string(unsigned(page))+"-"+std::to_string(unsigned(phase))+".ppm");++frames;
 }
 // Locate and display the concrete 3d6=4+2+4, successful save => 5 example.
 bool example=false;for(unsigned seed=1;seed<200000&&!example;++seed){auto g=rpg::create(0,seed);g.p.xp=rpg::dndXp[2];rpg::levelUp(g);g.phase=rpg::Phase::Hero;g.enemyId=2;g.enemyHp=17;ViewState v;v.page=Page::Battle;assert(!rpg::act(g,rpg::Action::BurningHands,&v.heroTrace));auto& t=v.heroTrace;if(t.dice[0]==4&&t.dice[1]==2&&t.dice[2]==4&&t.net==5){assert(t.potential==10&&t.applied==5);v.traceOpen=true;c.labels.clear();render(c,g,v);assert(c.has("4+2+4=10")&&c.has("10 / 2 = 5"));c.save(out+"/fire-real-4-2-4-save-half.ppm");v.traceOpen=false;v.effect=Effect::FlameVolley;v.effectElapsedMs=320;v.effectFrame=2;v.effectProgress=290;v.combatResult=heroCombatResult(g,rpg::Action::BurningHands);render(c,g,v);c.save(out+"/fire-reveal-real-4-2-4.ppm");auto heroReceipt=v.heroTrace;assert(rpg::enemy(g,&v.enemyTrace));assert(!memcmp(&heroReceipt,&v.heroTrace,sizeof(heroReceipt)));v.effect=Effect::None;v.traceOpen=true;v.traceEnemy=true;render(c,g,v);c.save(out+"/receipt-reopened-enemy-action.ppm");v.traceEnemy=false;render(c,g,v);c.save(out+"/receipt-hero-after-enemy-action.ppm");printf("Concrete real fire example seed=%u: 4+2+4=10, save halves to 5\n",seed);example=true;}}
 assert(example);
 {auto g=rpg::create(0,1);g.phase=rpg::Phase::Enemy;g.damage=5;ViewState v;v.page=Page::Battle;v.traceOpen=true;render(c,g,v);c.save(out+"/receipt-reloaded-summary.ppm");assert(c.has("Nenhuma rolagem foi refeita."));}
 for(bool dungeon:{false,true}){auto g=rpg::create(0,42);g.p.xp=rpg::dndXp[4];rpg::levelUp(g);g.city=1;g.crystals=1;if(dungeon){assert(!rpg::enterDungeon(g,0));g.dungeonXY=0x72;}g.phase=rpg::Phase::Hero;g.enemyId=2;g.enemyHp=17;ViewState v;v.page=dungeon?Page::Dungeon:Page::Battle;assert(!rpg::act(g,rpg::Action::Fireball,&v.heroTrace));v.effect=Effect::FireBurst;v.combatResult=heroCombatResult(g,rpg::Action::Fireball);for(unsigned ms:{0u,240u,480u,900u}){v.effectElapsedMs=ms;v.effectFrame=ms*8/1200;v.effectProgress=ms*1000/1200;auto before=g;render(c,g,v);assert(!memcmp(&g,&before,sizeof(g)));c.save(out+"/reveal-"+std::to_string(dungeon)+"-"+std::to_string(ms)+".ppm");++frames;}}
 free(artMemory);artMemory=nullptr;printf("PASS: %u trace pages/reveal frames, bounded labels, render purity, real fire and honest reload summary\n",frames);
}
