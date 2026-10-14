#include "../firmware/RPG_POKET_2/Persistence.h"
#include "../firmware/RPG_POKET_2/View.h"
#include "../firmware/RPG_POKET_2/src/GFX/font/glcdfont.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
using namespace rpg;

// Exercise the production renderer and the production Save30 codec, not the
// frozen legacy journal used by the older event suites.
struct EventCanvas {
 struct Label {int x,y,size;uint16_t color;std::string text;};
 uint16_t pixels[240*320]{};int tx=0,ty=0,size=1;uint16_t color=UI_WHITE;
 std::vector<Label> labels;
 void pixel(int x,int y,uint16_t c){if(x>=0&&x<240&&y>=0&&y<320)pixels[y*240+x]=c;}
 void fillRect(int x,int y,int w,int h,uint16_t c){for(int j=0;j<h;++j)for(int i=0;i<w;++i)pixel(x+i,y+j,c);}
 void drawRect(int x,int y,int w,int h,uint16_t c){fillRect(x,y,w,1,c);fillRect(x,y+h-1,w,1,c);fillRect(x,y,1,h,c);fillRect(x+w-1,y,1,h,c);}
 void fillScreen(uint16_t c){fillRect(0,0,240,320,c);}
 void setTextWrap(bool){}
 void setTextColor(uint16_t c){color=c;}
 void setTextSize(int s){size=s;}
 void setCursor(int x,int y){tx=x;ty=y;}
 void print(const char* s){
  assert(tx>=0&&ty>=0&&tx+int(strlen(s))*6*size<=240&&ty+8*size<=320);
  labels.push_back({tx,ty,size,color,s});
  while(*s){unsigned ch=static_cast<unsigned char>(*s++);assert(ch<128);for(int i=0;i<5;++i)for(int j=0;j<8;++j)if(font[ch*5+i]&(1<<j))fillRect(tx+i*size,ty+j*size,size,size,color);tx+=6*size;}
 }
 void draw16bitRGBBitmap(int x,int y,uint16_t* b,int w,int h){for(int j=0;j<h;++j)for(int i=0;i<w;++i)pixel(x+i,y+j,b[j*w+i]);}
 bool has(const char* s)const{for(const auto& label:labels)if(label.text==s)return true;return false;}
 bool contains(const char* s)const{for(const auto& label:labels)if(label.text.find(s)!=std::string::npos)return true;return false;}
 void visible()const{for(const auto& l:labels)for(unsigned n=0;n<l.text.size();++n)for(int i=0;i<5;++i)for(int j=0;j<8;++j)if(font[uint8_t(l.text[n])*5+i]&(1<<j))for(int yy=0;yy<l.size;++yy)for(int xx=0;xx<l.size;++xx)assert(pixels[(l.y+j*l.size+yy)*240+l.x+(n*6+i)*l.size+xx]==l.color);}
 void save(const std::string& path)const{std::ofstream f(path,std::ios::binary);assert(f);f<<"P6\n240 320\n255\n";for(auto p:pixels){char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};f.write(rgb,3);}}
};
unsigned frames=0,checkpoints=0;
std::vector<uint8_t> packed(const Game& g){std::vector<uint8_t> b(PERSISTENT_SAVE_SIZE);encodePersistent(g,17,1,b.data());return b;}
Game checkpoint(const Game& g){assert(persistentValid(g));auto b=packed(g);Game h;uint32_t seq=0;assert(get16(b.data(),4)==30&&get32(b.data(),144)==1);assert(decodePersistent(b.data(),1,h,seq)==Decode::Ok&&seq==17&&packed(h)==b);++checkpoints;return h;}
Game offer(unsigned cls,unsigned level,unsigned kind,bool night,unsigned story=0){
 for(unsigned seed=1;seed<2048;++seed){auto g=create(cls,seed);g.p.xp=dndXp[level-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=g.guildMember=true;g.city=0;
  if(story){g.dungeonClears=1;g.campaignFlags=story==1?7:story>=4?255:135;g.campaignEnding=story==3||story==4?1:story==5?2:0;}
  assert(offerEvent(g,20733,night?21:10));if(g.eventKind==kind)return checkpoint(g);
 }assert(false);return Game{};
}
void immutableRender(EventCanvas& c,const Game& g,Page page){
 const auto before=packed(g);const auto copy=g;ViewState v;v.page=page;menu.notice="";menu.renderNow=1000;menu.letterStarted=0;c.labels.clear();render(c,g,v);assert(packed(g)==before&&!memcmp(&g,&copy,sizeof(g)));c.visible();
 if(page==Page::Letter)for(const auto& l:c.labels)if(l.y>=97&&l.y<=206)assert(l.x>=11&&l.x+int(l.text.size())*6*l.size<=229&&l.y+8*l.size<=217);
 ++frames;
}
void win(Game& g){assert(g.phase==Phase::Hero);g.enemyHp=0;finish(g);g=checkpoint(g);assert(resumeEventBattle(g));g=checkpoint(g);}
void takeSnapshot(const EventCanvas& c,const std::string& root,const std::string& name){if(!root.empty())c.save(root+"/"+name+".ppm");}
int main(int argc,char** argv){
 {auto letter=offer(0,1,7,false);assert(persistentValid(letter)&&!strcmp(eventBrief(letter,1),"Ha ameacas rondando a fogueira."));}

 assert(argc<=2);const std::string out=argc==2?argv[1]:"";artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);makeFallback();
 // Screenshots use the exact installed art pack when requested. Pure layout
 // checks can also run without it, using the existing device fallback.
 std::ifstream art("cartao/RPGPOKET/artes.pak",std::ios::binary);
 const bool hasArt=bool(art);if(hasArt){art.seekg(16);art.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(art.gcount()==ART_BYTES&&crc(artMemory,ART_BYTES)==ART_CRC);}
 assert(out.empty()||hasArt);
 EventCanvas c;
 const unsigned levels[]={1,5,10,18},investigationEnemies[4][2]={{1,1},{2,2},{6,25},{7,27}};
 // N03: all eight actual final investigation encounters, unchanged seal order,
 // wrong-order fight and no farming. Text queries and every render are pure.
 for(unsigned tier=0;tier<4;++tier)for(bool night:{false,true}){
  auto g=offer(0,levels[tier],6,night);immutableRender(c,g,Page::Letter);assert(c.has("Criaturas rondam entre as pedras."));assert(!c.contains("eco corrompeu"));
  assert(!acceptEvent(g,49));g=checkpoint(g);immutableRender(c,g,Page::EventMission);assert(c.has("Erro traz combate; pode tentar."));immutableRender(c,g,Page::EventChoice);
  for(unsigned i=0;i<3;++i){auto before=packed(g);assert(strlen(eventSealClue(tier,i))<36&&strlen(eventOrder(i))<36);assert(packed(g)==before);assert(!eventAct(g,i));g=checkpoint(g);immutableRender(c,g,Page::EventChoice);}
  assert(g.eventProgress==1);unsigned wrong=(tier+1)%3;assert(!eventAct(g,wrong));assert(g.enemyId==eventFoe(g));win(g);auto same=packed(g);assert(eventAct(g,wrong)&&packed(g)==same);
  assert(!eventAct(g,tier%3)&&g.eventProgress==2);g=checkpoint(g);immutableRender(c,g,Page::EventMission);assert(c.has("Abrir passagem")&&!c.contains("Enfrentar o eco"));takeSnapshot(c,out,"investigation-"+std::to_string(tier)+"-"+std::to_string(night));
  assert(!eventAct(g)&&g.enemyId==investigationEnemies[tier][unsigned(night)]);win(g);assert(eventReady(g));immutableRender(c,g,Page::EventResult);assert(c.has("Os nomes deixaram de se repetir."));assert(finishEvent(g));g=checkpoint(g);assert(!finishEvent(g));
 }
 // N03: expedition spawns remain physical creatures, with the bell and voices
 // separate in the offered and active text. Both day/night and low-level
 // goblin/wolf variants are covered without changing the encounter table.
 for(unsigned level:{1u,3u,5u,10u,18u})for(bool night:{false,true}){
  auto g=offer(0,level,3,night);immutableRender(c,g,Page::Letter);assert(c.has("Ele desperta vozes entre as pedras.")&&c.has("Criaturas bloqueiam a passagem.")&&c.has("Encontre a corda e cale o sino."));takeSnapshot(c,out,"bell-letter-"+std::to_string(level)+"-"+std::to_string(night));
  assert(!acceptEvent(g,49));g=checkpoint(g);immutableRender(c,g,Page::EventMission);assert(c.has("Criaturas bloqueiam a passagem.")&&c.has("Encontre a corda para cala-lo."));assert(!eventAct(g));g=checkpoint(g);
  const unsigned ids[4][3]={{level<3?0u:1u,level<3?0u:1u,0},{2,2,5},{25,25,26},{27,27,28}};
  assert(dungeonSpawnCount(g)==(g.eventTier?3u:2u));for(unsigned i=0;i<dungeonSpawnCount(g);++i)assert(dungeonSpawn(g,i).id==ids[g.eventTier][i]);
  assert(!strcmp(eventCompletion(g),"O sino calou. As vozes sossegaram."));
 }
 // N04: pre-deposition, deposed before first Heart, temporary ending missing
 // help, temporary-ready and full ending. New offers and accepted/restored
 // letters remain available; active text never reinstates pursuit by Odran.
 for(unsigned story=1;story<=5;++story)for(unsigned kind:{1u,2u})for(bool night:{false,true}){
  auto g=offer(0,18,kind,night,story);const auto reward=eventGold(g),xp=eventXp(g);immutableRender(c,g,Page::Letter);
  if(story==1)assert(c.contains(kind==1?"para salva-los de uma patrulha.":"A guarda segue seus"));
  else {assert(!c.contains("A guarda segue seus")&&!c.contains("passos. Nao o deixe sozinho."));assert(c.has(kind==1?"Eles podem provar abusos passados.":"A mensagem precisa chegar."));}
  takeSnapshot(c,out,"aurora-letter-"+std::to_string(kind)+"-story"+std::to_string(story)+"-"+std::to_string(night));
  assert(!acceptEvent(g,49));g=checkpoint(g);assert(eventGold(g)==reward&&eventXp(g)==xp);immutableRender(c,g,Page::EventMission);assert(c.has(kind==1?"Procurar carga":"Iniciar escolta")&&!c.contains("guarda"));takeSnapshot(c,out,"aurora-active-"+std::to_string(kind)+"-story"+std::to_string(story)+"-"+std::to_string(night));
  // Closing/reopening and reboot do not alter an accepted mission or its price.
  auto active=packed(g);immutableRender(c,g,Page::EventAbandon);immutableRender(c,g,Page::Letters);g=checkpoint(g);immutableRender(c,g,Page::EventMission);assert(packed(g)==active);
 }
 // N08/N09: same Grum binding, first person, an optional single ration and
 // exactly the existing roll/bonus/guard. Refused resource use is byte-pure.
 for(unsigned cls=0;cls<4;++cls)for(unsigned level:levels)for(bool night:{false,true})for(unsigned choice=0;choice<2;++choice){
  auto g=offer(cls,level,7,night);assert(eventSpeaker(g)==story::Npc::Grum&&!strcmp(eventSender(g),"Grum"));immutableRender(c,g,Page::Letter);assert(c.has("Uns viajantes me pediram abrigo."));assert(!acceptEvent(g,49));g.rations=0;
  auto before=packed(g);assert(!strcmp(eventAct(g,1),"Leve uma racao para a vigilia")&&packed(g)==before);g.rations=3;g=checkpoint(g);immutableRender(c,g,Page::EventChoice);assert(c.has("Uma racao sustenta quem vigia.")&&c.has("Reforcar vigilia / 1 racao")&&c.has("Erguer barricadas / gratis")&&!c.contains("tochas"));
  if(cls==0&&level==1&&!night&&!choice)takeSnapshot(c,out,"grum-vigil-choice");
  Game expected=g;const unsigned roll=1+random(expected)%20;assert(!eventAct(g,choice));assert(g.rations==3-choice&&g.eventProgress==1&&g.eventRoll==roll&&g.randomState==expected.randomState&&(g.eventFlags&15)==choice+1);g=checkpoint(g);
  for(unsigned wave=1;wave<=2;++wave){const unsigned bonus=choice?luck(g):survival(g);assert(!eventAct(g));assert(g.guard==(roll==20||(roll!=1&&roll+bonus>=12u+g.eventTier)?75:50));win(g);}assert(eventReady(g));assert(finishEvent(g)&&g.rations==4-choice);g=checkpoint(g);
 }
 // N11: the initial Carvalho contract still counts goblins and wolves, pays
 // exactly once and keeps its original generic target, goal and rewards.
 auto g=create(0,17);g.tutorial=g.guildMember=true;assert(!strcmp(contract(4).objective,"Venca 3 ameacas em Carvalho"));assert(contract(4).enemy==-1&&contract(4).count==3&&contract(4).gold==18&&contract(4).percent==15);assert(!acceptQuest(g,4));
 for(unsigned id:{0u,1u,0u}){assert(begin(g,id));g.enemyHp=0;finish(g);assert(home(g));g=checkpoint(g);}assert(questComplete(g));const unsigned gold=g.p.gold,xp=totalExperience(g),pay=contractGold(g,g.questId,g.questLevel),payXp=contractXp(g,g.questId,g.questLevel);assert(!claimQuest(g));assert(g.p.gold==gold+pay&&totalExperience(g)==xp+payXp);g=checkpoint(g);const auto paid=packed(g);assert(claimQuest(g)&&packed(g)==paid);
 free(artMemory);artMemory=nullptr;printf("PASS narrative8_events: %u real-render frames, %u Save30/epoch1 checkpoints; 8 investigation fights, 10 bell encounter variants, Aurora offer/active states, 64 optional vigil choices, generic three-threat contract; unchanged render/query bytes.\n",frames,checkpoints);
}
