// Focused PvP presentation regression/capture. Real renderer + tracked art pack.
#include "SceneCanvas.h"
#include <deque>
#include <filesystem>
#include <iostream>
#include <array>

rpg::Game game;ViewState view;Page afterSave=Page::Home;bool dirty=false;
uint32_t fakeNow=100;uint32_t millis(){return fakeNow;}uint32_t esp_random(){return 24;}
void resumeConnection(){}void say(const char*){}Page currentPage(){return Page::Home;}
struct FakeJournal{unsigned writes=0;bool save(const rpg::Game&){++writes;return true;}}journal;
void savedTransition(Page next){afterSave=next;view.page=next;journal.save(game);dirty=true;}
#include "../firmware/RPG_POKET_2/ClubRadio.h"

static bool rendering=false;static unsigned allocationCalls=0;
extern "C" void* __real_malloc(size_t);extern "C" void* __real_calloc(size_t,size_t);extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){if(rendering)++allocationCalls;return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t s){if(rendering)++allocationCalls;return __real_calloc(n,s);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(rendering)++allocationCalls;return __real_realloc(p,n);}
struct Probe:Canvas{
 struct Label{int x,y,size;char text[80];};std::array<Label,128> labels{};unsigned count=0;
 void print(const char* s){assert(count<labels.size());auto& label=labels[count++];label.x=tx;label.y=ty;label.size=size;snprintf(label.text,sizeof(label.text),"%s",s);Canvas::print(s);}
 bool has(const char* s)const{for(unsigned i=0;i<count;++i)if(!strcmp(labels[i].text,s))return true;return false;}
 void clear(){count=0;}
 void fillRect(int x,int y,int w,int h,uint16_t color){assert(x>=0&&y>=0&&w>=0&&h>=0&&x+w<=240&&y+h<=320);Canvas::fillRect(x,y,w,h,color);}
 void draw16bitRGBBitmap(int x,int y,uint16_t* p,int w,int h){assert(x>=0&&y>=0&&w>=0&&h>=0&&x+w<=240&&y+h<=320);Canvas::draw16bitRGBBitmap(x,y,p,w,h);}
};
struct Net{
 club::Duel duel[2];ClubVisual visual[2];rpg::Game games[2];
 struct Context{Net* net;unsigned who;}context[2]={{this,0},{this,1}};
 struct Message{unsigned who;uint16_t seq;uint8_t action;bool ack;};std::deque<Message> messages;unsigned now=100;
 static void send(void* p,uint16_t seq,uint8_t action,bool ack){auto& c=*static_cast<Context*>(p);c.net->messages.push_back({c.who,seq,action,ack});}
 void flush(){unsigned count=0;while(!messages.empty()){assert(++count<100);auto m=messages.front();messages.pop_front();auto& receiver=duel[1-m.who];receiver.receive(m.seq,m.action,m.ack,now);visual[1-m.who].capture(receiver,now);}assert(duel[0].revision==duel[1].revision);for(unsigned f=0;f<2;++f){assert(duel[0].fighters[f].hp==duel[1].fighters[f].hp);assert(duel[0].fighters[f].mp==duel[1].fighters[f].mp);assert(duel[0].fighters[f].guard==duel[1].fighters[f].guard);}}
 Net(unsigned a=3,unsigned b=0,unsigned level=5){for(unsigned i=0;i<2;++i){games[i]=rpg::create(i?b:a,42);games[i].p.level=level;games[i].guildMember=true;games[i].p.gold=100;}games[0].race=3;games[0].shirt=2;games[0].trousers=7;games[1].race=1;games[1].shirt=1;games[1].trousers=4;
  duel[0].begin(24,true,a,level,b,level,send,&context[0],now);duel[1].begin(24,false,b,level,a,level,send,&context[1],now);duel[0].tick(now);duel[1].tick(now);flush();assert(duel[0].ready&&duel[1].ready);}
 void play(club::Action action){now+=1200;unsigned actor=duel[0].turn;auto old=visual[actor];assert(duel[actor].play(action,now));assert(!visual[actor].capture(duel[actor],now));assert(!memcmp(&old,&visual[actor],sizeof(old)));flush();}
 void install(unsigned side){arena=ClubUi{};arena.duel=duel[side];arena.visual=visual[side];auto& peer=arena.session.opponent;peer.classId=games[1-side].p.cls;peer.race=games[1-side].race;peer.shirt=games[1-side].shirt;peer.trousers=games[1-side].trousers;view=ViewState{};view.page=Page::ClubBattle;}
};
static unsigned renders=0,immutable=0,matrix=0,actionMatrix=0;
void draw(Probe& c,const rpg::Game& g,unsigned frame=0){auto before=g;auto ui=arena;auto state=view;c.clear();rendering=true;render(c,g,view,frame);rendering=false;++renders;assert(!memcmp(&before,&g,sizeof(g))&&!memcmp(&ui,&arena,sizeof(arena))&&!memcmp(&state,&view,sizeof(view)));assert(!allocationCalls);++immutable;}
bool sameRegion(const Canvas& a,const Canvas& b,int x,int y,int w,int h){for(int row=y;row<y+h;++row)for(int col=x;col<x+w;++col)if(a.pixels[row*240+col]!=b.pixels[row*240+col])return false;return true;}
void outsideStable(const Canvas& a,const Canvas& b){assert(sameRegion(a,b,0,0,240,38)&&sameRegion(a,b,0,158,240,162));}
void identity(Probe& c,Net& n,unsigned side){n.install(side);draw(c,n.games[side]);Canvas expected;drawBackdrop(expected,backdropFor(Page::ClubBattle,n.games[side]));personalSprite(expected,n.games[side],8,38,0);personalSprite(expected,n.games[1-side],124,38,0,true);assert(sameRegion(c,expected,0,38,240,120));}
int main(int argc,char** argv){
 assert(argc==3);const std::string root=argv[2];std::filesystem::create_directories(root);artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);std::ifstream pack(argv[1],std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);menu=MenuState{};menu.art=ArtStatus::Ready;Probe c;
 // A fresh/default or expired visual cannot become active just because time ticks.
 ClubVisual untouched;assert(!untouched.tick(1)&&!untouched.active());assert(!untouched.tick(UINT32_MAX)&&!untouched.active());
 Net n;for(unsigned side=0;side<2;++side){identity(c,n,side);c.save(root+"/perspective-"+std::to_string(side)+"-start.ppm");Canvas baseline=c;for(unsigned frame=0;frame<18;++frame){draw(c,n.games[side],frame);assert(sameRegion(baseline,c,0,0,240,320));if(!side)c.save(root+"/idle-"+std::to_string(frame)+".ppm");}}
 // Independent shirt/trousers colors, every class/race, and both perspectives.
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race)for(unsigned shirt=0;shirt<8;++shirt)for(unsigned pants=0;pants<8;++pants){Net sample(cls,(cls+1)%4,99);sample.games[0].race=race;sample.games[0].shirt=shirt;sample.games[0].trousers=pants;sample.games[1].race=(race+1)%4;sample.games[1].shirt=pants;sample.games[1].trousers=shirt;
  for(unsigned side=0;side<2;++side){identity(c,sample,side);++matrix;if(side==0&&shirt==race+1&&pants==7-race)c.save(root+"/matrix-class-"+std::to_string(cls)+"-race-"+std::to_string(race)+".ppm");}}
 // Every class/race, all colors, attack/offensive/defense, both actor directions,
 // wind-up/contact/recovery/expiry. The HUD and buttons never move with effects.
 const unsigned phases[]={0,120,240,600,720,960,1100};
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race)for(unsigned color=0;color<8;++color)for(unsigned action=0;action<3;++action){Net sample(cls,(cls+1)%4,20);sample.games[0].race=race;sample.games[0].shirt=color;sample.games[0].trousers=7-color;
  if(action==2)for(auto& d:sample.duel)d.fighters[0].hp-=10;sample.play(club::Action(action));
  for(unsigned side=0;side<2;++side){sample.install(side);assert(arena.visual.active());draw(c,sample.games[side]);Canvas baseline=c;for(unsigned elapsed:phases){arena.visual.tick(sample.now+elapsed);draw(c,sample.games[side],elapsed/120);outsideStable(baseline,c);Canvas repeated=c;draw(c,sample.games[side],999);assert(sameRegion(repeated,c,0,0,240,320));++actionMatrix;
    if(elapsed==600){if(action<2){char number[24];snprintf(number,sizeof(number),"-%u HP",arena.visual.damage);assert(c.has(number));}else if(cls==3){char number[24];snprintf(number,sizeof(number),"+%u HP",arena.visual.healing);assert(c.has(number));}else assert(c.has("GUARDA"));}
   }assert(!arena.visual.active()&&!arena.visual.tick(sample.now+3000));Canvas expired=c;arena.visual=ClubVisual{};draw(c,sample.games[side]);assert(sameRegion(expired,c,0,0,240,320));}}
 // Duplicate revisions do not replay. UINT32 timer wrap preserves finite playback.
 n.play(club::Action::Defensive);ClubVisual event=n.visual[0];assert(!event.capture(n.duel[0],n.now+900)&&event.startedAt==n.now);event.startedAt=UINT32_MAX-99;event.elapsedMs=0;assert(event.tick(100)&&event.elapsedMs==200);assert(event.tick(1100)&&!event.active());
 // Real clubTap: mana failure is visible, and no game/action/animation is applied.
 Net failure(0,1);failure.install(0);arena.begun=true;game=failure.games[0];game.clubStage=2;arena.duel.fighters[0].mp=0;draw(c,game);Canvas before=c;auto revision=arena.duel.revision;auto visual=arena.visual;clubTap(130,230);assert(arena.duel.revision==revision&&!strcmp(arena.notice,"Mana insuficiente")&&!memcmp(&visual,&arena.visual,sizeof(visual)));draw(c,game);assert(c.has("Mana insuficiente")&&c.has("3 MP / sem mana")&&!sameRegion(before,c,0,200,240,20));c.save(root+"/insufficient-mana-visible.ppm");
 // Pending local prediction is explicitly confirming, never a confirmed effect.
 failure.install(0);arena.begun=true;game=failure.games[0];game.clubStage=2;fakeNow=1000;clubTap(20,230);assert(arena.duel.waiting&&!arena.visual.active());draw(c,game);assert(c.has("Confirmando acao"));Canvas pending=c;draw(c,game,91);assert(sameRegion(pending,c,0,0,240,320));c.save(root+"/unacknowledged-action.ppm");
 // The real receive callback commits the event on a matching ACK only, once.
 clubMoveReceive(nullptr,arena.duel.revision,uint8_t(club::Action::Offensive),true,1005);assert(!arena.visual.active());clubMoveReceive(nullptr,arena.duel.revision,uint8_t(club::Action::Attack),true,1010);assert(arena.visual.active()&&arena.visual.revision==arena.duel.confirmedRevision);auto start=arena.visual.startedAt;clubMoveReceive(nullptr,arena.duel.revision,uint8_t(club::Action::Attack),true,1500);assert(arena.visual.startedAt==start);
 // A real incoming committed defense starts once; no +0 HP at full health.
 Net incoming(0,3);incoming.install(0);arena.begun=true;arena.duel.turn=1;game=incoming.games[0];game.clubStage=2;
 clubMoveReceive(nullptr,1,uint8_t(club::Action::Defensive),false,1600);assert(arena.visual.active()&&arena.visual.actor==1&&arena.visual.healing==0);arena.visual.tick(2200);draw(c,game);assert(c.has("GUARDA")&&!c.has("+0 HP"));auto remoteStart=arena.visual.startedAt;clubMoveReceive(nullptr,1,uint8_t(club::Action::Defensive),false,2300);assert(arena.visual.startedAt==remoteStart);
 // Surrender remains available on the remote turn; it starts no attack animation.
 failure.install(0);arena.begun=true;arena.duel.turn=1;game=failure.games[0];game.clubStage=2;assert(arena.duel.canPlay(club::Action::Surrender));clubTap(130,280);assert(arena.duel.surrenderRequested&&!arena.visual.active());draw(c,game);assert(c.has("Confirmando desistencia"));c.save(root+"/remote-turn-surrender.ppm");
 // Exact existing touch geometry, including exclusive high edges.
 for(unsigned i=0;i<4;++i){int x=14+int(i%2)*110,y=220+int(i/2)*50;assert(hit(x,y,x,y,102)&&hit(x+101,y+39,x,y,102)&&!hit(x+102,y,x,y,102)&&!hit(x,y+40,x,y,102));}
 for(unsigned level:{5u,20u,99u})for(unsigned cls=0;cls<4;++cls){Net sample(cls,(cls+1)%4,level);for(unsigned side=0;side<2;++side)for(unsigned state=0;state<4;++state){sample.install(side);arena.duel.ready=state!=0;arena.duel.waiting=state==1;arena.duel.turn=state==3?1-side:side;draw(c,sample.games[side]);if(level==5&&cls==0&&side==0)c.save(root+"/readiness-"+std::to_string(state)+".ppm");}}
 // Readable comparisons/GIFs: production renderer frames at nominal 60 ms.
 for(unsigned cls=0;cls<4;++cls)for(unsigned action=0;action<3;++action){Net sample(cls,(cls+1)%4,20);sample.games[0].race=cls;sample.games[0].shirt=cls+1;sample.games[0].trousers=7-cls;if(action==2)for(auto& d:sample.duel)d.fighters[0].hp-=10;sample.play(club::Action(action));for(unsigned side=0;side<2;++side){sample.install(side);for(unsigned frame=0;frame<23;++frame){arena.visual.tick(sample.now+frame*60);draw(c,sample.games[side],frame);c.save(root+"/class-"+std::to_string(cls)+"-action-"+std::to_string(action)+"-side-"+std::to_string(side)+"-frame-"+std::to_string(frame)+".ppm");}}}
 // Defense persists in the HUD after its transient effect ends; next hit clears it.
 Net guarded(3,0);for(auto& d:guarded.duel)d.fighters[0].hp-=5;guarded.play(club::Action::Defensive);guarded.install(0);arena.visual.tick(guarded.now+1100);draw(c,guarded.games[0]);assert(c.has("Guarda 50%"));c.save(root+"/guard-persistent.ppm");guarded.play(club::Action::Attack);guarded.install(0);draw(c,guarded.games[0]);assert(!c.has("Guarda 50%"));
 std::ofstream report(root+"/render-results.txt");report<<"PASS: "<<matrix<<" independent identity matrix cases; "<<actionMatrix<<" action phase cases; "<<renders<<" total renders; "<<immutable<<" Game/Duel/Session/Visual/View immutability checks.\n";report<<"PASS: idle pose 0, exact local/remote appearance pixels, damage/healing, persistent guard, mana failure through real clubTap, ACK-only local action capture, duplicate suppression, expiry, timer wrap, both perspectives, unchanged hitboxes, bounded primitives/text, zero wrapped render allocation calls.\n";report<<"Host sizeof: ClubVisual="<<sizeof(ClubVisual)<<", ClubUi="<<sizeof(ClubUi)<<", ViewState="<<sizeof(ViewState)<<", Game="<<sizeof(rpg::Game)<<" bytes. No new image payload/framebuffer/PSRAM allocation. These host sizes and nominal capture timings are not ESP32 stack/heap/FPS/latency measurements.\n";report.close();std::cout<<"PASS: "<<matrix<<" identity cases, "<<actionMatrix<<" action phases, "<<renders<<" immutable native renders; allocation calls="<<allocationCalls<<"; ClubVisual="<<sizeof(ClubVisual)<<" bytes.\n";free(artMemory);artMemory=nullptr;
}
