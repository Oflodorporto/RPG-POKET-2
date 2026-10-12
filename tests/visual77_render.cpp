#define main originalRenderTests
#include "render.cpp"
#undef main

struct VisualCanvas : Canvas {
 struct Label {int x,y;std::string text;};
 std::vector<std::string> labels;std::vector<Label> positions;
 void print(const char* text){labels.emplace_back(text);positions.push_back({tx,ty,text});Canvas::print(text);}
 bool has(const char* text)const{return std::find(labels.begin(),labels.end(),text)!=labels.end();}
 void clear(){labels.clear();positions.clear();}
};
static void immutableRender(VisualCanvas& c,const rpg::Game& g,const ViewState& v,unsigned frame=0){
 uint8_t before[sizeof(g)],save[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];memcpy(before,&g,sizeof(g));rpg::encode(g,77,save);
 c.clear();render(c,g,v,frame);rpg::encode(g,77,after);
 assert(!memcmp(before,&g,sizeof(g))&&!memcmp(save,after,sizeof(save)));
}
int main(int argc,char** argv){
 assert(argc==2);std::string root=argv[1];VisualCanvas c;auto g=rpg::create(0,77);ViewState v;v.page=Page::Updates;
 // Opaque OTA screen works without even allocating the art buffer; no SD fallback pixels needed.
 assert(!artMemory);menu=MenuState{};unsigned frames=0;
 for(unsigned state=0;state<=unsigned(updater::State::Testing);++state)for(bool busy:{false,true})for(bool connected:{false,true})for(bool resume:{false,true})for(bool artOnly:{false,true})for(unsigned percent:{0u,50u,100u,101u}){
  updateInfo=updater::Info{};updateInfo.state=updater::State(state);updateInfo.busy=busy;updateInfo.canResume=resume;updateInfo.artOnly=artOnly;updateInfo.progress=percent;
  const char* version="2026.10.12-abcdefghijklmnopqrstuvwxyz12";assert(strlen(version)==39);strcpy(updateInfo.version,version);
  strcpy(updateInfo.message,"12345678901234567890123456789012345678");
  menu.connected=connected;immutableRender(c,g,v);++frames;
  assert(c.has("ATUALIZACAO"));assert(c.has("123456789012345678901234567890123456"));assert(c.has("78"));
  if(!busy){assert(c.has("2026.10.12-abcdefghijklmnopqrstuvwxy"));assert(c.has("z12"));}
  if(busy){assert(!c.has("Instalar")&&!c.has("Voltar"));assert(c.has("Nao retire o cartao ou a energia."));}
  else if(updateInfo.state==updater::State::Available||resume){assert(c.has("Instalacao exige cartao SD."));assert(c.has(resume?"Continuar":"Instalar"));assert(c.has("Cancelar"));}
  else assert(c.has("Verificar versao")&&c.has("Voltar"));
 }
 // A short first word must not strand a 37-character token beyond the last row.
 {const char* edge="a abcdefghijklmnopqrstuvwxyz12345678901";assert(strlen(edge)==39);c.clear();updateLines(c,119,edge);assert(c.has(std::string(edge,36).c_str()));assert(c.has(edge+36));}
 menu.connected=false;updateInfo=updater::Info{};updateInfo.state=updater::State::Downloading;updateInfo.busy=true;updateInfo.progress=54;strcpy(updateInfo.message,"Baixando arquivo 2/2");immutableRender(c,g,v);c.save(root+"/update-download.ppm");
 updateInfo.busy=false;updateInfo.state=updater::State::Error;strcpy(updateInfo.message,"Cartao SD indisponivel; tente novamente");immutableRender(c,g,v);c.save(root+"/update-error-no-art.ppm");
 updateInfo.state=updater::State::Available;updateInfo.artOnly=true;strcpy(updateInfo.version,"2026.10.12-fatigue76");strcpy(updateInfo.message,"Artes precisam de reparo");immutableRender(c,g,v);c.save(root+"/update-art-repair.ppm");
 artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 updateInfo=updater::Info{};
 for(unsigned city=0;city<4;++city)for(unsigned destination=0;destination<4;++destination)for(unsigned fatigue:{0u,89u,90u,100u})for(bool supplies:{false,true}){
  g=rpg::create(0,77);g.city=city;g.fatigue=fatigue;g.rations=g.charts=g.charms=supplies?9:0;v.page=Page::TravelConfirm;v.tripDestination=destination;immutableRender(c,g,v);++frames;
  assert(c.has("Cancelar")&&c.has("Viajar"));assert(c.pixels[150*240+16]==0x10c6);assert(c.pixels[220*240+16]==0x10c6);
  if(city==0&&destination==3&&fatigue==100&&!supplies)c.save(root+"/travel-fatigue100-supplies0.ppm");
 }
 g=rpg::create(0,77);assert(!rpg::prepareTrip(g,3));menu.rollReady=true;v.page=Page::TravelRoll;immutableRender(c,g,v,24);c.save(root+"/travel-d20-result.ppm");
 assert(c.pixels[230*240+9]==UI_INK&&c.pixels[260*240+229]==UI_INK);
 menu.journey.start(0,3,0);menu.journey.tick(1500);v.page=Page::Travel;immutableRender(c,g,v,12);c.save(root+"/travel-map-1500.ppm");

 // All primitives share the arena clip, including partially visible bitmaps.
 {Canvas clip;clip.fillScreen(0x1234);PanelEffectCanvas<Canvas> arena(clip);uint16_t pixels[20*20];std::fill(pixels,pixels+400,0xabcd);
  arena.fillRect(-20,-100,300,500,0);arena.drawRect(-1,-1,243,330,1);arena.draw16bitRGBBitmap(-10,150,pixels,20,20);
  for(int y=0;y<320;++y)for(int x=0;x<240;++x)if(y<86||y>=192)assert(clip.pixels[y*240+x]==0x1234);
  assert(clip.pixels[184*240]==0xabcd&&clip.pixels[191*240+9]==0xabcd);
 }
 unsigned battleFrames=0;menu=MenuState{};
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race)for(unsigned shirt=0;shirt<8;++shirt)for(unsigned pants=0;pants<8;++pants){
  g=rpg::create(cls,77);g.race=race;g.shirt=shirt;g.trousers=pants;g.city=1;rpg::begin(g,0);
  v=ViewState{};v.page=Page::Battle;v.effect=cls==0?Effect::MagicDarts:cls==1?Effect::DivineSlash:cls==2?Effect::Thrust:Effect::Rage;
  v.combatResult={CombatFeedback::Damage,7,false,false};v.effectProgress=600;v.effectElapsedMs=660;v.effectFrame=4;v.heroFrame=4;
  immutableRender(c,g,v,0);++battleFrames;
  if(shirt==5&&pants==7)c.save(root+"/battle-class"+std::to_string(cls)+"-race"+std::to_string(race)+".ppm");
 }
 // A real resolved missile action supplies the number; rendering never recomputes damage.
 g=rpg::create(0,42);g.race=0;g.shirt=5;g.trousers=7;g.city=1;rpg::begin(g,0);assert(!rpg::act(g,rpg::Action::MagicMissile));
 v=ViewState{};v.page=Page::Battle;v.effect=Effect::MagicDarts;v.combatResult=heroCombatResult(g,rpg::Action::MagicMissile);
 Canvas fixedHud;bool first=true;
 for(unsigned ms:{0u,120u,180u,360u,540u,600u,720u,900u,1099u}){
  v.effectElapsedMs=ms;v.effectProgress=ms*1000/1100;v.effectFrame=ms*8/1100;
  immutableRender(c,g,v,0);++battleFrames;c.save(root+"/mage-pilot-"+std::to_string(ms)+".ppm");
  if(first){fixedHud=c;first=false;}else for(int y=0;y<320;++y)if(y<86||y>=192)for(int x=0;x<240;++x)assert(c.pixels[y*240+x]==fixedHud.pixels[y*240+x]);
 }
 for(auto result:{CombatResult{CombatFeedback::Damage,19,true,true},CombatResult{CombatFeedback::Miss,0,false,true},CombatResult{CombatFeedback::NoDamage,0,false,true},CombatResult{CombatFeedback::Heal,9,false,false},CombatResult{CombatFeedback::Guard,0,false,false},CombatResult{CombatFeedback::Buff,0,false,false}}){
  v.combatResult=result;v.effectOnHero=true;v.effect=result.attack()?Effect::Slash:result.kind==CombatFeedback::Buff?Effect::Rage:Effect::Shield;v.effectProgress=600;v.effectFrame=4;
  immutableRender(c,g,v);++battleFrames;c.save(root+"/feedback-"+std::to_string(unsigned(result.kind))+".ppm");
  assert((combatRecoil(result,600)>0)==result.injury());
 }
 // Shared feedback in the existing dungeon, with no weapon-range or game changes.
 g=rpg::testHero(0,77);g.city=1;g.crystals=1;assert(!rpg::enterDungeon(g));v=ViewState{};v.page=Page::Dungeon;v.effect=Effect::MagicDarts;v.combatResult={CombatFeedback::Damage,8,false,false};
 for(unsigned ms:{0u,180u,540u,900u}){v.effectElapsedMs=ms;v.effectFrame=ms*8/1100;v.effectProgress=ms*1000/1100;immutableRender(c,g,v);++battleFrames;c.save(root+"/dungeon-pilot-"+std::to_string(ms)+".ppm");}
 // Player-target and enemy-target dungeon outcomes have distinct visual anchors.
 for(auto kind:{CombatFeedback::Heal,CombatFeedback::Damage})for(bool targetHero:{false,true}){
  v.combatResult={kind,9,false,kind==CombatFeedback::Heal?!targetHero:targetHero};v.effectOnHero=targetHero;v.effect=kind==CombatFeedback::Heal?Effect::Radiant:Effect::Slash;v.effectProgress=600;v.effectFrame=4;
  immutableRender(c,g,v);++battleFrames;bool found=false;
  for(const auto& label:c.positions)if(label.text==(kind==CombatFeedback::Heal?"+9 HP":"-9")){assert(targetHero?label.y>120:label.y<90);found=true;}
  assert(found);c.save(root+"/dungeon-target-"+std::to_string(unsigned(kind))+"-"+std::to_string(targetHero)+".ppm");
 }
 printf("PASS: visual77 %u battle frames; all 4 races x 4 classes x 8 shirt x 8 pants, exact missile phases, local clipping, static HUD, semantic feedback and dungeon.\n",battleFrames);
 assert(rpg::crc(artMemory,ART_BYTES)==ART_CRC);free(artMemory);artMemory=nullptr;
 printf("PASS: visual77 %u travel/OTA frames; complete dynamic text, absent art buffer, fixed controls, Save28/runtime unchanged.\n",frames);
}
