// Actual firmware renderer, real tracked authored art, physical portrait storage.
#include "../firmware/RPG_POKET_2/Coop/CoopView.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>
#include <new>
#include <filesystem>
using namespace coopview;
static bool rendering=false;static unsigned allocationCalls=0,renders=0;
extern "C" void* __real_malloc(size_t);extern "C" void* __real_calloc(size_t,size_t);extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){if(rendering)++allocationCalls;return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t s){if(rendering)++allocationCalls;return __real_calloc(n,s);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(rendering)++allocationCalls;return __real_realloc(p,n);}
void* operator new(std::size_t size){if(rendering)++allocationCalls;if(void* p=std::malloc(size))return p;throw std::bad_alloc();}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
struct Portrait {
 std::array<uint16_t,240*320> pixels{};unsigned writes=0;
 void drawPixel(int x,int y,uint16_t color){assert(x>=0&&x<240&&y>=0&&y<320);pixels[y*240+x]=color;++writes;}
 void fillRect(int x,int y,int w,int h,uint16_t color){assert(x>=0&&y>=0&&w>0&&h>0&&x+w<=240&&y+h<=320);for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx)drawPixel(xx,yy,color);}
 uint16_t logical(int x,int y)const{auto p=coopplatform::toPortrait(x,y);assert(p.x>=0);return pixels[p.y*240+p.x];}
 void save(const std::string& path,bool landscape)const{
  std::ofstream f(path,std::ios::binary);assert(f);int w=landscape?320:240,h=landscape?240:320;f<<"P6\n"<<w<<" "<<h<<"\n255\n";
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){uint16_t p=landscape?logical(x,y):pixels[y*240+x];char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};f.write(rgb,3);}
  assert(f);
 }
};
struct Probe:coopplatform::LandscapeCanvas<Portrait>{
 using Base=coopplatform::LandscapeCanvas<Portrait>;explicit Probe(Portrait& c):Base(c){}
 struct Label{int x,y;char value[80];};std::array<Label,128> labels{};unsigned count=0;
 void print(const char* value){assert(count<labels.size());auto& l=labels[count++];l.x=getCursorX();l.y=getCursorY();std::snprintf(l.value,sizeof(l.value),"%s",value);
  if(!(l.x>=0&&l.y>=0&&l.x+int(std::strlen(value))*6<=320&&l.y+8<=240)){std::fprintf(stderr,"Text overflow %d %d %s\n",l.x,l.y,value);std::abort();}Base::print(value);
 }
 bool has(const char* s)const{for(unsigned i=0;i<count;++i)if(!std::strcmp(labels[i].value,s))return true;return false;}
 void clear(){count=0;}
 void noTextOverlaps()const{for(unsigned i=0;i<count;++i)for(unsigned j=i+1;j<count;++j){auto a=labels[i],b=labels[j];Rect ra{int16_t(a.x),int16_t(a.y),int16_t(std::strlen(a.value)*6),8},rb{int16_t(b.x),int16_t(b.y),int16_t(std::strlen(b.value)*6),8};if(ra.w&&rb.w&&coopui::intersects(ra,rb)){std::fprintf(stderr,"Labels overlap: %s / %s\n",a.value,b.value);std::abort();}}}
};
static Portrait physical;static Probe canvas(physical);static std::array<uint16_t,240*320> previous;
static uint64_t pixelsHash(){uint64_t h=1469598103934665603ull;for(uint16_t p:physical.pixels){h^=p;h*=1099511628211ull;}return h;}
static rpg::Game profile(unsigned cls=0,unsigned race=0){auto g=rpg::create(cls,541+cls);while(g.p.level<5){g.p.xp=rpg::xpNeeded(g);rpg::levelUp(g);}g.race=race;g.shirt=cls+1;g.trousers=7-cls;g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.p.gold=789;assert(rpg::persistentValid(g));return g;}
static coopplatform::Training training(const rpg::Game& a,const rpg::Game& b){
 coopplatform::Training t;coopplatform::Binding binding{{{2,1,2,3,4,5}},{{2,1,2,3,4,6}},0x0123456789abcdefull,0};assert(t.start(binding,a,b,789,0)==coopplatform::StartError::None);t.endpoint.paused=false;t.endpoint.awaitingAck=false;t.ui.sync(t.endpoint);return t;
}
static void renderChecked(const ViewModel& v,const coopplatform::Training& t,const rpg::Game& a,const rpg::Game& b,bool overlap=true){
 auto before=t;auto modelBefore=v;auto gameA=a,gameB=b;uint8_t sa[rpg::PERSISTENT_SAVE_SIZE],sb[rpg::PERSISTENT_SAVE_SIZE],after[rpg::PERSISTENT_SAVE_SIZE];rpg::encodePersistent(a,42,0,sa);rpg::encodePersistent(b,99,1,sb);
 canvas.clear();rendering=true;draw(canvas,v,t);rendering=false;++renders;
 assert(!allocationCalls&&!std::memcmp(&before,&t,sizeof(t))&&!std::memcmp(&v,&modelBefore,sizeof(v))&&!std::memcmp(&a,&gameA,sizeof(a))&&!std::memcmp(&b,&gameB,sizeof(b)));
 rpg::encodePersistent(a,42,0,after);assert(!std::memcmp(sa,after,sizeof(sa))&&rpg::get16(after,4)==30);rpg::encodePersistent(b,99,1,after);assert(!std::memcmp(sb,after,sizeof(sb)));
 assert(canvas.has("SAIR")&&canvas.has("SEM XP/OURO | SEM GASTOS NA CAMPANHA"));if(overlap)canvas.noTextOverlaps();
 if(v.screen==Screen::Playing)for(unsigned i=0;i<canvas.count;++i){const auto& l=canvas.labels[i];if(l.y>=kStats[0].y&&l.y<kStats[0].y+kStats[0].h){const Rect r=kStats[l.x>=kStats[1].x];assert(l.x>=r.x+3&&l.x+int(std::strlen(l.value))*6<=r.x+r.w-3&&l.y+8<=r.y+r.h-2);}
  if(l.y>=199&&l.y<229){bool bounded=false;for(auto r:coopui::kButtons)if(l.x>=r.x+3&&l.x+int(std::strlen(l.value))*6<=r.x+r.w-3&&l.y>=r.y+3&&l.y+8<=r.y+r.h-3)bounded=true;assert(bounded);}}

}
static void capture(const std::string& out,const char* name,const ViewModel& v,const coopplatform::Training& t,const rpg::Game& a,const rpg::Game& b){
 renderChecked(v,t,a,b);previous=physical.pixels;uint64_t hash=pixelsHash();renderChecked(v,t,a,b);assert(previous==physical.pixels&&hash==pixelsHash());physical.save(out+"/"+name+".ppm",true);
 std::ofstream labels(out+"/"+name+"-labels.tsv");for(unsigned i=0;i<canvas.count;++i)labels<<canvas.labels[i].x<<'\t'<<canvas.labels[i].y<<'\t'<<canvas.labels[i].value<<'\n';
}
struct SessionRig {
 struct Frame {coopplatform::Mac from{},to{};uint16_t size=0;uint8_t bytes[coop::kMaxPacket]{};};
 static constexpr coopplatform::Mac addresses[2]={{{2,0,0,0,0,1}},{{2,0,0,0,0,2}}};
 struct Port {SessionRig* rig=nullptr;unsigned side=0;};
 coopplatform::Session sessions[2];Port ports[2]{{this,0},{this,1}};coop::Queue<Frame,64> queue{};uint32_t now=100;
 static bool send(void* context,const coopplatform::Mac& to,const uint8_t* bytes,size_t size){auto& p=*static_cast<Port*>(context);Frame f{};f.from=addresses[p.side];f.to=to;assert(size<=sizeof(f.bytes));f.size=size;std::memcpy(f.bytes,bytes,size);assert(p.rig->queue.push(f));return true;}
 void flush(){Frame f{};unsigned n=0;while(queue.pop(f)){assert(++n<200);for(unsigned i=0;i<2;++i)if(!coopplatform::sameMac(f.from,addresses[i])&&(f.to.bytes[0]==255||coopplatform::sameMac(f.to,addresses[i])))sessions[i].receive(f.from,f.bytes,f.size,now);}}
 void open(const rpg::Game& a,const rpg::Game& b){for(unsigned i=0;i<2;++i){sessions[i].setSender(send,&ports[i]);assert(sessions[i].open(i?b:a,addresses[i],45+i,now));}flush();}
};
static void sessionCapture(const std::string& out,const char* name,const coopplatform::Session& session,const rpg::Game& a,const rpg::Game& b){
 const auto before=session;rendering=true;auto v=model(session,true);rendering=false;assert(!std::memcmp(&before,&session,sizeof(session))&&!allocationCalls);
 capture(out,name,v,session.training(),a,b);assert(!std::memcmp(&before,&session,sizeof(session)));
}
static void sessionFlow(const std::string& out,const rpg::Game& a,const rpg::Game& b){
 SessionRig rig;assert(model(rig.sessions[0],true).screen==Screen::Unavailable);rig.open(a,b);
 auto v=model(rig.sessions[0],true);assert(v.screen==Screen::Lobby&&v.hasLocal&&v.allowHost&&v.allowJoin&&!v.hasPeer&&!v.allowAccept);sessionCapture(out,"session-browsing",rig.sessions[0],a,b);
 assert(rig.sessions[0].host(rig.now));rig.flush();assert(model(rig.sessions[0],true).screen==Screen::Hosting);
 auto guest=model(rig.sessions[1],true);assert(guest.peers[0].present&&guest.peers[0].available&&guest.peers[0].tag==uint16_t(rig.sessions[0].sessionToken()));assert(hitTest(guest,kJoin[0].x+2,kJoin[0].y+2)==Control::Join0);
 sessionCapture(out,"session-discovered-host",rig.sessions[1],a,b);assert(rig.sessions[1].join(0,rig.now));rig.flush();assert(model(rig.sessions[1],true).screen==Screen::Connecting);
 v=model(rig.sessions[0],true);assert(v.screen==Screen::ConfirmPeer&&v.hasPeer&&v.allowAccept&&v.peerProfile.cls==b.p.cls);sessionCapture(out,"session-host-accept",rig.sessions[0],a,b);
 assert(rig.sessions[0].accept(rig.now));rig.flush();assert(model(rig.sessions[0],true).screen==Screen::Playing&&model(rig.sessions[1],true).screen==Screen::Playing);
 sessionCapture(out,"session-playing-host",rig.sessions[0],a,b);sessionCapture(out,"session-playing-guest",rig.sessions[1],a,b);
 assert(model(rig.sessions[1],true).local==1);rig.sessions[1].leave(rig.now);rig.flush();assert(model(rig.sessions[0],true).end==EndReason::PeerLeft&&model(rig.sessions[1],true).end==EndReason::Left);
 sessionCapture(out,"session-peer-left",rig.sessions[0],a,b);sessionCapture(out,"session-left",rig.sessions[1],a,b);
}
static void authoredAssets(){
 // Every displayed hero pixel must equal the production personalPixel path.
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race){
  auto g=profile(cls,race);coop::Actor a{};assert(coop::playerProfile(g,0,a));const auto crop=kHeroCrop[race][cls];
  int left=112,top=120,right=0,bottom=0;
  for(unsigned pose=0;pose<6;++pose)for(unsigned y=0;y<120;++y)for(unsigned x=0;x<112;++x)if(personalPixel(g,pose,y*112+x)!=indexed4::transparent){left=std::min(left,int(x));top=std::min(top,int(y));right=std::max(right,int(x+1));bottom=std::max(bottom,int(y+1));}
  assert(crop.x==left&&crop.y==top&&crop.w==right-left&&crop.h==bottom-top);
  Rect box{18,54,58,64};const Rect d=fit(crop,box);canvas.fillScreen(Ink);hero(canvas,a,box,true);
  for(int y=0;y<d.h;++y)for(int x=0;x<d.w;++x){unsigned sx=crop.x+x*crop.w/d.w,sy=crop.y+y*crop.h/d.h;auto expected=personalPixel(g,0,sy*112+sx);assert(physical.logical(d.x+x,d.y+y)==(expected==indexed4::transparent?Ink:expected));}
 }
 static uint16_t workspace[enemyArt::WORKSPACE_PIXELS];auto* old=enemyArt::workspace;enemyArt::workspace=workspace;
 for(uint8_t species=0;species<5;++species){coop::Actor a{};a.kind=coop::Kind::Enemy;a.species=species;Rect box{192,51,48,40};const auto& p=enemyScale::profile(species);Crop crop{p.left,p.top,int16_t(p.right-p.left),int16_t(p.bottom-p.top)};Rect d=fit(crop,box);
  auto decoded=enemyArt::decode(enemyArt::body(species,enemyArt::Pose::Idle));assert(decoded.pixels);canvas.fillScreen(Ink);enemy(canvas,a,box,true);
  for(int y=0;y<d.h;++y)for(int x=0;x<d.w;++x){unsigned sx=crop.x+x*crop.w/d.w,sy=crop.y+y*crop.h/d.h;auto expected=decoded.pixels[sy*decoded.w+sx];assert(physical.logical(d.x+x,d.y+y)==(expected==indexed4::transparent?Ink:expected));}
 }
 enemyArt::workspace=old;
}
static void geometry(){
 for(auto b:coopui::kButtons)assert(!coopui::intersects(b,kExit));
 for(auto b:coopui::kEnemyHits)assert(!coopui::intersects(b,kExit));
 assert(!coopui::intersects(coopui::kBossHit,kExit));
 ViewModel v;v.hasLocal=true;for(auto& p:v.peers)p.present=true;
 unsigned counts[10]={};for(int y=0;y<240;++y)for(int x=0;x<320;++x){auto p=coopplatform::toPortrait(x,y);auto q=coopplatform::toLandscape(p.x,p.y);assert(q.x==x&&q.y==y);++counts[uint8_t(hitTest(v,x,y))];}
 assert(counts[uint8_t(Control::Exit)]==kExit.w*kExit.h&&counts[uint8_t(Control::Host)]==kHost.w*kHost.h);for(unsigned i=0;i<4;++i)assert(counts[uint8_t(Control::Join0)+i]==unsigned(kJoin[i].w*kJoin[i].h));
 for(auto screen:{Screen::Hosting,Screen::Connecting,Screen::Playing,Screen::Ended,Screen::Unavailable}){v.screen=screen;for(int y=0;y<240;++y)for(int x=0;x<320;++x)assert(hitTest(v,x,y)==(kExit.contains(x,y)?Control::Exit:Control::None));}
 v.screen=Screen::ConfirmPeer;v.hasPeer=true;v.allowAccept=true;assert(hitTest(v,30,190)==Control::Accept&&hitTest(v,180,190)==Control::Decline);v.allowAccept=false;assert(hitTest(v,30,190)==Control::None);v.screen=Screen::Lobby;v.allowHost=v.allowJoin=false;assert(hitTest(v,30,190)==Control::None&&hitTest(v,180,60)==Control::None);
 // Wide geometry clips without overflow; no invalid writes reach physical storage.
 canvas.fillScreen(0);canvas.fillRect(INT32_MIN,INT32_MIN,INT32_MAX,INT32_MAX,1);canvas.fillRect(-2,-2,4,4,2);assert(physical.logical(0,0)==2&&physical.logical(1,1)==2&&physical.logical(2,2)==0);
 canvas.drawRect(INT32_MAX,INT32_MAX,INT32_MAX,INT32_MAX,3);canvas.fillRect(-10,-10,INT32_MAX,INT32_MAX,4);for(auto p:physical.pixels)assert(p==4);
 // Exact classic font rendered to logical x/y, then rotated into native storage.
 canvas.fillScreen(0);canvas.setCursor(10,20);canvas.setTextColor(0xffff);canvas.setTextSize(1);canvas.Base::print("A");for(int y=0;y<8;++y)for(int x=0;x<5;++x)assert(physical.logical(10+x,20+y)==((font['A'*5+x]&(1<<y))?0xffff:0));
}
int main(int argc,char** argv){
 assert(argc==3);std::filesystem::create_directories(argv[2]);std::string out=argv[2];artMemory=static_cast<uint8_t*>(std::malloc(ART_BYTES));assert(artMemory);
 std::ifstream f(argv[1],std::ios::binary);assert(f);uint8_t header[16];f.read(reinterpret_cast<char*>(header),16);assert(!std::memcmp(header,"PKA1",4)&&rpg::get32(header,4)==ART_BYTES&&rpg::get32(header,8)==ART_CRC&&rpg::get32(header,12)==ART_COUNT);f.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(f.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);geometry();authoredAssets();
 auto a=profile(0),b=profile(2);sessionFlow(out,a,b);auto t=training(a,b);ViewModel v;v.hasLocal=v.hasPeer=true;v.localProfile=t.endpoint.state.actors[0];v.peerProfile=t.endpoint.state.actors[1];
 capture(out,"lobby-search",v,t,a,b);assert(canvas.has("BUSCANDO SALAS")&&canvas.has("CRIAR SALA"));
 for(unsigned i=0;i<4;++i){v.peers[i].present=true;v.peers[i].tag=uint16_t(0xab10+i);assert(coop::playerProfile(profile(i),0,v.peers[i].profile));}capture(out,"lobby-four-hosts",v,t,a,b);assert(canvas.has("ENTRAR  #AB13"));v.peers[0].available=false;capture(out,"lobby-busy-host",v,t,a,b);assert(canvas.has("OCUPADA #AB10")&&hitTest(v,180,60)==Control::None);v.peers[0].available=true;
 for(auto note:{LobbyNotice::Declined,LobbyNotice::Busy,LobbyNotice::PeerExpired,LobbyNotice::SendFailed}){v.notice=note;char name[40];std::snprintf(name,sizeof(name),"lobby-notice-%u",unsigned(note));capture(out,name,v,t,a,b);}v.notice=LobbyNotice::None;
 v.screen=Screen::Hosting;v.roomCode=0xab12;capture(out,"host-wait",v,t,a,b);assert(canvas.has("SALA #AB12"));
 v.screen=Screen::ConfirmPeer;v.allowAccept=true;capture(out,"host-accept",v,t,a,b);assert(canvas.has("ACEITAR")&&canvas.has("RECUSAR"));
 v.screen=Screen::Connecting;capture(out,"join-pending",v,t,a,b);assert(canvas.has("SEM ENTRADA"));
 v.screen=Screen::Playing;capture(out,"playing",v,t,a,b);assert(canvas.has("J1 MAGO N5 *")&&canvas.has("J2 GUERREIRO N5"));
 t.ui.mode=coopui::Mode::Target;t.ui.selected=coop::Action::Attack;capture(out,"attack-targets",v,t,a,b);assert(canvas.has("ATAQUE: TOQUE NO INIMIGO"));
 for(uint8_t slot=3;slot<6;++slot){assert(t.ui.eligible(t.endpoint,slot));Rect hit=t.ui.targetRect(t.endpoint,slot);assert(physical.logical(hit.x,hit.y)==Target&&physical.logical(hit.x+hit.w-1,hit.y+hit.h-1)==Target);}
 physical.save(out+"/attack-targets-native-portrait.ppm",false);
 // Dead actors are not highlighted or hittable.
 t.endpoint.state.actors[4].hp=0;capture(out,"one-target-down",v,t,a,b);assert(!t.ui.eligible(t.endpoint,4));Rect dead=coopui::kEnemyHits[1];assert(physical.logical(dead.x,dead.y)!=Target);
 t=training(a,b);t.ui.mode=coopui::Mode::Ability;capture(out,"ability-focus-test",v,t,a,b);assert(canvas.has("FOCO TESTE")&&canvas.has("FOCO TESTE / HABILIDADE EXPERIMENTAL"));
 t.ui.mode=coopui::Mode::Target;t.ui.selected=coop::Action::Focus;capture(out,"focus-targets",v,t,a,b);
 t.ui.mode=coopui::Mode::Items;t.endpoint.state.actors[0].hp-=5;t.endpoint.state.actors[0].mp-=2;capture(out,"training-items",v,t,a,b);assert(canvas.has("POCOES COPIADAS SO PARA O TREINO"));
 t.ui.mode=coopui::Mode::Pending;t.endpoint.pending=true;capture(out,"pending-no-input",v,t,a,b);assert(canvas.has("AGUARDANDO CONFIRMACAO / SEM ENTRADA"));
 t.endpoint.paused=true;capture(out,"reconnect-no-input",v,t,a,b);assert(canvas.has("RECONECTANDO / SEM ENTRADA"));
 t=training(a,b);t.endpoint.state.turn=1;capture(out,"other-player-turn",v,t,a,b);assert(canvas.has("AGUARDE A SUA VEZ / SEM ENTRADA"));
 t.endpoint.state.turn=3;capture(out,"enemy-turn",v,t,a,b);assert(canvas.has("VEZ: GOB  /  RODADA 1"));
 t=training(a,b);t.endpoint.state.wave=1;coop::spawnWave(t.endpoint.state);capture(out,"wave-two",v,t,a,b);
 t.endpoint.state.wave=2;coop::spawnWave(t.endpoint.state);t.ui.mode=coopui::Mode::Target;capture(out,"boss-target",v,t,a,b);assert(physical.logical(coopui::kBossHit.x,coopui::kBossHit.y)==Target);
 t.endpoint.hasState=false;capture(out,"sync-no-input",v,t,a,b);assert(canvas.has("AGUARDANDO ESTADO / SEM ENTRADA"));
 t=training(a,b);v.screen=Screen::Ended;for(auto reason:{EndReason::Won,EndReason::Lost,EndReason::Left,EndReason::PeerLeft,EndReason::LinkLost,EndReason::JoinTimeout,EndReason::Protocol,EndReason::Ineligible}){v.end=reason;char name[40];std::snprintf(name,sizeof(name),"ended-%u",unsigned(reason));capture(out,name,v,t,a,b);assert(canvas.has("PERSONAGENS INALTERADOS")&&canvas.has("SEM XP / OURO / ITENS RECEBIDOS"));}
 v.screen=Screen::Unavailable;v.end=EndReason::Ineligible;capture(out,"unavailable",v,t,a,b);
 // Real appearance and exact state stats across every class/race pairing.
 v.screen=Screen::Playing;for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race){a=profile(cls,race);b=profile((cls+1)%4,(race+1)%4);t=training(a,b);v.localProfile=t.endpoint.state.actors[0];v.peerProfile=t.endpoint.state.actors[1];char name[48];std::snprintf(name,sizeof(name),"class-%u-race-%u",cls,race);capture(out,name,v,t,a,b);}
 // Full 16-bit stat labels stay inside panels. No arbitrary wire profile overrun.
 t.endpoint.state.actors[0].hp=t.endpoint.state.actors[0].maxhp=65535;t.endpoint.state.actors[0].mp=t.endpoint.state.actors[0].maxmp=65535;capture(out,"max-stats",v,t,a,b);assert(canvas.has("HP 65535/65535")&&canvas.has("MP 65535/65535"));
 v.artReady=false;capture(out,"art-unavailable",v,t,a,b);assert(canvas.has("ARTES INDISPONIVEIS / USE SAIR"));auto* art=artMemory;artMemory=nullptr;renderChecked(v,t,a,b);artMemory=art;
 // Repeated renders remain deterministic and allocation-free (1,024+ frames).
 v.artReady=true;for(unsigned i=0;i<1024;++i)renderChecked(v,t,a,b);
 std::printf("PASS: %u actual C++ renders; Save30/RNG/training/model unchanged; zero C/C++ render allocations; all labels bounded; controls/targets and 76,800 mappings checked; 16 class/race captures\n",renders);
 std::free(artMemory);artMemory=nullptr;
}
