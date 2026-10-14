#include "CoopTouchUi.h"
#include "../../firmware/RPG_POKET_2/Persistence.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
using namespace coop;using namespace coopui;
static size_t allocations=0;static unsigned checks=0,scenarios=0;
void* operator new(std::size_t n){++allocations;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort();}}while(0)
static rpg::Game hero(uint8_t cls,uint8_t level,uint32_t seed){auto g=rpg::create(cls,seed);for(uint8_t l=1;l<level;++l){g.p.xp=rpg::xpNeeded(g);rpg::levelUp(g);}g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.race=cls;g.shirt=cls+2;g.trousers=cls+3;return g;}
static State initial(){State s{};auto a=hero(0,5,111),b=hero(2,7,222);CHECK(initialize(s,a,b,123));return s;}
static void clean(Endpoint&e){Packet p{};while(e.outgoing.pop(p)){};UiEvent u{};while(e.ui.pop(u)){};}
static void relay(Endpoint&a,Endpoint&b,uint32_t now){Packet p{};while(a.outgoing.pop(p))b.receive(a.local,p,now);}
static void sync(Endpoint&h,Endpoint&c,uint32_t now=0){for(unsigned i=0;i<4;++i){relay(h,c,now);relay(c,h,now);}CHECK(h.hasState&&c.hasState&&!h.paused&&!c.paused&&!h.awaitingAck);CHECK(stateHash(h.state)==stateHash(c.state));}
static void setup(Endpoint&h,Endpoint&c,State s=initial()){h.hostStart(s,0x778899AABBCCDDull,0);c.clientStart(h.nonce,0);sync(h,c);clean(h);clean(c);}
static void tap(TouchUi&u,Endpoint&e,int x,int y,uint32_t now=1){u.onTouch(e,true,x,y,now);u.onTouch(e,false,x,y,now+1);}
static void button(TouchUi&u,Endpoint&e,int b,uint32_t now=1){auto r=kButtons[b];tap(u,e,r.x+r.w/2,r.y+r.h/2,now);}
static void enemyTap(TouchUi&u,Endpoint&e,int s,uint32_t now=1){auto r=u.targetRect(e,uint8_t(s));tap(u,e,r.x+r.w/2,r.y+r.h/2,now);}
static Packet commandPacket(uint64_t nonce,const Command&c){Packet p{};auto w=begin(p,Type::Command,1,nonce,c.sequence);writeCommand(w,c);CHECK(finish(p,w));return p;}
static Packet snapshot(const Endpoint&h,const State&s){Packet p{};auto w=begin(p,Type::Snapshot,0,h.nonce,s.revision);writeState(w,s);CHECK(finish(p,w));return p;}
static void geometry(){
 for(int i=0;i<4;++i){auto a=kButtons[i];CHECK(a.x>=0&&a.y>=0&&a.x+a.w<=320&&a.y+a.h<=240);for(int j=i+1;j<4;++j)CHECK(!intersects(a,kButtons[j]));for(auto r:kEnemyHits)CHECK(!intersects(a,r));CHECK(!intersects(a,kBossHit));}
 for(int i=0;i<3;++i)for(int j=i+1;j<3;++j)CHECK(!intersects(kEnemyHits[i],kEnemyHits[j]));
 Endpoint h,c;setup(h,c);TouchUi u;u.sync(h);
 for(int mode=0;mode<3;++mode){if(mode==1)button(u,h,0);if(mode==2){State s=initial();s.wave=2;spawnWave(s);setup(h,c,s);u=TouchUi{};button(u,h,0);}
  for(int y=0;y<240;++y)for(int x=0;x<320;++x){unsigned hits=0;Hit expected{};if(mode==0){for(uint8_t b=0;b<4;++b)if(kButtons[b].contains(x,y)){++hits;expected={HitKind::Button,b};}}else{if(kButtons[3].contains(x,y)){++hits;expected={HitKind::Button,3};}for(uint8_t s=3;s<6;++s)if(u.eligible(h,s)&&u.targetRect(h,s).contains(x,y)){++hits;expected={HitKind::Enemy,s};}}CHECK(hits<=1);CHECK(u.hitTest(h,x,y)==expected);}
 }
 for(auto p:{Hit{HitKind::None,0}}){CHECK(u.hitTest(h,-1,0)==p);CHECK(u.hitTest(h,320,0)==p);CHECK(u.hitTest(h,0,240)==p);CHECK(u.hitTest(h,INT32_MAX,INT32_MIN)==p);}
 ++scenarios;
}
static void selectionCancelAndResources(){
 Endpoint h,c;setup(h,c);TouchUi u;uint32_t before=stateHash(h.state);button(u,h,0);CHECK(u.mode==Mode::Target&&u.eligible(h,3)&&u.eligible(h,4)&&u.eligible(h,5));CHECK(!u.eligible(h,0)&&!u.eligible(h,2)&&!u.eligible(h,255));CHECK(stateHash(h.state)==before&&u.submitted==0);
 tap(u,h,25,80);tap(u,h,179,90);CHECK(u.mode==Mode::Target&&stateHash(h.state)==before);button(u,h,3);CHECK(u.mode==Mode::Action&&u.cancelled==1&&stateHash(h.state)==before);
 button(u,h,1);CHECK(u.mode==Mode::Ability);button(u,h,0);CHECK(u.mode==Mode::Target&&u.selected==Action::Focus);button(u,h,3);CHECK(u.cancelled==2&&stateHash(h.state)==before&&h.outgoing.count==0);
 State s=initial();s.actors[0].mp=1;s.actors[0].mana=0;s.actors[0].life=0;setup(h,c,s);u=TouchUi{};before=stateHash(h.state);button(u,h,1);CHECK(u.mode==Mode::Action&&u.error==Error::Resource&&stateHash(h.state)==before);button(u,h,3);CHECK(u.mode==Mode::Action&&u.error==Error::Resource);
 s=initial();s.actors[0].hp-=4;s.actors[0].mp-=3;s.actors[0].life=2;s.actors[0].mana=2;setup(h,c,s);u=TouchUi{};before=stateHash(h.state);button(u,h,3);CHECK(u.mode==Mode::Items);button(u,h,3);CHECK(stateHash(h.state)==before);button(u,h,3);button(u,h,0);CHECK(u.mode==Mode::Pending&&h.state.actors[0].hp==s.actors[0].maxhp&&h.state.actors[0].life==1);CHECK(h.state.actors[1].hp==s.actors[1].hp);sync(h,c);u.sync(h);CHECK(u.mode==Mode::Action);
 ++scenarios;
}
static void gesturesAndDuplicates(){
 Endpoint h,c;setup(h,c);TouchUi u;uint32_t before=stateHash(h.state);
 u.onTouch(h,false,40,214,1);CHECK(u.mode==Mode::Action);u.onTouch(h,true,40,214,2);u.onTouch(h,true,40,214,3);u.onTouch(h,false,0,0,4);CHECK(u.mode==Mode::Action&&stateHash(h.state)==before);
 u.onTouch(h,true,40,214,5);u.onTouch(h,true,283,96,6);u.onTouch(h,false,283,96,7);CHECK(u.mode==Mode::Action);button(u,h,0);CHECK(u.mode==Mode::Target);
 u.onTouch(h,true,283,96,10);for(int i=0;i<100;++i)u.onTouch(h,true,283,96,11+i);CHECK(h.state.revision==0);u.onTouch(h,false,283,96,111);CHECK(h.state.revision==1&&u.submitted==1&&u.mode==Mode::Pending&&h.awaitingAck);uint32_t after=stateHash(h.state);for(int i=0;i<30;++i){u.onTouch(h,false,283,96,112+i);button(u,h,3);enemyTap(u,h,4);}CHECK(stateHash(h.state)==after&&u.submitted==1);
 sync(h,c,150);u.sync(h);CHECK(u.mode==Mode::Action&&u.notice==Notice::Confirmed);button(u,h,0);CHECK(u.mode==Mode::Action&&u.submitted==1); // opponent's turn
 TouchUi cu;cu.sync(c);button(cu,c,0);CHECK(cu.mode==Mode::Target);before=stateHash(c.state);enemyTap(cu,c,3,160);CHECK(c.pending&&cu.mode==Mode::Pending&&stateHash(c.state)==before);for(int i=0;i<20;++i)enemyTap(cu,c,3,170+i);CHECK(cu.submitted==1&&c.state.accepted[1]==0);sync(h,c,200);cu.sync(c);CHECK(cu.mode==Mode::Action&&c.state.accepted[1]==1);
 ++scenarios;
}
static void snapshotsAndInvalidTargets(){
 Endpoint h,c;State s=initial();s.turn=1;setup(h,c,s);TouchUi u;button(u,c,0);CHECK(u.mode==Mode::Target);u.onTouch(c,true,283,96,10);
 State next=c.state;next.actors[4].hp=0;++next.revision;CHECK(validState(next));c.receive(0,snapshot(h,next),11);u.sync(c);CHECK(u.mode==Mode::Action&&u.staleCleared==1);u.onTouch(c,false,283,96,12);CHECK(!c.pending&&u.submitted==0);button(u,c,0);CHECK(!u.eligible(c,4));enemyTap(u,c,4);CHECK(!c.pending&&u.mode==Mode::Target);button(u,c,3);
 // New wave reuses slot but changes instance ID. An old gesture cannot target it.
 button(u,c,0);u.onTouch(c,true,215,70,20);next=c.state;next.wave=1;++next.revision;spawnWave(next);c.receive(0,snapshot(h,next),21);u.onTouch(c,false,215,70,22);CHECK(u.mode==Mode::Action&&!c.pending&&u.submitted==0);
 // Turn switch, transport pause and session switch independently clear selection.
 button(u,c,0);next=c.state;next.turn=0;++next.revision;c.receive(0,snapshot(h,next),30);u.sync(c);CHECK(u.mode==Mode::Action);next.turn=1;++next.revision;c.receive(0,snapshot(h,next),31);button(u,c,0);CHECK(u.mode==Mode::Target);c.paused=true;u.sync(c);CHECK(u.mode==Mode::Action);c.paused=false;button(u,c,0);c.clientStart(h.nonce+1,32);u.sync(c);CHECK(u.mode==Mode::Action&&!u.playable(c));
 // Dead and empty actor slots are never hits, even if their art could be fading.
 s=initial();s.actors[3].hp=0;setup(h,c,s);u=TouchUi{};button(u,h,0);CHECK(!u.eligible(h,3)&&u.eligible(h,4));enemyTap(u,h,3);CHECK(h.state.revision==0);s.wave=2;spawnWave(s);setup(h,c,s);u=TouchUi{};button(u,h,0);CHECK(u.eligible(h,3)&&!u.eligible(h,4)&&!u.eligible(h,5));CHECK(u.targetRect(h,255).w==0);
 ++scenarios;
}
static void hostCannotBeBypassed(){
 Endpoint h,c;State s=initial();s.turn=1;s.actors[1].mp=0;s.actors[3].hp=0;setup(h,c,s);uint32_t before=stateHash(h.state);Command cmd{1,0,1,4,Action::Focus};h.receive(1,commandPacket(h.nonce,cmd),10);CHECK(h.lastError==Error::Resource&&stateHash(h.state)==before);
 cmd.action=Action::Attack;cmd.target=0;h.receive(1,commandPacket(h.nonce,cmd),11);CHECK(h.lastError==Error::Target&&stateHash(h.state)==before);cmd.target=3;h.receive(1,commandPacket(h.nonce,cmd),12);CHECK(h.lastError==Error::Target&&stateHash(h.state)==before);cmd.target=255;h.receive(1,commandPacket(h.nonce,cmd),13);CHECK(h.lastError==Error::Invalid&&stateHash(h.state)==before);cmd.actor=0;cmd.target=4;h.receive(1,commandPacket(h.nonce,cmd),14);CHECK(h.lastError==Error::Turn&&stateHash(h.state)==before);
 // UI queued from an old accepted snapshot: host's newer revision rejects; no rebase/replay.
 setup(h,c,s);TouchUi u;button(u,c,0);enemyTap(u,c,4,20);CHECK(c.pending);h.state.revision=1;CHECK(validState(h.state));relay(c,h,21);CHECK(h.lastError==Error::Revision&&h.state.accepted[1]==0);relay(h,c,22);u.sync(c);CHECK(!c.pending&&u.mode==Mode::Action&&u.notice==Notice::Rejected);sync(h,c,25);u.sync(c);CHECK(c.state.revision==1&&c.state.accepted[1]==0);c.tick(500);relay(c,h,500);CHECK(h.state.accepted[1]==0);
 ++scenarios;
}
static void droppedAckAndRetry(){
 Endpoint h,c;State s=initial();s.turn=1;setup(h,c,s);TouchUi u;button(u,c,1);button(u,c,0);uint16_t mp=c.state.actors[1].mp;enemyTap(u,c,4,10);CHECK(c.pending&&c.state.actors[1].mp==mp);Packet command{};CHECK(c.outgoing.pop(command));h.receive(1,command,11);uint32_t once=stateHash(h.state),rng=h.state.rng;CHECK(h.state.actors[1].mp==mp-2);clean(c);relay(h,c,12);u.sync(c);CHECK(c.state.actors[1].mp==mp-2&&u.mode==Mode::Action);clean(c); // drop ACK
 h.tick(220);relay(h,c,220);CHECK(c.duplicateEvents==1&&stateHash(c.state)==once);relay(c,h,221);CHECK(!h.awaitingAck&&stateHash(h.state)==once&&h.state.rng==rng);
 h.receive(1,command,222);CHECK(stateHash(h.state)==once&&h.state.accepted[1]==1);sync(h,c,223);CHECK(h.state.rng==rng&&u.submitted==1);
 // An unreceived command retries with the identical sequence and no premature cost.
 setup(h,c,s);u=TouchUi{};button(u,c,1);button(u,c,0);enemyTap(u,c,4,10);Packet original{},retry{};CHECK(c.outgoing.pop(original));c.tick(220);CHECK(c.outgoing.pop(retry));CHECK(original.size==retry.size&&std::memcmp(original.bytes,retry.bytes,original.size)==0);CHECK(c.state.actors[1].mp==mp);h.receive(1,retry,221);sync(h,c,222);CHECK(c.state.actors[1].mp==mp-2&&h.state.accepted[1]==1);
 ++scenarios;
}
static void noCampaignWrites(){
 auto a=hero(0,5,111),b=hero(2,7,222);uint8_t beforeA[160],beforeB[160],after[160];rpg::encodePersistent(a,88,0,beforeA);rpg::encodePersistent(b,89,1,beforeB);uint32_t ar=a.randomState,br=b.randomState;State s{};CHECK(initialize(s,a,b,123));Endpoint h,c;setup(h,c,s);TouchUi u;button(u,h,1);button(u,h,0);button(u,h,3);button(u,h,0);enemyTap(u,h,4);sync(h,c);rpg::encodePersistent(a,88,0,after);CHECK(std::memcmp(beforeA,after,160)==0&&a.randomState==ar);rpg::encodePersistent(b,89,1,after);CHECK(std::memcmp(beforeB,after,160)==0&&b.randomState==br);++scenarios;
}
static void traceRow(FILE*f,const char*label,uint32_t ms,TouchUi&u,Endpoint&e,int x=-1,int y=-1,const UiEvent*event=nullptr){
 u.sync(e);const auto&s=e.state;
 std::fprintf(f,"{\"timestamp_ms\":%u,\"step\":\"%s\",\"status\":\"%s\",\"ui_mode\":\"%s\",\"selected_action\":%u,\"local\":%u,\"eligible_slots\":[",ms,label,label,modeName(u.mode),unsigned(u.selected),e.local);bool sep=false;for(uint8_t i=3;i<6;++i)if(u.eligible(e,i)){std::fprintf(f,"%s%u",sep?",":"",i);sep=true;}
 std::fprintf(f,"],\"click\":");if(x>=0)std::fprintf(f,"[%d,%d]",x,y);else std::fputs("null",f);
 std::fprintf(f,",\"revision\":%u,\"actor_id\":%d,\"target_id\":%d,\"action\":%d,\"amount\":%u,\"critical\":%s,\"from_wave\":%u,\"wave\":%u,\"round\":%u,\"next_actor_id\":%u,\"phase\":%u,\"state_hash\":%u,\"pending\":%s,\"awaiting_ack\":%s,\"actors\":[",s.revision,event?int(event->effect.actor):-1,event?int(event->effect.target):-1,event?int(event->effect.action):-1,event?event->effect.amount:0,event&&event->effect.critical?"true":"false",event?event->effect.fromWave+1:s.wave+1,s.wave+1,s.round,s.actors[s.turn].id,unsigned(s.phase),stateHash(s),e.pending?"true":"false",e.awaitingAck?"true":"false");
 for(unsigned i=0;i<kActors;++i){const Actor&a=s.actors[i];std::fprintf(f,"%s{\"slot\":%u,\"id\":%u,\"kind\":%u,\"species\":%u,\"class\":%u,\"level\":%u,\"hp\":%u,\"maxhp\":%u,\"mp\":%u,\"maxmp\":%u,\"race\":%u,\"shirt\":%u,\"trousers\":%u,\"guard\":%u}",i?",":"",i,a.id,unsigned(a.kind),a.species,a.cls,a.level,a.hp,a.maxhp,a.mp,a.maxmp,a.race,a.shirt,a.trousers,a.guard);}
 std::fputs("]}\n",f);
}
static void generateTrace(const char*path){
 FILE*f=std::fopen(path,"w");CHECK(f);Endpoint h,c;setup(h,c);CHECK(h.submit(Action::Guard,0,1)==Error::None);sync(h,c,2);clean(h);clean(c);TouchUi u;
 traceRow(f,"J2: escolha a acao",0,u,c);u.onTouch(c,true,41,214,300);traceRow(f,"Toque em Ataque",300,u,c,41,214);u.onTouch(c,false,41,214,600);traceRow(f,"Ataque: escolha o inimigo",600,u,c);
 u.onTouch(c,true,283,96,1100);traceRow(f,"Toque no segundo inimigo: Lobo",1100,u,c,283,96);u.onTouch(c,false,283,96,1350);traceRow(f,"Comando enviado: aguardando host",1350,u,c);relay(c,h,1400);traceRow(f,"Host resolveu; resposta ainda retida",1700,u,c);relay(h,c,2000);UiEvent event{};CHECK(c.ui.pop(event));traceRow(f,"Ataque confirmado pelo host",2000,u,c,-1,-1,&event);relay(c,h,2001);
 // Finish the independent enemies' turns only through the existing host tick/ACK flow.
 for(uint32_t t=2100;h.state.turn!=0&&t<3000;t+=100){h.tick(t);c.tick(t);sync(h,c,t);}CHECK(h.state.turn==0);clean(h);clean(c);TouchUi hu;
 traceRow(f,"J1: escolha uma habilidade",3100,hu,h);hu.onTouch(h,true,120,214,3200);traceRow(f,"Toque em Habilidade",3200,hu,h,120,214);hu.onTouch(h,false,120,214,3300);traceRow(f,"Habilidade: Foco experimental",3400,hu,h);button(hu,h,0,3500);traceRow(f,"Foco: escolha o inimigo",3700,hu,h);const auto unchanged=stateHash(h.state);hu.onTouch(h,true,278,214,3900);traceRow(f,"Toque em Cancelar",3900,hu,h,278,214);hu.onTouch(h,false,278,214,4000);CHECK(stateHash(h.state)==unchanged);traceRow(f,"Cancelado: MP e turno preservados",4100,hu,h);
 // Separate valid boss fixture, explicitly labelled. It is not a skipped campaign progression.
 State boss=initial();boss.wave=2;spawnWave(boss);setup(h,c,boss);hu=TouchUi{};button(hu,h,0,4300);traceRow(f,"Cenario separado: alvo chefe",4500,hu,h);std::fclose(f);
}
int main(int argc,char**argv){size_t baseline=allocations;geometry();selectionCancelAndResources();gesturesAndDuplicates();snapshotsAndInvalidTargets();hostCannotBeBypassed();droppedAckAndRetry();noCampaignWrites();if(argc>1)generateTrace(argv[1]);CHECK(allocations==baseline);std::printf("PASS checks=%u scenarios=%u cpp_heap_allocations=%zu sizeof_TouchUi=%zu sizeof_Endpoint=%zu geometry_points=230400\n",checks,scenarios,allocations-baseline,sizeof(TouchUi),sizeof(Endpoint));}
