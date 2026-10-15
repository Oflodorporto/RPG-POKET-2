// Standalone native Device diagnostics tests. Driver calls use the existing fake
// radio; callback -> bounded queue -> real Session remains the production path.
#include <esp_now.h>
#include <cassert>
struct RecordedSend {uint8_t destination[6]{},bytes[240]{};size_t size=0;} recordedSend;
static int recordRadioSend(const uint8_t* destination,const uint8_t* bytes,size_t size){
 assert(size<=sizeof(recordedSend.bytes));recordedSend.size=size;
 std::memcpy(recordedSend.destination,destination,6);std::memcpy(recordedSend.bytes,bytes,size);
 return esp_now_send(destination,bytes,size);
}
#define esp_now_send recordRadioSend
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main inherited_radio_main
#include <coop_training_radio.cpp>
#undef main
#pragma GCC diagnostic pop
#undef esp_now_send

namespace devicediag {
static unsigned scenarios=0;
constexpr Mac remote{{2,0,0,0,0,2}},stranger{{2,0,0,0,0,3}};
static admission::Packet control(admission::Type type,uint64_t room=0,uint64_t join=0,const coop::Actor* profile=nullptr,uint32_t value=0){
 admission::Packet packet{};auto writer=admission::begin(packet,type,room,join);
 if(type==admission::Type::Beacon)writer.u8(1);
 if(profile)coop::writeActor(writer,*profile);
 if(type==admission::Type::Ready)writer.u32(value);
 CHECK(admission::finish(packet,writer));return packet;
}
struct Fixture {
 rpg::Game game=hero(),before=game;Session session;coopradio::Device device;uint32_t now;
 explicit Fixture(uint32_t start=100):now(start){
  fake::reset();CHECK(device.open(session,game,false,777,now)==coopradio::OpenError::None);device.tick(now);
 }
 ~Fixture(){session.leave(now);device.close();CHECK(!std::memcmp(&game,&before,sizeof(game)));CHECK(game.randomState==before.randomState);CHECK(!fake::nvsWrites&&!fake::erases);CHECK(!fakeCriticalDepth&&fakeCriticalEntries==fakeCriticalExits);}
 void queue(const Mac& source,const uint8_t* bytes,size_t size,uint32_t at,const wifi_pkt_rx_ctrl_t* rx=nullptr){
  fake::microseconds=uint64_t(at)*1000;esp_now_recv_info_t info{source.bytes,nullptr,rx};receiver(&info,bytes,int(size));
 }
 template<class Packet> void queue(const Packet& packet,uint32_t at,const wifi_pkt_rx_ctrl_t* rx=nullptr,const Mac& source=remote){queue(source,packet.bytes,packet.size,at,rx);}
 void tick(uint32_t at){now=at;device.tick(now);}
 coopdiag::Snapshot snapshot()const{return device.snapshot(now);}
 coop::Packet ack(bool correct=true){
  const auto& endpoint=session.training().endpoint;coop::Packet packet{};
  auto writer=coop::begin(packet,coop::Type::Ack,1,session.sessionToken(),endpoint.state.revision);
  writer.u32(coop::stateHash(endpoint.state)+(correct?0:1));CHECK(coop::finish(packet,writer));return packet;
 }
 void starting(){
  CHECK(session.host(now));coop::Actor guest{};CHECK(coop::playerProfile(game,1,guest));
  auto join=control(admission::Type::Join,session.sessionToken(),456,&guest);queue(join,now);tick(now);
  CHECK(session.state()==SessionState::AwaitingAccept);CHECK(session.accept(now));
  admission::Header header{};CHECK(admission::decode(recordedSend.bytes,recordedSend.size,header)&&header.type==admission::Type::Start);
  coop::Reader reader{recordedSend.bytes+admission::kHeaderBytes,header.payload};auto host=coop::readActor(reader);const auto seed=reader.u32();CHECK(reader.ok);
  coop::State state{};CHECK(admittedState(state,host,guest,seed));
  auto ready=control(admission::Type::Ready,session.sessionToken(),session.joinToken(),nullptr,coop::stateHash(state));queue(ready,now);tick(now);
  CHECK(session.state()==SessionState::Starting&&session.training().active()&&session.training().endpoint.awaitingAck);
 }
};
static void metadataAndBinding(){
 Fixture f;coop::Actor host{};CHECK(coop::playerProfile(f.game,0,host));auto beacon=control(admission::Type::Beacon,123,0,&host);
 wifi_pkt_rx_ctrl_t rx{-55,6,4};f.queue(beacon,100,&rx);rx={-1,11,7};beacon.bytes[0]^=0xff;
 CHECK(!f.session.peer(0)&&f.snapshot().accepted==0&&f.snapshot().callbacks.received==1);
 f.tick(150);auto snap=f.snapshot();CHECK(f.session.peer(0)&&snap.accepted==1&&snap.rxAdmission[unsigned(admission::Type::Beacon)]==1);
 CHECK(snap.signal.valid&&snap.signal.last==-55&&snap.signal.channel==6&&snap.signal.rate==4&&snap.signal.at==100&&snap.signal.age(150)==50);
 beacon.bytes[0]^=0xff;f.queue(beacon,160);f.tick(170);snap=f.snapshot();CHECK(snap.accepted==2&&snap.signal.samples==1&&snap.signal.at==100);
 uint8_t bad[4]={1,2,3,4};f.queue(remote,bad,sizeof(bad),180,&rx);f.tick(190);snap=f.snapshot();CHECK(snap.rejected==1&&snap.signal.last==-55&&snap.signal.samples==1);
 CHECK(f.session.join(0,200));auto probe=control(admission::Type::Probe);f.queue(probe,210,&rx,stranger);f.tick(220);
 snap=f.snapshot();CHECK(snap.selectedPeer&&snap.accepted==3&&snap.signal.last==-55&&snap.signal.at==100);
 rx={-67,6,3};f.queue(probe,230,&rx);f.tick(240);snap=f.snapshot();CHECK(snap.accepted==4&&snap.rxAdmission[unsigned(admission::Type::Probe)]==2);
 CHECK(snap.signal.last==-67&&snap.signal.minimum==-67&&snap.signal.maximum==-55&&snap.signal.samples==2&&snap.signal.bars(240)==3);
 auto sessionBefore=f.session;const int mutations=fake::mutations;for(unsigned i=0;i<20;++i){auto copy=f.device.snapshot(240+i);CHECK(copy.signal.at==230&&copy.selectedPeer);}
 CHECK(!std::memcmp(&f.session,&sessionBefore,sizeof(sessionBefore))&&fake::mutations==mutations);++scenarios;
 std::puts("PASS: RX metadata and bytes copied at callback time; null rx_ctrl preserves prior signal; rejected traffic and accepted stranger Probe cannot replace selected-peer RSSI; snapshot is pure");
}
static void invalidAndQueue(){
 Fixture f;auto probe=control(admission::Type::Probe);esp_now_recv_info_t info{remote.bytes};
 receiver(nullptr,probe.bytes,probe.size);receiver(&info,nullptr,4);receiver(&info,probe.bytes,-1);receiver(&info,probe.bytes,241);esp_now_recv_info_t empty{};receiver(&empty,probe.bytes,probe.size);
 auto snap=f.snapshot();CHECK(snap.callbacks.received==5&&snap.callbacks.invalid==5&&snap.accepted==0&&snap.rejected==0&&snap.queueDrops==0);
 for(unsigned i=0;i<9;++i)f.queue(probe,100+i);snap=f.snapshot();CHECK(snap.callbacks.received==14&&snap.callbacks.invalid==5&&snap.queueDrops==1&&snap.accepted==0);
 f.tick(120);snap=f.snapshot();CHECK(snap.accepted==8&&snap.rejected==0&&snap.maxQueueAge==20&&snap.maxTickGap==20);
 CHECK(!snap.signal.valid&&snap.rxAdmission[unsigned(admission::Type::Probe)]==8);++scenarios;
 std::puts("PASS: invalid callback/accepted/rejected counters remain distinct; eight copied datagrams drain once, ninth records a bounded queue drop and oldest queue age");
}
static void applicationAckAndCallbacks(){
 Fixture f;f.starting();auto snap=f.snapshot();CHECK(snap.apiOk>0&&!snap.apiFail&&!snap.ackAccepted&&snap.awaitingAck);
 CHECK(!snap.callbacks.unicastOk&&!snap.callbacks.unicastFail&&!snap.callbacks.broadcastOk&&!snap.callbacks.broadcastFail);
 const auto apiOk=snap.apiOk;esp_now_send_info_t info{remote.bytes};
 senderCallback(&info,ESP_NOW_SEND_SUCCESS);senderCallback(&info,ESP_NOW_SEND_FAIL);uint8_t broadcast[6]={255,255,255,255,255,255};info.des_addr=broadcast;
 senderCallback(&info,ESP_NOW_SEND_SUCCESS);senderCallback(&info,ESP_NOW_SEND_FAIL);senderCallback(nullptr,ESP_NOW_SEND_SUCCESS);info.des_addr=nullptr;senderCallback(&info,ESP_NOW_SEND_SUCCESS);
 snap=f.snapshot();CHECK(snap.apiOk==apiOk&&!snap.ackAccepted&&snap.awaitingAck&&f.session.state()==SessionState::Starting);
 CHECK(snap.callbacks.unicastOk==1&&snap.callbacks.unicastFail==1&&snap.callbacks.broadcastOk==1&&snap.callbacks.broadcastFail==1);
 auto wrong=f.ack(false);f.queue(wrong,110);f.tick(110);CHECK(!f.snapshot().ackAccepted&&f.snapshot().awaitingAck&&f.snapshot().rejected==1);
 auto valid=f.ack();f.queue(valid,120);CHECK(!f.snapshot().ackAccepted);f.tick(120);snap=f.snapshot();CHECK(snap.ackAccepted==1&&!snap.awaitingAck&&f.session.state()==SessionState::Playing);
 CHECK(f.session.training().endpoint.submit(coop::Action::Guard,0,130)==coop::Error::None);f.tick(130);snap=f.snapshot();CHECK(snap.awaitingAck&&snap.ackAccepted==1&&snap.apiOk>apiOk);
 info.des_addr=remote.bytes;senderCallback(&info,ESP_NOW_SEND_SUCCESS);CHECK(f.snapshot().awaitingAck&&f.snapshot().ackAccepted==1);
 valid=f.ack();f.queue(valid,140);f.tick(140);CHECK(f.snapshot().ackAccepted==2&&!f.snapshot().awaitingAck);
 f.tick(1940);CHECK(f.snapshot().paused);f.tick(10140);snap=f.snapshot();CHECK(snap.frozen&&snap.lastEnd==uint8_t(SessionEnd::LinkLost));
 const auto mutations=fake::mutations;CHECK(f.device.returnToLobby(10141));f.now=10141;snap=f.snapshot();
 CHECK(f.session.state()==SessionState::Browsing&&f.device.active()&&fake::mutations==mutations);
 CHECK(!snap.pending&&!snap.awaitingAck&&!snap.paused&&!snap.rxAge&&!snap.stallAge&&!snap.selectedPeer&&!snap.signal.valid);
 CHECK(snap.lastEnd==uint8_t(SessionEnd::LinkLost)&&snap.ackAccepted==2);++scenarios;
 std::puts("PASS: API enqueue, unicast/broadcast MAC callbacks and validated application ACKs are separate; only valid app ACK clears wait; lobby clears live flags/ages while retaining failure evidence");
}
static void apiFailures(){
 for(int failure:{7,8}){
  Fixture f;const auto initial=f.snapshot();fake::fail=failure;
  if(failure==8){CHECK(f.session.host(f.now));}
  else{
   CHECK(f.session.host(f.now));coop::Actor guest{};CHECK(coop::playerProfile(f.game,1,guest));auto join=control(admission::Type::Join,f.session.sessionToken(),345,&guest);f.queue(join,f.now);f.tick(f.now);
  }
  const auto snap=f.snapshot();CHECK(snap.apiFail>initial.apiFail&&!snap.ackAccepted&&!snap.callbacks.unicastOk&&!snap.callbacks.broadcastOk);
  if(failure==7)CHECK(snap.peerError==-1);else CHECK(snap.sendError==-1&&snap.txAdmission[unsigned(admission::Type::Beacon)]==1);
  CHECK(f.session.notice()==SessionNotice::SendFailed);fake::fail=0;++scenarios;
 }
 std::puts("PASS: peer-allocation and send API failures record failure/error without inventing MAC completion or application confirmation");
}
static void queueCancellation(){
 Fixture f;auto probe=control(admission::Type::Probe);wifi_pkt_rx_ctrl_t rx{-40,6,1};
 const auto old=coopradio::callbackCounters.generation();for(unsigned i=0;i<8;++i)f.queue(probe,100+i,&rx);
 const auto mutations=fake::mutations;CHECK(f.device.returnToLobby(110));CHECK(coopradio::callbackCounters.generation()!=old&&fake::mutations==mutations&&f.device.active());
 f.tick(120);auto snap=f.snapshot();CHECK(snap.stale==8&&!snap.accepted&&!snap.rejected&&!snap.signal.valid&&snap.maxQueueAge==0);
 f.queue(probe,130,&rx);f.tick(140);snap=f.snapshot();CHECK(snap.accepted==1&&snap.signal.valid&&snap.signal.at==130&&snap.maxQueueAge==10);
 const auto generation=coopradio::nextGeneration,active=coopradio::callbackCounters.generation();const auto before=f.session;
 coopradio::nextGeneration=UINT32_MAX;CHECK(!f.device.returnToLobby(150));CHECK(!std::memcmp(&before,&f.session,sizeof(before))&&coopradio::callbackCounters.generation()==active);coopradio::nextGeneration=generation;++scenarios;
 std::puts("PASS: cancel rotates the callback epoch without RF reconfiguration; all old queued frames are stale, fresh frames work and exhausted generation fails without mutation");
}
static void callbackAcrossCancellation(){
 Fixture f;const auto old=coopradio::callbackCounters.generation();Pause paused;paused.info.src_addr=remote.bytes;pauseState=&paused;fakeAfterCritical=afterUnlock;
 pthread_t thread;CHECK(!pthread_create(&thread,nullptr,delayedCallback,&paused));CHECK(!pthread_mutex_lock(&paused.mutex));while(!paused.captured)assert(!pthread_cond_wait(&paused.changed,&paused.mutex));CHECK(!pthread_mutex_unlock(&paused.mutex));
 const auto mutations=fake::mutations;CHECK(f.device.returnToLobby(150));CHECK(coopradio::callbackCounters.generation()!=old&&fake::mutations==mutations);
 CHECK(!pthread_mutex_lock(&paused.mutex));paused.released=true;CHECK(!pthread_cond_broadcast(&paused.changed));CHECK(!pthread_mutex_unlock(&paused.mutex));CHECK(!pthread_join(thread,nullptr));fakeAfterCritical=nullptr;pauseState=nullptr;
 f.tick(160);auto snap=f.snapshot();CHECK(snap.stale==1&&!snap.callbacks.received&&!snap.accepted&&!snap.rejected&&!snap.signal.valid&&f.session.state()==SessionState::Browsing);
 CHECK(!pthread_cond_destroy(&paused.changed));CHECK(!pthread_mutex_destroy(&paused.mutex));++scenarios;
 std::puts("PASS: real two-thread callback paused after epoch capture cannot update new-epoch telemetry or Session after in-place cancellation");
}
static void gettersAreReadOnly(){
 Fixture f;const auto mutations=fake::mutations;const auto initial=f.snapshot();CHECK(initial.channel==6&&initial.ps==WIFI_PS_MIN_MODEM&&initial.powerQuarterDbm==80&&initial.protocol==7);
 const auto mode=fake::mode;const auto storage=fake::storage;const auto ps=fake::ps;const auto power=fake::power;const auto channel=fake::channel;
 fake::queryError=-77;f.tick(1100);auto snap=f.snapshot();CHECK(snap.channelError==-77&&snap.psError==-77&&snap.powerError==-77&&snap.protocolError==-77&&snap.modeError==-77);
 CHECK(!snap.channel&&!snap.powerQuarterDbm&&!snap.protocol&&!snap.channelDrifts);
 CHECK(fake::mutations==mutations&&fake::mode==mode&&fake::storage==storage&&fake::ps==ps&&fake::power==power&&fake::channel==channel);
 fake::queryError=0;fake::channel=11;f.tick(2100);snap=f.snapshot();CHECK(snap.channel==11&&snap.channelDrifts==1&&snap.channelError==ESP_OK&&fake::channel==11&&fake::mutations==mutations);
 f.tick(3100);CHECK(f.snapshot().channelDrifts==2&&fake::mutations==mutations);++scenarios;
 std::puts("PASS: getter failures expose explicit errors and unknown values; channel drift is observed without changing channel, power, power-save, mode or storage");
}
static void boundedAges(){
 struct Age {uint32_t now,at,want;};
 const Age ages[]={{100,100,0},{100,101,0},{100,116,0},{100,117,UINT32_MAX},{10,UINT32_MAX-5,16},{100,uint32_t(100-0x7fffffffu),0x7fffffffu},{100,uint32_t(100-0x80000000u),UINT32_MAX},{100,uint32_t(100-0x80000001u),UINT32_MAX}};
 for(const auto age:ages){
  CHECK(coopdiag::elapsed(age.now,age.at)==age.want);coopdiag::Signal signal;signal.observe(-55,6,4,age.at);
  CHECK(signal.age(age.now)==age.want&&signal.fresh(age.now)==(age.want<2800));
  Fixture f(age.now);auto probe=control(admission::Type::Probe);wifi_pkt_rx_ctrl_t rx{-55,6,4};f.queue(probe,age.at,&rx);f.tick(age.now);
  auto snap=f.snapshot();CHECK(snap.accepted==1&&snap.maxQueueAge==age.want&&snap.signal.age(age.now)==age.want);++scenarios;
 }
 std::puts("PASS: callback queue and RSSI age share exact wrap/half-range rules; only 1..16ms future skew clamps to zero, 17ms/ambiguous ages become UINT32_MAX");
}
}
int main(){
 std::setvbuf(stdout,nullptr,_IONBF,0);using namespace devicediag;
 metadataAndBinding();invalidAndQueue();applicationAckAndCallbacks();apiFailures();queueCancellation();callbackAcrossCancellation();gettersAreReadOnly();boundedAges();
 CHECK(!allocations);std::printf("PASS: %u device diagnostics scenarios, %u checks; unchanged campaign/RNG, no fake NVS writes, zero C++ allocations; native evidence only\n",scenarios,checks);
}
