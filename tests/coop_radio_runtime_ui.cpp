// Active production View + controller + CoopDevice integration. The unchanged
// package12 controller fixture is generated with only its test main renamed.
#include "coop_controller_fixture.generated.h"
#include "SceneCanvas.h"

namespace runtimeui {
using coopplatform::Session;
using coopplatform::SessionState;
using coopplatform::SessionEnd;
using coopplatform::Mac;
using coopview::Control;
unsigned cases=0,renderChecks=0;

struct Portrait:Canvas {
 void drawPixel(int x,int y,uint16_t color){assert(x>=0&&x<240&&y>=0&&y<320);pixel(x,y,color);}
};
struct LabelCanvas:coopplatform::LandscapeCanvas<Portrait> {
 using Base=coopplatform::LandscapeCanvas<Portrait>;
 std::vector<std::string> labels;
 explicit LabelCanvas(Portrait& portrait):Base(portrait){}
 void print(const char* value){labels.emplace_back(value);Base::print(value);}
 bool has(const char* value)const{return std::find(labels.begin(),labels.end(),value)!=labels.end();}
};
struct Unchanged {
 Game saved=game;decltype(nvs.blobs) blobs=nvs.blobs;
 decltype(journal.seq) sequence=journal.seq;
 decltype(journal.active) active=journal.active;
 decltype(backend.slot) slot=backend.slot;
 void check()const{
  same(game,saved);assert(game.randomState==saved.randomState);
  assert(nvs.blobs==blobs&&journal.seq==sequence&&journal.active==active&&backend.slot==slot);
  assert(!fake::nvsWrites&&!fake::erases);
 }
};

// Real View dispatch must produce exactly the directly rendered live runtime
// model, not the historical standalone model whose linkTools defaults false.
void rendered(const char* required=nullptr){
 assert(coopIsOpen()&&view.page==Page::CoopTraining);
 const auto state=coopSession;const auto input=coopInput;const Unchanged unchanged;
 const auto mutations=fake::mutations;const auto sends=fake::sendCount;
 const auto vm=coopruntime::view(menu.art==ArtStatus::Ready);assert(vm.linkTools);
 static Portrait actual,direct,historical;
 render(actual,game,view);
 LabelCanvas expected(direct);coopview::draw(expected,vm,coopSession.training());
 assert(!std::memcmp(actual.pixels,direct.pixels,sizeof(actual.pixels)));
 assert(expected.has("SAIR")&&expected.has("SEM XP/OURO")&&expected.has(vm.diagnostics?"VOLTAR":"INFO"));
 if(required&&!expected.has(required)){
  std::fprintf(stderr,"Missing live runtime label: %s\n",required);
  for(const auto& label:expected.labels)std::fprintf(stderr,"  %s\n",label.c_str());
  assert(expected.has(required));
 }
 auto withoutTools=vm;withoutTools.linkTools=false;
 coopplatform::LandscapeCanvas<Portrait> legacy(historical);coopview::draw(legacy,withoutTools,coopSession.training());
 assert(std::memcmp(actual.pixels,historical.pixels,sizeof(actual.pixels))!=0);
 assert(!std::memcmp(&state,&coopSession,sizeof(state))&&!std::memcmp(&input,&coopInput,sizeof(input)));
 assert(fake::mutations==mutations&&fake::sendCount==sends);unchanged.check();++renderChecks;
}

struct Frame {unsigned from=0;Mac destination{};size_t size=0;uint8_t bytes[coop::kMaxPacket]{};};
struct Pair {
 Session peer;
 struct Port {Pair* pair;unsigned from;} ports[2];
 coop::Queue<Frame,512> frames;
 Mac macs[2]={{{2,0,0,0,0,1}},{{2,0,0,0,0,2}}};
 bool partition=false;
 explicit Pair(bool localHost,bool connect=true){
  setup(0,0,0);enterGuild();press(175,238);assert(coopIsOpen());
  menu.art=ArtStatus::Ready;
  ports[0]={this,0};ports[1]={this,1};
  coopSession.setSender(send,&ports[0]);peer.setSender(send,&ports[1]);
  assert(peer.open(game,macs[1],0xabcdef,now));flush();
  if(connect)start(localHost);
 }
 ~Pair(){assert(!coopIsOpen());}
 static bool send(void* context,const Mac& destination,const uint8_t* bytes,size_t size){
  auto& port=*static_cast<Port*>(context);auto& pair=*port.pair;
  if(pair.partition)return true;
  Frame frame{};frame.from=port.from;frame.destination=destination;frame.size=size;
  assert(size<=sizeof(frame.bytes));std::memcpy(frame.bytes,bytes,size);assert(pair.frames.push(frame));return true;
 }
 void flush(){
  Frame frame{};unsigned count=0;
  while(frames.pop(frame)){
   assert(++count<10000);unsigned target=1-frame.from;
   if(!coopplatform::sameMac(frame.destination,macs[target])&&frame.destination.bytes[0]!=255)continue;
   (target?peer:coopSession).receive(macs[frame.from],frame.bytes,frame.size,now);
  }
 }
 void start(bool localHost){
  assert(coopSession.state()==SessionState::Browsing&&peer.state()==SessionState::Browsing);
  if(localHost){
   coopClick(50,195);assert(coopSession.state()==SessionState::Hosting);rendered();flush();
   assert(peer.peer(0)&&peer.join(0,now));flush();assert(coopSession.state()==SessionState::AwaitingAccept);
   rendered("ACEITAR");coopClick(50,195);flush();
  }else{
   assert(peer.host(now));flush();assert(coopSession.peer(0));rendered();
   coopClick(200,60);flush();assert(peer.state()==SessionState::AwaitingAccept);
   rendered("AGUARDE ACEITE");assert(peer.accept(now));flush();
  }
  assert(coopSession.state()==SessionState::Playing&&peer.state()==SessionState::Playing);rendered();
 }
 void step(uint32_t elapsed){now+=elapsed;tickCoopTraining(now);peer.tick(now);flush();}
 void localTurn(){
  if(!coopSession.isHost()){
   assert(peer.training().endpoint.submit(coop::Action::Guard,0,now)==coop::Error::None);step(0);
  }
  assert(coopSession.training().ui.playable(coopSession.training().endpoint));
 }
 void recover(){
  partition=true;for(unsigned i=0;i<12;++i)step(250);
  assert(coopSession.state()==SessionState::Playing&&coopSession.recovering());rendered("CANCELAR RECONEXAO");
 }
 void lost(){
  partition=true;for(unsigned i=0;i<41;++i)step(250);
  assert(coopSession.state()==SessionState::Ended&&coopSession.endReason()==SessionEnd::LinkLost);
 }
};

void capture(bool valid,bool down,int x,int y,uint32_t elapsed=1){
 now+=elapsed;auto p=coopplatform::toPortrait(x,y);captureCoopTouch(valid,down,p.x,p.y,now);
}
void capturedClick(int x,int y){
 capture(true,false,0,0);capture(true,false,0,0,31);capture(true,true,x,y);
 capture(true,false,0,0);capture(true,false,0,0,31);
}
void clearedLobby(){
 assert(coopIsOpen()&&!coopClosing&&view.page==Page::CoopTraining);
 assert(coopradio::owner==coopradio::Owner::Training&&coopSession.state()==SessionState::Browsing);
 assert(!coopSession.sessionToken()&&!coopSession.joinToken()&&!coopSession.pendingPeer());
 assert(coopSession.endReason()==SessionEnd::None&&!coopSession.recovering()&&!coopInput.diagnosticsOpen());
 for(unsigned i=0;i<Session::kMaxPeers;++i)assert(!coopSession.peer(i));
 const auto& t=coopSession.training();assert(!t.active()&&!t.binding().nonce&&!t.endpoint.nonce);
 assert(!t.endpoint.hasState&&!t.endpoint.pending&&!t.endpoint.awaitingAck&&!t.endpoint.outgoing.count&&!t.endpoint.ui.count);
 const auto live=coopruntime::view(true);
 assert(!live.recovering&&!live.link.pending&&!live.link.awaitingAck&&!live.link.paused);
 assert(live.link.state==uint8_t(SessionState::Browsing));
 assert(!t.ui.submitted);rendered("CRIAR SALA");
}
void closedIn350(const Unchanged& unchanged,bool reenter=true){
 assert(coopClosing&&coopIsOpen());
 now+=349;tickCoopTraining(now);assert(coopIsOpen()&&view.page==Page::CoopTraining);
 ++now;tickCoopTraining(now);assert(!coopIsOpen()&&view.page==Page::Guild);
 assert(coopradio::owner==coopradio::Owner::None&&!pendingTouch&&!gate.armed);unchanged.check();
 // Physical storage and mapping are portrait again, and the old held finger
 // cannot activate Guild Back until a fresh release/press sequence.
 portrait(true,true,120,292);assert(view.page==Page::Guild);
 press(120,292);assert(view.page==Page::Village);unchanged.check();
 if(reenter){
  enterGuild();press(175,238);assert(coopIsOpen()&&coopSession.state()==SessionState::Browsing);
  coopSample(true,true,280,10);coopRelease();assert(!coopClosing);
  coopClick(280,10);closedIn350(unchanged,false);
 }
}
void exitTop(const Unchanged& unchanged){coopClick(280,10);closedIn350(unchanged);}

void diagnostics(){
 {
  Pair pair(true,false);const Unchanged unchanged;
  rendered("CRIAR SALA");menu.art=ArtStatus::Missing;rendered();menu.art=ArtStatus::Ready;
  capturedClick(235,10);capturedClick(50,195);tickCoopTraining(now);
  assert(coopInput.diagnosticsOpen()&&coopSession.state()==SessionState::Browsing);rendered("DIAGNOSTICO LOCAL");
  coopClick(50,195);assert(coopSession.state()==SessionState::Browsing);
  capturedClick(235,10);capturedClick(50,195);tickCoopTraining(now);
  assert(!coopInput.diagnosticsOpen()&&coopSession.state()==SessionState::Browsing);rendered("CRIAR SALA");
  exitTop(unchanged);++cases;
 }
 for(bool host:{true,false}){
  Pair pair(host);const Unchanged unchanged;pair.localTurn();
  auto& t=coopSession.training();const auto revision=t.endpoint.state.revision,submitted=t.ui.submitted;
  capturedClick(235,10);capturedClick(180,210);tickCoopTraining(now);
  assert(coopInput.diagnosticsOpen()&&t.ui.submitted==submitted&&t.endpoint.state.revision==revision);
  rendered("DIAGNOSTICO LOCAL");coopClick(180,210);
  assert(t.ui.submitted==submitted&&t.endpoint.state.revision==revision);
  capturedClick(235,10);capturedClick(180,210);tickCoopTraining(now);
  assert(!coopInput.diagnosticsOpen()&&t.ui.submitted==submitted&&t.endpoint.state.revision==revision);
  coopClick(180,210);pair.step(0);assert(t.ui.submitted==submitted+1&&t.endpoint.state.revision==revision+1);
  // SAIR stays available from the diagnostic modal and retains its drain.
  coopClick(235,10);assert(coopInput.diagnosticsOpen());exitTop(unchanged);++cases;
 }
 std::puts("PASS: active View pixel parity, INFO/VOLTAR modal gating and queued-command invalidation, both roles; fresh command and SAIR drain");
}

void recoveryCancel(){
 for(bool host:{true,false})for(bool queued:{false,true}){
  Pair pair(host);const Unchanged unchanged;pair.localTurn();
  // Leave a real locally submitted reliable action in flight. Host waits for
  // an ACK; guest retains a pending command. Cancellation must clear both.
  pair.partition=true;coopClick(180,210);pair.step(0);
  assert(coopSession.training().ui.submitted==1);
  assert(host?coopSession.training().endpoint.awaitingAck:coopSession.training().endpoint.pending);
  const auto token=coopSession.sessionToken(),join=coopSession.joinToken();
  const auto generation=coopSession.training().generationId();
  coopRelease();
  if(queued)capture(true,true,160,130);else coopSample(true,true,160,130);
  pair.recover();coopRelease();assert(coopSession.recovering()&&!coopClosing);
  const auto submitted=coopSession.training().ui.submitted;
  coopClick(180,210);assert(coopSession.training().ui.submitted==submitted);
  coopClick(235,10);assert(coopInput.diagnosticsOpen());rendered("DIAGNOSTICO LOCAL");
  coopClick(160,130);assert(coopSession.recovering());
  coopClick(235,10);assert(!coopInput.diagnosticsOpen());
  const auto epoch=coopradio::callbackCounters.generation();const auto mutations=fake::mutations;
  capturedClick(160,130);capturedClick(50,195);tickCoopTraining(now);
  clearedLobby();assert(coopradio::callbackCounters.generation()!=epoch&&fake::mutations==mutations);
  // No held or queued old contact may host or submit in the new admission.
  coopSample(true,true,50,195);coopRelease();assert(coopSession.state()==SessionState::Browsing);
  tickCoopTraining(now);clearedLobby();unchanged.check();
  pair.partition=false;pair.frames.clear();assert(pair.peer.returnToLobby(now));pair.flush();
  pair.start(!host);
  assert(coopSession.sessionToken()!=token&&coopSession.joinToken()!=join);
  assert(coopSession.training().generationId()>generation);
  exitTop(unchanged);++cases;
 }
 std::puts("PASS: host/guest recovery cancel stays landscape, clears session/training/tokens/old touch, preserves radio ownership, permits fresh opposite-role host/join");
}

void recoveryResume(){
 for(bool host:{true,false}){
  Pair pair(host);const Unchanged unchanged;pair.localTurn();pair.recover();
  auto& t=coopSession.training();const auto submitted=t.ui.submitted,revision=t.endpoint.state.revision;
  coopRelease();coopSample(true,true,180,210);
  pair.partition=false;for(unsigned i=0;i<4;++i)pair.step(250);
  assert(!coopSession.recovering()&&coopSession.state()==SessionState::Playing);
  coopRelease();assert(t.ui.submitted==submitted&&t.endpoint.state.revision==revision);
  coopClick(180,210);pair.step(0);assert(t.ui.submitted==submitted+1&&t.endpoint.state.revision==revision+1);
  rendered();exitTop(unchanged);++cases;
 }
 std::puts("PASS: closing recovery on RF restoration invalidates modal-held input; fresh gameplay resumes once, both roles");
}

void terminal(){
 for(bool host:{true,false}){
  Pair pair(host);const Unchanged unchanged;pair.lost();rendered("VOLTAR A SALA");
  assert(coopview::hitTest(coopruntime::view(true),160,199)==Control::ReturnLobby);
  const auto previous=coopruntime::view(true).link;
  coopClick(160,199);clearedLobby();
  const auto current=coopruntime::view(true).link;
  assert(current.lastEnd==previous.lastEnd&&current.lastEnd==uint8_t(SessionEnd::LinkLost));
  assert(current.retries==previous.retries&&current.snapshots==previous.snapshots&&current.ackAccepted==previous.ackAccepted);
  exitTop(unchanged);++cases;
 }
 {
  Pair pair(false,false);const Unchanged unchanged;
  assert(pair.peer.host(now));pair.flush();assert(coopSession.peer(0));pair.partition=true;
  coopClick(200,60);assert(coopSession.state()==SessionState::Joining);
  pair.step(Session::kAdmissionTimeoutMs);assert(coopSession.endReason()==SessionEnd::JoinTimeout);
  rendered("VOLTAR A SALA");assert(coopview::hitTest(coopruntime::view(true),160,199)==Control::ReturnLobby);
  coopClick(160,199);clearedLobby();exitTop(unchanged);++cases;
 }
 for(bool host:{true,false})for(SessionEnd reason:{SessionEnd::Won,SessionEnd::Lost,SessionEnd::PeerLeft}){
  Pair pair(host);const Unchanged unchanged;
  if(reason==SessionEnd::PeerLeft){pair.peer.leave(now);pair.flush();}
  else{
   // Valid terminal combat fixture; Session tick performs the actual transition.
   auto state=coopSession.training().endpoint.state;
   if(reason==SessionEnd::Won){state.wave=2;coop::spawnWave(state);for(unsigned i=coop::kPlayerCapacity;i<coop::kActors;++i)state.actors[i].hp=0;}
   else state.actors[0].hp=state.actors[1].hp=0;
   coop::advance(state);assert(coop::validState(state));
   for(Session* s:{&coopSession,&pair.peer}){auto& e=s->training().endpoint;e.state=state;e.pending=e.awaitingAck=false;}
   pair.step(0);
  }
  assert(coopSession.state()==SessionState::Ended&&coopSession.endReason()==reason);
  rendered("VOLTAR A GUILDA");assert(coopview::hitTest(coopruntime::view(true),160,199)==Control::Exit);
  coopClick(160,199);closedIn350(unchanged);++cases;
 }
 std::puts("PASS: live LinkLost/JoinTimeout center returns to lobby; Won/Lost/PeerLeft center exits to Guild through 350ms drain, both roles");
}
}

int main(){
 static_assert(RPG_ENABLE_COOP_TRAINING==1,"Active runtime integration requires coop ON");
 std::setvbuf(stdout,nullptr,_IONBF,0);
 // Art-independent control/dispatch proof; authored-art visual qualification
 // belongs to the dedicated radio diagnostics renderer, not these host tests.
 runtimeui::diagnostics();runtimeui::recoveryCancel();runtimeui::recoveryResume();runtimeui::terminal();
 std::printf("PASS: %u active runtime/controller cases, %u exact View/runtime render comparisons; Save30/RNG/NVS unchanged. Native simulated input/radio only.\n",runtimeui::cases,runtimeui::renderChecks);
}
