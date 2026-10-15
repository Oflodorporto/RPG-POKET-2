// Exercise the new production UI with linkTools explicitly ON. Historical
// captures remain independent; these assertions cover the runtime additions.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main historical_renderer_main
#include "coop_training_render.cpp"
#undef main
#pragma GCC diagnostic pop
static void visibleOverlap(const ViewModel& v){
 const Rect cover{48,73,224,80};
 for(unsigned i=0;i<canvas.count;++i)for(unsigned j=i+1;j<canvas.count;++j){
  const auto a=canvas.labels[i],b=canvas.labels[j];
  const Rect ra{int16_t(a.x),int16_t(a.y),int16_t(std::strlen(a.value)*6),8},rb{int16_t(b.x),int16_t(b.y),int16_t(std::strlen(b.value)*6),8};
  auto hidden=[&](unsigned n,const Rect& r){
   // The recovery panel is an opaque fill drawn after arena text. Ignore only
   // labels fully erased by that fill, never clipped/partially visible text.
   bool old=true;for(unsigned k=0;k<=n;++k)if(!std::strncmp(canvas.labels[k].value,"Reconectando",12))old=false;
   return v.recovering&&!v.diagnostics&&old&&r.x>=cover.x&&r.y>=cover.y&&r.x+r.w<=cover.x+cover.w&&r.y+r.h<=cover.y+cover.h;
  };
  if(ra.w&&rb.w&&!hidden(i,ra)&&!hidden(j,rb)&&coopui::intersects(ra,rb)){
   std::fprintf(stderr,"Visible labels overlap: %s / %s\n",a.value,b.value);std::abort();
  }
 }
}
static void radioCapture(const std::string& out,const char* name,const ViewModel& v,const coopplatform::Training& t,const rpg::Game& a,const rpg::Game& b){
 assert(v.linkTools);const auto before=t;const auto beforeView=v;const auto ga=a,gb=b;
 for(unsigned repeat=0;repeat<2;++repeat){canvas.clear();rendering=true;draw(canvas,v,t);rendering=false;++renders;
  assert(!allocationCalls&&!std::memcmp(&before,&t,sizeof(t))&&!std::memcmp(&v,&beforeView,sizeof(v))&&!std::memcmp(&ga,&a,sizeof(a))&&!std::memcmp(&gb,&b,sizeof(b)));
  assert(canvas.has("SAIR")&&canvas.has("SEM XP/OURO"));assert(canvas.has(v.diagnostics?"VOLTAR":"INFO"));visibleOverlap(v);
  if(!repeat)previous=physical.pixels;else assert(previous==physical.pixels);
 }
 physical.save(out+"/"+name+".ppm",true);std::ofstream labels(out+"/"+name+"-labels.tsv");
 for(unsigned i=0;i<canvas.count;++i)labels<<canvas.labels[i].x<<'\t'<<canvas.labels[i].y<<'\t'<<canvas.labels[i].value<<'\n';
 std::printf("CAPTURE %s %016llx\n",name,static_cast<unsigned long long>(pixelsHash()));
}
int main(int argc,char** argv){
 assert(argc==3);std::filesystem::create_directories(argv[2]);const std::string out=argv[2];
 artMemory=static_cast<uint8_t*>(std::malloc(ART_BYTES));assert(artMemory);std::ifstream f(argv[1],std::ios::binary);assert(f);uint8_t header[16];f.read(reinterpret_cast<char*>(header),16);
 assert(!std::memcmp(header,"PKA1",4)&&rpg::get32(header,4)==ART_BYTES&&rpg::get32(header,8)==ART_CRC&&rpg::get32(header,12)==ART_COUNT);
 f.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(f.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 const auto a=profile(0),b=profile(2);auto t=training(a,b);ViewModel v;v.linkTools=true;v.hasLocal=true;v.localProfile=t.endpoint.state.actors[0];
 auto shot=[&](const char* name){radioCapture(out,name,v,t,a,b);};
 shot("discovery-no-sample");assert(canvas.has("SEM AMOSTRA"));
 v.link.signal.observe(-48,6,0,100);v.link.now=200;shot("discovery-strong");assert(canvas.has("DESCOBERTA -48dBm"));
 v.link.signal.observe(-88,6,0,200);shot("discovery-weak");
 v.link.now=3000;shot("discovery-stale");assert(canvas.has("SEM SINAL (2s)"));
 v.screen=Screen::ConfirmPeer;v.hasPeer=true;v.peerProfile=t.endpoint.state.actors[1];v.allowAccept=true;v.showDecisionCountdown=true;v.decisionSeconds=30;v.link.selectedPeer=true;v.link.now=200;shot("accept-with-signal");
 v.screen=Screen::Playing;v.showDecisionCountdown=false;shot("playing-with-signal");
 v.recovering=true;t.endpoint.paused=true;uint64_t hashes[3]{};
 for(uint8_t dots=1;dots<=3;++dots){v.dots=dots;char name[32];std::snprintf(name,sizeof(name),"recovery-dots-%u",dots);shot(name);hashes[dots-1]=pixelsHash();assert(canvas.has("CANCELAR RECONEXAO"));
  assert(hitTest(v,kRecoveryCancel.x+4,kRecoveryCancel.y+4)==Control::ReturnLobby);assert(hitTest(v,8,210)==Control::None);
 }
 assert(hashes[0]!=hashes[1]&&hashes[1]!=hashes[2]&&hashes[0]!=hashes[2]);
 v.artReady=false;shot("recovery-no-art");v.artReady=true;
 v.diagnostics=true;v.link.channel=6;v.link.ps=1;v.link.powerQuarterDbm=80;shot("diagnostics-live");
 v.link.signal={};shot("diagnostics-no-sample");
 v.link.signal.observe(INT8_MIN,14,255,0);v.link.signal.observe(0,14,255,0);v.link.now=UINT32_MAX/2;v.link.state=255;v.link.lastEnd=255;v.link.frozen=true;
 v.link.channelError=v.link.psError=v.link.powerError=v.link.sendError=v.link.peerError=INT32_MIN;v.link.disconnectResult=1;
 v.link.accepted=v.link.rejected=v.link.apiOk=v.link.apiFail=v.link.queueDrops=v.link.stale=v.link.maxQueueAge=v.link.maxTickGap=v.link.ackAccepted=v.link.retries=v.link.snapshots=v.link.outgoingDrops=v.link.rxAge=v.link.stallAge=v.link.channelDrifts=UINT32_MAX;
 v.link.callbacks={UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};for(auto& n:v.link.txAdmission)n=UINT32_MAX;for(auto& n:v.link.rxAdmission)n=UINT32_MAX;
 shot("diagnostics-extremes");
 v.diagnostics=false;v.recovering=false;v.screen=Screen::Ended;
 for(auto reason:{EndReason::LinkLost,EndReason::JoinTimeout,EndReason::Won,EndReason::Lost,EndReason::PeerLeft}){v.end=reason;char name[32];std::snprintf(name,sizeof(name),"terminal-%u",unsigned(reason));shot(name);
  const bool again=reason==EndReason::LinkLost||reason==EndReason::JoinTimeout;assert(canvas.has(again?"VOLTAR A SALA":"VOLTAR A GUILDA"));
  assert(hitTest(v,kTerminalReturn.x+5,kTerminalReturn.y+5)==(again?Control::ReturnLobby:Control::Exit));
 }
 // Every pixel maps to exactly the declared modal control, no hidden combat hit.
 v.screen=Screen::Playing;v.recovering=true;v.diagnostics=false;
 for(int y=0;y<240;++y)for(int x=0;x<320;++x){const auto expected=kExit.contains(x,y)?Control::Exit:kDiagnostics.contains(x,y)?Control::Diagnostics:kRecoveryCancel.contains(x,y)?Control::ReturnLobby:Control::None;assert(hitTest(v,x,y)==expected);}
 v.diagnostics=true;for(int y=0;y<240;++y)for(int x=0;x<320;++x)assert(hitTest(v,x,y)==(kExit.contains(x,y)?Control::Exit:kDiagnostics.contains(x,y)?Control::Diagnostics:Control::None));
 std::free(artMemory);artMemory=nullptr;std::printf("PASS: %u ON diagnostic/recovery renders, deterministic pixels, pure model/training/campaign, zero allocations, full-screen modal geometry\n",renders);return 0;
}
