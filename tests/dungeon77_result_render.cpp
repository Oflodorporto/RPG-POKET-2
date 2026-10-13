#define main priorRenderRegression
#include "render.cpp"
#undef main
struct ResultCanvas:Canvas {
 struct Label{int x,y,size;uint16_t color;std::string text;};std::vector<Label> labels;
 void print(const char* s){labels.push_back({tx,ty,size,color,s});Canvas::print(s);}
 bool has(const char* s)const{for(const auto& l:labels)if(l.text==s)return true;return false;}
 void glyphsVisible(const Label& l)const{
  for(unsigned n=0;n<l.text.size();++n)for(int i=0;i<5;++i)for(int j=0;j<8;++j)if(font[uint8_t(l.text[n])*5+i]&(1<<j))
   for(int yy=0;yy<l.size;++yy)for(int xx=0;xx<l.size;++xx)assert(pixels[(l.y+j*l.size+yy)*240+l.x+(n*6+i)*l.size+xx]==l.color);
 }
};
int main(int argc,char** argv){
 assert(argc==2);const std::string out=argv[1];artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);
 std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 unsigned count=0;ResultCanvas c;
 for(unsigned cls=0;cls<4;++cls)for(auto phase:{rpg::Phase::Home,rpg::Phase::Hero,rpg::Phase::Enemy,rpg::Phase::Won,rpg::Phase::Lost,rpg::Phase::Fled})for(bool active:{false,true}){
  auto g=rpg::testHero(cls,42);g.city=1;g.crystals=1;assert(!rpg::enterDungeon(g));g.dungeonXY=0x72;g.phase=phase;g.enemyId=2;g.enemyHp=phase==rpg::Phase::Won?0:5;g.gainXp=3276;g.gainGold=255;
  ViewState v;v.page=Page::Dungeon;v.message="Explore e encontre o selo";v.effect=active?Effect::Slash:Effect::None;v.effectProgress=900;v.effectFrame=7;v.combatResult={CombatFeedback::Damage,5,false,false};v.combatResult.enemyDefeated=phase==rpg::Phase::Won;
  uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,19,before);auto state=g;c.labels.clear();render(c,g,v);rpg::encode(g,19,after);assert(!memcmp(before,after,sizeof(before))&&!memcmp(&g,&state,sizeof(g)));++count;
  bool pending=dungeonUi::pendingResult(g);assert(c.has("Continuar")==bool(pending&&!active));assert(c.has("Aguarde...")==bool(pending&&active));assert(c.has("Avancar")==!pending);assert(c.has("Girar <")==!pending);assert(c.has("Bolsa")&&c.has("Menu"));
  for(const auto& l:c.labels){if(l.color==UI_GREEN){assert(l.y==192&&l.y+8<=202);c.glyphsVisible(l);}if(l.text=="Continuar"||l.text=="Aguarde..."){assert(dungeonUi::continueHit(l.x,l.y)&&dungeonUi::continueHit(l.x+int(l.text.size())*6*l.size-1,l.y+8*l.size-1));c.glyphsVisible(l);}}
  if(cls==0&&!active)c.save(out+"/dungeon-state-"+std::to_string(unsigned(phase))+".ppm");
  if(pending){v.page=Page::DungeonMenu;c.labels.clear();render(c,g,v);assert(c.has("Continuar")&&c.has("Bolsa")&&c.has("Voltar")&&!c.has("Tecnica")&&!c.has("Sair da dungeon"));rpg::encode(g,19,after);assert(!memcmp(before,after,sizeof(before)));++count;if(cls==0&&!active)c.save(out+"/menu-state-"+std::to_string(unsigned(phase))+".ppm");}
 }

 // The reported normal-battle HUD and Skills overlay are separate screens.
 const auto guide=panelUi::skillsGuide;
 assert(guide.x+guide.w<=90&&guide.y+guide.h<=140&&guide.y>=89);
 for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls){
  auto g=modern?rpg::create(cls,42):rpg::testHero(cls,42);g.p.level=20;g.phase=rpg::Phase::Hero;
  ViewState v;v.page=Page::Skills;
  for(const char* message:{"","Mana insuficiente","ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789012"}){v.message=message;auto before=g;c.labels.clear();render(c,g,v);assert(!memcmp(&g,&before,sizeof(g)));assert(c.has("Guia"));for(const auto& l:c.labels)if(l.text=="Guia"&&l.color==UI_GOLD){assert(guide.contains(l.x,l.y)&&guide.contains(l.x+int(l.text.size())*6*l.size-1,l.y+8*l.size-1));c.glyphsVisible(l);}++count;}
  if(cls==0){v.message="";render(c,g,v);c.save(out+"/skills-guide-"+std::to_string(modern)+".ppm");}
  v.page=Page::Battle;
  for(unsigned id=0;id<29;++id)for(bool extreme:{false,true}){g.enemyId=id;g.enemyHp=extreme?UINT16_MAX:rpg::encounterHp(g,id);auto before=g;c.labels.clear();render(c,g,v);assert(!memcmp(&g,&before,sizeof(g)));bool hp=false,ficha=false;
   for(const auto& l:c.labels){if(l.text=="Ficha >"&&l.color==UI_GOLD){assert(l.y==75&&l.y+8<=84);c.glyphsVisible(l);ficha=true;}
    if(l.text.rfind("HP ",0)==0&&l.y<90&&l.color==UI_WHITE){assert(l.y==63&&l.y+8<75);char full[48];snprintf(full,sizeof(full),"HP %u/%u",g.enemyHp,rpg::encounterHp(g,id));assert(l.text==full);c.glyphsVisible(l);hp=true;}
    if(l.text==rpg::enemySpec(id).name&&l.color==UI_GOLD&&l.y<60){assert(l.y+8*l.size<55);c.glyphsVisible(l);}}
   assert(hp&&ficha);++count;if(cls==0&&!modern&&id==3&&!extreme)c.save(out+"/battle-hud-guardian.ppm");
  }
 }
 free(artMemory);artMemory=nullptr;printf("PASS: %u 240x320 dungeon/result/menu/Skills/29-enemy HUD frames; explicit Continue/waiting vs movement controls, complete status glyphs above arrows, matching hitbox, unchanged Game/Save28.\n",count);
}
