// Actual controller/runtime touch-queue regression; fixture generated outside checkout.
#include "coop_controller_fixture.generated.h"

namespace lossdiag {
using coopplatform::Session;
using coopplatform::SessionState;
using coopplatform::SessionEnd;
using coopplatform::Mac;
struct Frame { unsigned from=0; Mac destination{}; size_t size=0; uint8_t bytes[coop::kMaxPacket]{}; };
struct Pair {
 Session peer;
 struct Port {Pair* pair;unsigned from;} ports[2];
 coop::Queue<Frame,512> frames;
 Mac macs[2]={{{2,0,0,0,0,1}},{{2,0,0,0,0,2}}};
 bool partition=false;
 unsigned dropped=0;
 Pair(bool localHost){
  setup(0,0,0);enterGuild();press(175,238);assert(coopIsOpen());
  ports[0]={this,0};ports[1]={this,1};
  coopSession.setSender(send,&ports[0]);peer.setSender(send,&ports[1]);
  assert(peer.open(game,macs[1],0xabcdef,now));flush();
  if(localHost){
   coopClick(50,195);assert(coopSession.state()==SessionState::Hosting);flush();
   assert(peer.peer(0));assert(peer.join(0,now));flush();assert(coopSession.state()==SessionState::AwaitingAccept);
   coopClick(50,195);flush();
  }else{
   assert(peer.host(now));flush();assert(coopSession.peer(0));coopClick(200,60);flush();
   assert(peer.state()==SessionState::AwaitingAccept);assert(peer.accept(now));flush();
  }
  assert(coopSession.state()==SessionState::Playing&&peer.state()==SessionState::Playing);
 }
 static bool send(void* context,const Mac& destination,const uint8_t* data,size_t size){
  auto& port=*static_cast<Port*>(context);auto& pair=*port.pair;
  if(pair.partition){++pair.dropped;return true;}
  Frame frame{};frame.from=port.from;frame.destination=destination;frame.size=size;
  assert(size<=sizeof(frame.bytes));std::memcpy(frame.bytes,data,size);assert(pair.frames.push(frame));return true;
 }
 void flush(){
  Frame frame{};unsigned count=0;
  while(frames.pop(frame)){
   assert(++count<10000);unsigned target=1-frame.from;
   if(!coopplatform::sameMac(frame.destination,macs[target])&&frame.destination.bytes[0]!=255)continue;
   (target?peer:coopSession).receive(macs[frame.from],frame.bytes,frame.size,now);
  }
 }
 void step(uint32_t elapsed){now+=elapsed;tickCoopTraining(now);peer.tick(now);flush();}
 void cut(){partition=true;for(unsigned i=0;i<41;++i)step(250);
  assert(coopSession.state()==SessionState::Ended&&coopSession.endReason()==SessionEnd::LinkLost);
  assert(peer.state()==SessionState::Ended&&peer.endReason()==SessionEnd::LinkLost);
  assert(coopIsOpen()&&view.page==Page::CoopTraining&&!coopClosing);
 }
 void recoveredStillEnded(){partition=false;for(unsigned i=0;i<8;++i)step(250);
  assert(coopSession.state()==SessionState::Ended&&peer.state()==SessionState::Ended);
 }
 void checkExit(const Game& before,const std::map<std::string,std::vector<uint8_t>>& blobs){
  coopClick(280,10);assert(coopClosing&&coopIsOpen());
  now+=349;tickCoopTraining(now);assert(coopIsOpen());++now;tickCoopTraining(now);
  assert(!coopIsOpen()&&view.page==Page::Guild&&coopradio::owner==coopradio::Owner::None);
  assert(!pendingTouch&&!gate.armed);same(game,before);assert(nvs.blobs==blobs);assert(!fake::nvsWrites&&!fake::erases);
  portrait(true,true,120,292);assert(view.page==Page::Guild);press(120,292);assert(view.page==Page::Village);
  enterGuild();press(175,238);assert(coopIsOpen()&&coopSession.state()==SessionState::Browsing);
  coopClick(280,10);now+=350;tickCoopTraining(now);assert(!coopIsOpen());same(game,before);assert(nvs.blobs==blobs);
 }
};
}

void candidateCases();
int main(){
 std::setvbuf(stdout,nullptr,_IONBF,0);
 using namespace lossdiag;
 for(bool host:{true,false}){
  {
   Pair pair(host);auto before=game;auto blobs=nvs.blobs;
   pair.partition=true;for(unsigned i=0;i<12;++i)pair.step(250);
   assert(coopSession.state()==SessionState::Playing&&coopSession.training().endpoint.paused);
   pair.partition=false;for(unsigned i=0;i<4;++i)pair.step(250);
   assert(coopSession.state()==SessionState::Playing&&!coopSession.training().endpoint.paused);
   pair.checkExit(before,blobs);
   std::printf("PASS %s: 3-second loss resumes; actual controller Exit restores portrait and re-entry\n",host?"host":"guest");
  }
  for(unsigned gesture=0;gesture<4;++gesture){
   Pair pair(host);auto before=game;auto blobs=nvs.blobs;
   if(gesture==1){coopRelease();coopSample(true,true,280,10);}
   if(gesture==2){coopRelease();coopSample(true,true,8,210);}
   pair.cut();pair.recoveredStillEnded();
   assert(coopview::hitTest(coopview::model(coopSession,true),280,10)==coopview::Control::Exit);
   if(gesture==1||gesture==2){coopRelease();assert(!coopClosing);}
   if(gesture==3){coopRelease();coopSample(true,true,280,10);coopSample(false,true,280,10);coopRelease();assert(!coopClosing);}
   pair.checkExit(before,blobs);
   std::printf("PASS %s gesture %u: 10-second loss stays terminal after RF restoration; fresh Exit closes in 350 ms, portrait + re-entry works, Save30/NVS/RNG unchanged\n",host?"host":"guest",gesture);
  }
 }
 candidateCases();
}

#ifndef EXPECT_COALESCED
#define EXPECT_COALESCED 1
#endif
namespace candidatediag {
using namespace lossdiag;
unsigned scenarios=10;
void captureRun(bool valid,bool down,int x,int y,unsigned count){
 auto p=coopplatform::toPortrait(x,y);
 for(unsigned sample=0;sample<count;++sample){now+=8;captureCoopTouch(valid,down,p.x,p.y,now);}
}
void finishClosed(const Game& before,const std::map<std::string,std::vector<uint8_t>>& blobs){
 assert(coopClosing&&coopIsOpen());now+=350;tickCoopTraining(now);
 assert(!coopIsOpen()&&view.page==Page::Guild&&coopradio::owner==coopradio::Owner::None);
 assert(!pendingTouch&&!gate.armed);same(game,before);assert(nvs.blobs==blobs&&!fake::nvsWrites&&!fake::erases);
 portrait(true,true,120,292);assert(view.page==Page::Guild);press(120,292);assert(view.page==Page::Village);
 enterGuild();press(175,238);assert(coopIsOpen()&&coopSession.state()==SessionState::Browsing);
 coopSample(true,true,280,10);coopRelease();assert(!coopClosing); // No carried held touch after re-entry.
 coopClick(280,10);now+=350;tickCoopTraining(now);assert(!coopIsOpen());same(game,before);assert(nvs.blobs==blobs);
}
void longTerminal(){
 for(bool host:{true,false})for(unsigned count:{17u,20u,32u,50u,64u})for(bool combined:{false,true}){
  Pair pair(host);auto before=game;auto blobs=nvs.blobs;pair.cut();auto drops=coopInput.dropped;
  for(bool down:{false,true,false}){captureRun(true,down,down?280:0,down?10:0,count);if(!combined)tickCoopTraining(now);}
  if(combined)tickCoopTraining(now);
  if(EXPECT_COALESCED){assert(coopClosing&&coopInput.dropped==drops);finishClosed(before,blobs);}
  else{assert(!coopClosing&&coopIsOpen()&&coopInput.dropped>drops);pair.checkExit(before,blobs);}
  ++scenarios;
 }
 std::printf("PASS %s: 20 terminal host/guest repeated 17/20/32/50/64-sample cases, combined and separately dispatched; safe re-entry\n",EXPECT_COALESCED?"candidate Exit succeeds":"baseline starvation reproduced");
}
void terminalFaults(){
 for(bool host:{true,false})for(unsigned fault=0;fault<6;++fault){
  Pair pair(host);auto before=game;auto blobs=nvs.blobs;pair.cut();coopRelease();
  const auto drops=coopInput.dropped,stales=coopInput.stale;
  if(fault==0){ // More than 16 genuinely distinct positions, even inside one control.
   for(unsigned sample=0;sample<20;++sample)captureRun(true,true,270+sample,10,1);
   captureRun(true,false,0,0,5);
  }else if(fault==1){ // More than 16 distinct validity edges cannot be silently dropped.
   for(unsigned sample=0;sample<20;++sample)captureRun(sample%2,true,280,10,1);
   captureRun(true,false,0,0,5);
  }else if(fault==2){ // Invalid reading breaks a held contact, even after long identical runs.
   captureRun(true,true,280,10,32);captureRun(false,true,280,10,32);captureRun(true,true,280,10,32);captureRun(true,false,0,0,32);
  }else if(fault==3){ // Drag out and back must retain both transitions and cancel.
   captureRun(true,true,280,10,32);captureRun(true,true,240,40,32);captureRun(true,true,280,10,32);captureRun(true,false,0,0,32);
  }else if(fault==4){ // Reopening changes phase and identity before captured contact dispatch.
   captureRun(true,true,280,10,32);
   assert(coopSession.open(game,pair.macs[0],0x444444,now));
   captureRun(true,false,0,0,32);
  }else{ // Retain first release timestamp, but never interpret <30 ms as a release.
   captureRun(true,true,280,10,3);captureRun(true,false,0,0,4); // 24 ms from first to last up.
  }
  tickCoopTraining(now);assert(!coopClosing&&coopIsOpen());
  if(fault<=1)assert(coopInput.dropped>drops);
  if(EXPECT_COALESCED&&fault>=2)assert(coopInput.dropped==drops);
  if(EXPECT_COALESCED&&fault==4)assert(coopInput.stale>stales);
  // The <30-ms release is still a legitimate held gesture; invalidate it before proving fresh recovery.
  if(fault==5)coopSample(false,false,0,0);
  pair.checkExit(before,blobs);++scenarios;
 }
 std::puts("PASS: 12 terminal distinct-position/error-overflow, invalid continuation, drag-out/back, stale identity and <30ms-release cases remain fail-closed with fresh Exit recovery");
}
void battleRuns(){
 for(bool host:{true,false})for(unsigned count:{17u,32u,50u,64u}){
  Pair pair(host);auto before=game;auto blobs=nvs.blobs;
  if(!host){assert(pair.peer.training().endpoint.submit(coop::Action::Guard,0,now)==coop::Error::None);pair.step(0);}
  auto& training=coopSession.training();assert(training.ui.playable(training.endpoint));
  const auto drops=training.touchDrops,inputDrops=coopInput.dropped,submitted=training.ui.submitted;
  const auto revision=training.endpoint.state.revision;
  captureRun(true,false,0,0,count);captureRun(true,true,180,210,count);captureRun(true,false,0,0,count);
  // Every render callback is capture-only; no combat mutation can precede dispatch.
  assert(training.ui.submitted==submitted&&training.endpoint.state.revision==revision);
  tickCoopTraining(now);pair.step(1);
  if(EXPECT_COALESCED){
   assert(training.ui.submitted==submitted+1&&training.endpoint.state.revision==revision+1);
   assert(training.touchDrops==drops&&coopInput.dropped==inputDrops);
   pair.step(1);assert(training.ui.submitted==submitted+1); // No duplicate command.
  }else{assert(training.ui.submitted==submitted&&training.endpoint.state.revision==revision&&training.touchDrops>drops&&coopInput.dropped>inputDrops);}
  pair.checkExit(before,blobs);++scenarios;
 }
 std::printf("PASS %s: 8 battle host/guest long Guard gestures; capture-only, command multiplicity, training+UI queue accounting and unchanged Save30\n",EXPECT_COALESCED?"candidate resolves once":"baseline overflow rejects");
}
void battleFaults(){
 for(unsigned fault=0;fault<6;++fault){
  Pair pair(true);auto before=game;auto blobs=nvs.blobs;auto& training=coopSession.training();
  // Initial release is dispatched with each sample; both implementations arm equally.
  coopRelease();const auto submitted=training.ui.submitted,drops=training.touchDrops;
  const auto revision=training.endpoint.state.revision;
  if(fault==0){for(unsigned i=0;i<20;++i)captureRun(true,true,165+i,210,1);captureRun(true,false,0,0,5);}
  if(fault==1){captureRun(true,true,180,210,32);captureRun(false,true,180,210,32);captureRun(true,true,180,210,32);captureRun(true,false,0,0,32);}
  if(fault==2){captureRun(true,true,180,210,32);captureRun(true,true,150,170,32);captureRun(true,true,180,210,32);captureRun(true,false,0,0,32);}
  if(fault==3){
   captureRun(true,true,180,210,32); // Then a real independently submitted command changes hash/revision.
   assert(training.endpoint.submit(coop::Action::Guard,0,now)==coop::Error::None);
   captureRun(true,false,0,0,32);
  }
  if(fault==4){
   captureRun(true,true,180,210,32); // Pending reliable ACK changes playable flag without changing old gesture.
   training.endpoint.awaitingAck=true;captureRun(true,false,0,0,32);
  }
  if(fault==5){
   captureRun(true,true,180,210,32); // State fingerprint changes inside one render batch.
   training.endpoint.state.rng^=0x55aa55aau;captureRun(true,false,0,0,32);
  }
  tickCoopTraining(now);assert(training.ui.submitted==submitted);
  if(fault==0||!EXPECT_COALESCED)assert(training.touchDrops>drops);
  else assert(training.touchDrops==drops);
  if(fault==3)assert(training.endpoint.state.revision==revision+1);
  else assert(training.endpoint.state.revision==revision);
  if(fault>=3&&EXPECT_COALESCED)assert(training.staleTouches>0);
  // Exit is a separate control and stays available even during the induced combat wait.
  pair.checkExit(before,blobs);++scenarios;
 }
 std::puts("PASS: 6 battle distinct-overflow/invalid/drag/revision/ACK/hash interruptions cannot dispatch a stale command");
}
void wrappedRelease(){
 for(bool host:{true,false}){
  Pair pair(host);auto before=game;auto blobs=nvs.blobs;pair.cut();
  now=UINT32_MAX-20u;
  // 15 total captures fit even the original queue. Initial up crosses wrap.
  captureRun(true,false,0,0,5);captureRun(true,true,280,10,5);captureRun(true,false,0,0,5);
  tickCoopTraining(now);assert(coopClosing);finishClosed(before,blobs);++scenarios;
 }
 std::puts("PASS: first/last release timestamps crossing uint32 clock wrap still trigger one valid Exit");
}
// Central terminal recovery uses the real runtime/controller. Terminal combat
// fixtures below are valid model states; their Session transitions remain real.
void terminalReason(Pair& pair,unsigned reason){
 if(reason==0){pair.cut();return;}
 if(reason==3){coopSession.leave(now);pair.flush();}
 else if(reason==4){pair.peer.leave(now);pair.flush();}
 else{
  auto state=coopSession.training().endpoint.state;
  if(reason==1){
   state.wave=2;coop::spawnWave(state);
   for(unsigned i=coop::kPlayerCapacity;i<coop::kActors;++i)state.actors[i].hp=0;
  }else{assert(reason==2);state.actors[0].hp=state.actors[1].hp=0;}
  coop::advance(state);assert(coop::validState(state));
  for(Session* session:{&coopSession,&pair.peer}){
   auto& endpoint=session->training().endpoint;
   endpoint.state=state;endpoint.pending=endpoint.awaitingAck=false;
  }
  pair.step(0);
 }
 const SessionEnd expected=reason==1?SessionEnd::Won:reason==2?SessionEnd::Lost:reason==3?SessionEnd::Left:SessionEnd::PeerLeft;
 assert(coopSession.state()==SessionState::Ended&&coopSession.endReason()==expected);
 assert(coopIsOpen()&&!coopClosing);
}
// The central transport-failure button now starts a clean lobby; the top SAIR
// and non-transport terminal buttons retain their portrait/350ms-exit contract.
void finishLobbyThenClosed(const Game& before,const std::map<std::string,std::vector<uint8_t>>& blobs){
 assert(coopIsOpen()&&!coopClosing&&view.page==Page::CoopTraining);
 assert(coopSession.state()==SessionState::Browsing&&!coopSession.training().active());
 assert(!coopSession.sessionToken()&&!coopSession.joinToken()&&!coopSession.pendingPeer());
 const auto diagnostic=coopruntime::view(true);assert(!diagnostic.recovering&&!diagnostic.diagnostics);
 assert(!diagnostic.link.awaitingAck&&!diagnostic.link.pending&&!diagnostic.link.paused&&!diagnostic.link.signal.valid);
 same(game,before);assert(nvs.blobs==blobs&&!fake::nvsWrites&&!fake::erases);
 coopClick(50,195);assert(coopSession.state()==SessionState::Hosting&&coopSession.sessionToken());
 coopClick(280,10);assert(coopClosing);finishClosed(before,blobs);
}
void centralTerminalButton(){
 const auto r=coopview::kTerminalReturn;
 assert(r.x==48&&r.y==180&&r.w==224&&r.h==38);
 const coopplatform::Point points[]={
  {48,180},{271,180},{48,217},{271,217}, // All inclusive corner pixels.
  {160,180},{160,217},{48,199},{271,199},{160,199}
 };
 for(bool host:{true,false}){
  for(auto point:points){
   Pair pair(host);auto before=game;auto blobs=nvs.blobs;terminalReason(pair,0);
   coopClick(point.x,point.y);finishLobbyThenClosed(before,blobs);++scenarios;
  }
  for(unsigned reason=1;reason<=4;++reason){
   Pair pair(host);auto before=game;auto blobs=nvs.blobs;terminalReason(pair,reason);
   coopClick(160,199);assert(coopClosing);finishClosed(before,blobs);++scenarios;
  }
  {
   Pair pair(host);auto before=game;auto blobs=nvs.blobs;terminalReason(pair,0);
   for(auto point:{coopplatform::Point{47,199},coopplatform::Point{272,199},coopplatform::Point{160,179},coopplatform::Point{160,218}}){
    coopClick(point.x,point.y);assert(!coopClosing&&coopIsOpen());
   }
   coopClick(160,199);finishLobbyThenClosed(before,blobs);++scenarios;
  }
 }
 std::puts("PASS: 28 central terminal runtime cases: both roles, LinkLost corners/edges/center return to clean lobby; Won/Lost/Left/PeerLeft return to Guild; exclusive outside edges, portrait/reentry and unchanged saves");
}
void centralBatchedAndHeld(){
 for(bool host:{true,false}){
  for(unsigned count:{17u,64u})for(bool combined:{false,true}){
   Pair pair(host);auto before=game;auto blobs=nvs.blobs;terminalReason(pair,0);auto drops=coopInput.dropped;
   for(bool down:{false,true,false}){
    captureRun(true,down,down?160:0,down?199:0,count);
    if(!combined)tickCoopTraining(now);
   }
   if(combined)tickCoopTraining(now);
   assert(!coopClosing&&coopInput.dropped==drops);finishLobbyThenClosed(before,blobs);++scenarios;
  }
  for(bool queued:{false,true}){
   Pair pair(host);auto before=game;auto blobs=nvs.blobs;
   if(!host){assert(pair.peer.training().endpoint.submit(coop::Action::Guard,0,now)==coop::Error::None);pair.step(0);}
   assert(coopSession.training().ui.playable(coopSession.training().endpoint));
   assert(coopview::kTerminalReturn.contains(180,210)); // Existing Guard button overlaps new recovery area.
   assert(coopview::hitTest(coopview::model(coopSession,true),180,210)==coopview::Control::None);
   coopRelease();
   if(queued){
    captureRun(true,true,180,210,17); // Captured while Playing, dispatched only after loss.
    now+=Session::kLinkTimeoutMs;tickCoopTraining(now);pair.peer.tick(now);
    assert(coopSession.state()==SessionState::Ended&&coopSession.endReason()==SessionEnd::LinkLost);
   }else{coopSample(true,true,180,210);pair.cut();}
   // The original held combat finger must never become a terminal Exit.
   captureRun(true,true,180,210,64);tickCoopTraining(now);assert(!coopClosing&&coopIsOpen()&&coopSession.state()==SessionState::Ended);
   captureRun(true,false,0,0,17);tickCoopTraining(now);assert(!coopClosing&&coopIsOpen()&&coopSession.state()==SessionState::Ended);
   coopClick(160,199);finishLobbyThenClosed(before,blobs);++scenarios;
  }
 }
 std::puts("PASS: 12 central recovery cases: 17/64-sample batched lobby return and queued/already-dispatched combat holds across LinkLost cannot auto-cancel; fresh clean touch succeeds");
}

void shortTraceDifferential(){
 uint32_t random=0x19851a7u,signature=2166136261u;
 for(unsigned trace=0;trace<4000;++trace){
  Session session;auto profile=rpg::create(0,123);Mac own{{2,0,0,0,0,1}};
  uint32_t time=trace%2?UINT32_MAX-60u:100u;
  assert(session.open(profile,own,100+trace,time));if(trace%3==0)session.leave(time);
  coopplatform::Input streamed,batched;coopview::Control first=coopview::Control::None;
  auto next=[&](){random^=random<<13;random^=random>>17;random^=random<<5;return random;};
  auto seed=next();
  for(unsigned i=0;i<16;++i){
   bool down=i>=5&&i<11;bool valid=true;int x=280,y=10;
   if(trace%5==1){x=50;y=195;}
   if(trace%5==2&&i==8){x=250;y=40;}
   if(trace%5==3&&i==8)valid=false;
   if(trace%5==4){down=(seed>>(i%16))&1;valid=((seed>>((i+7)%24))&7)!=0;x=(seed>>(i%7))%330;y=(seed>>((i+5)%9))%250;}
   auto p=coopplatform::toPortrait(x,y);time+=8;
   streamed.capture(session,valid,down,p.x,p.y,time);auto result=streamed.dispatch(session,time);
   if(first==coopview::Control::None&&result!=coopview::Control::None)first=result;
   batched.capture(session,valid,down,p.x,p.y,time);
  }
  auto result=batched.dispatch(session,time);assert(result==first&&batched.dropped==0);
  signature=(signature^unsigned(result))*16777619u;
 }
 std::printf("PASS: 4000 <=16-sample batched/streamed gesture differentials, errors/drag/positions/Browsing/Ended/clock-wrap; signature %08x\n",signature);
}

}
void candidateCases(){
 using namespace candidatediag;
 longTerminal();terminalFaults();battleRuns();battleFaults();wrappedRelease();centralTerminalButton();centralBatchedAndHeld();shortTraceDifferential();
 std::printf("PASS: %u independent controller/runtime cases on %s; instrumentation is reported by runner; no physical timing claims\n",candidatediag::scenarios,EXPECT_COALESCED?"coalescing candidate":"frozen baseline");
}
