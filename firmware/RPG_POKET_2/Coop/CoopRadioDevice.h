#pragma once
#include "CoopSession.h"
#include "RadioOwner.h"
#include "CoopDiagnostics.h"
#include <esp_timer.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
namespace coopradio {
struct Datagram {uint32_t generation=0;coopplatform::Mac source{};uint16_t size=0;uint8_t bytes[coop::kMaxPacket]{};uint32_t receivedAt=0;int8_t rssi=0;uint8_t channel=0,rate=0;bool hasSignal=false;};
// Lifetime is the entire firmware, not a session. A late callback can only
// append a stale-generation item; no callback touches Session or campaign Game.
inline StaticQueue_t queueState;
inline uint8_t queueBytes[8*sizeof(Datagram)]{};
inline QueueHandle_t inbox=nullptr;
// ESP-NOW receive callbacks run in the Wi-Fi task, not an ISR. IDF's SMP
// portMUX provides the cross-core publication barrier that Xtensa std::atomic
// cannot promise here. Hold it only for scalar access, never queue/driver work.
class CallbackCounters {
 portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
 uint32_t active=0,dropped=0;
public:
 explicit CallbackCounters(uint32_t initialDrops=0):dropped(initialDrops){}
 CallbackCounters(const CallbackCounters&)=delete;CallbackCounters& operator=(const CallbackCounters&)=delete;
 uint32_t generation(){portENTER_CRITICAL(&mux);const uint32_t value=active;portEXIT_CRITICAL(&mux);return value;}
 void publish(uint32_t value){portENTER_CRITICAL(&mux);active=value;portEXIT_CRITICAL(&mux);}
 void drop(){portENTER_CRITICAL(&mux);if(dropped!=UINT32_MAX)++dropped;portEXIT_CRITICAL(&mux);}
 uint32_t drops(){portENTER_CRITICAL(&mux);const uint32_t value=dropped;portEXIT_CRITICAL(&mux);return value;}
};
static_assert(sizeof(CallbackCounters)<=64,"Callback synchronization exceeded static RAM allowance");
inline CallbackCounters callbackCounters;
inline uint32_t nextGeneration=0;
// Aggregate MAC completions describe radio activity, not application receipts.
// Driver callbacks have no application command/generation ID; never use them to
// advance Session or claim per-command delivery across cancellation/reordering.
class CallbackTelemetry {
 portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;uint32_t active=0;coopdiag::CallbackStats values{};
public:
 void publish(uint32_t token,bool clear=false){portENTER_CRITICAL(&mux);active=token;if(clear)values={};portEXIT_CRITICAL(&mux);}
 void receive(uint32_t token,bool invalid){portENTER_CRITICAL(&mux);if(active&&active==token){coopdiag::increment(values.received);if(invalid)coopdiag::increment(values.invalid);}portEXIT_CRITICAL(&mux);}
 void sent(bool broadcast,bool ok){portENTER_CRITICAL(&mux);if(active)coopdiag::increment(broadcast?(ok?values.broadcastOk:values.broadcastFail):(ok?values.unicastOk:values.unicastFail));portEXIT_CRITICAL(&mux);}
 coopdiag::CallbackStats snapshot(){portENTER_CRITICAL(&mux);auto copy=values;portEXIT_CRITICAL(&mux);return copy;}
};
static_assert(sizeof(CallbackTelemetry)<=96,"Callback telemetry exceeded bounded storage");
inline CallbackTelemetry telemetry;
inline coopdiag::Snapshot diagnostic;
struct SignalSlot {coopplatform::Mac mac{};coopdiag::Signal value{};};
inline SignalSlot signals[4]{};
inline void receiveCallback(const esp_now_recv_info_t* info,const uint8_t* bytes,int length){
 const uint32_t token=callbackCounters.generation();
 if(!token)return;
 const bool invalid=!info||!info->src_addr||!bytes||length<4||length>int(coop::kMaxPacket);
 telemetry.receive(token,invalid);if(invalid)return;
 Datagram d{};d.generation=token;d.size=uint16_t(length);d.receivedAt=uint32_t(esp_timer_get_time()/1000);
 std::memcpy(d.source.bytes,info->src_addr,6);std::memcpy(d.bytes,bytes,length);
 if(info->rx_ctrl){d.hasSignal=true;d.rssi=info->rx_ctrl->rssi;d.channel=info->rx_ctrl->channel;d.rate=info->rx_ctrl->rate;}
 if(xQueueSend(inbox,&d,0)!=pdTRUE)callbackCounters.drop();
}
inline void sendCallback(const esp_now_send_info_t* info,esp_now_send_status_t status){
 if(info&&info->des_addr)telemetry.sent((info->des_addr[0]&1)!=0,status==ESP_NOW_SEND_SUCCESS);
}
enum class OpenError:uint8_t {None,Busy,Profile,Queue,WifiMode,WifiStorage,Channel,Mac,Radio,Callback,Session};
class Device {
 coopplatform::Session* session=nullptr;bool opened=false,radioReady=false,wifiTouched=false,lease=false;
 wifi_mode_t oldMode=WIFI_MODE_NULL;bool oldAuto=false,oldConnected=false;uint8_t oldChannel=0;
 wifi_second_chan_t oldSecondary=WIFI_SECOND_CHAN_NONE;uint32_t generation=0;
 uint32_t polledAt=0,tickedAt=0,dropBase=0;bool hasTick=false;
 uint8_t peerCount=0,nextEvict=0;coopplatform::Mac peerCache[8]{};
 static bool send(void* context,const coopplatform::Mac& destination,const uint8_t* bytes,size_t size){
  auto& self=*static_cast<Device*>(context);
  if(!self.opened||owner!=Owner::Training||!bytes||!size||size>coop::kMaxPacket)return false;
  coopplatform::admission::Header header{};
  if(coopplatform::admission::decode(bytes,size,header))coopdiag::increment(diagnostic.txAdmission[unsigned(header.type)]);
  if(!esp_now_is_peer_exist(destination.bytes)){
   // Bound ESP-NOW peer records even under unsolicited admission traffic.
   if(self.peerCount>=8){
    bool evicted=false;const auto* keep=self.session?self.session->pendingPeer():nullptr;
    for(unsigned i=0;i<8;++i){const uint8_t slot=(self.nextEvict+i)%8;
     if(self.peerCache[slot].bytes[0]==255||(keep&&coopplatform::sameMac(self.peerCache[slot],keep->mac)))continue;
     diagnostic.peerError=esp_now_del_peer(self.peerCache[slot].bytes);if(diagnostic.peerError!=ESP_OK){coopdiag::increment(diagnostic.apiFail);return false;}
     self.peerCache[slot]=self.peerCache[--self.peerCount];self.nextEvict=uint8_t((slot+1)%8);evicted=true;break;
    }
    if(!evicted)return false;
   }
   esp_now_peer_info_t peer{};std::memcpy(peer.peer_addr,destination.bytes,6);peer.channel=6;peer.ifidx=WIFI_IF_STA;peer.encrypt=false;
   diagnostic.peerError=esp_now_add_peer(&peer);if(diagnostic.peerError!=ESP_OK){coopdiag::increment(diagnostic.apiFail);return false;}self.peerCache[self.peerCount++]=destination;
  }
  diagnostic.sendError=esp_now_send(destination.bytes,bytes,size);
  coopdiag::increment(diagnostic.sendError==ESP_OK?diagnostic.apiOk:diagnostic.apiFail);
  return diagnostic.sendError==ESP_OK;
 }
 bool restoreWifi(){
  if(!wifiTouched)return true;bool ok=true;WiFi.setAutoReconnect(oldAuto);
  // This repository/core starts with WiFiGeneric::_persistent=true and has no
  // other persistent() caller. Temporary changes run with RAM storage; never
  // call resumeConnection()/connectNetwork(), which write app NVS/preferences.
  if(oldMode==WIFI_MODE_NULL){if(!WiFi.mode(WIFI_MODE_NULL))ok=false;}
  else {
   if(oldChannel&&esp_wifi_set_channel(oldChannel,oldSecondary)!=ESP_OK)ok=false;
   if(oldConnected&&!WiFi.reconnect())ok=false;
   if(esp_wifi_set_storage(WIFI_STORAGE_FLASH)!=ESP_OK)ok=false;
  }
  WiFi.setAutoReconnect(oldAuto);WiFi.persistent(true);wifiTouched=false;return ok;
 }
 OpenError fail(OpenError error){lastError=error;close();return error;}
void poll(uint32_t now){
  polledAt=now;uint8_t ch=0;wifi_second_chan_t secondary=WIFI_SECOND_CHAN_NONE;wifi_ps_type_t ps=WIFI_PS_NONE;wifi_mode_t mode=WIFI_MODE_NULL;int8_t power=0;uint8_t protocol=0;
  diagnostic.channelError=esp_wifi_get_channel(&ch,&secondary);diagnostic.psError=esp_wifi_get_ps(&ps);
  diagnostic.powerError=esp_wifi_get_max_tx_power(&power);diagnostic.protocolError=esp_wifi_get_protocol(WIFI_IF_STA,&protocol);diagnostic.modeError=esp_wifi_get_mode(&mode);
  diagnostic.channel=ch;diagnostic.secondary=uint8_t(secondary);diagnostic.ps=uint8_t(ps);diagnostic.powerQuarterDbm=power;diagnostic.protocol=protocol;diagnostic.mode=uint8_t(mode);diagnostic.connected=WiFi.status()==WL_CONNECTED;
  if(diagnostic.channelError==ESP_OK&&ch!=6)coopdiag::increment(diagnostic.channelDrifts);
 }
 void application(uint32_t now){
  if(!session)return;diagnostic.state=uint8_t(session->state());
  if(session->state()==coopplatform::SessionState::Ended){diagnostic.lastEnd=uint8_t(session->endReason());diagnostic.frozen=true;return;}
  if(!session->training().active()){diagnostic.awaitingAck=diagnostic.pending=diagnostic.paused=false;diagnostic.rxAge=diagnostic.stallAge=0;return;}
  diagnostic.frozen=false;const auto& e=session->training().endpoint;
  diagnostic.ackAccepted=e.acceptedAcks;diagnostic.retries=e.retries;diagnostic.snapshots=e.snapshots;diagnostic.outgoingDrops=e.outgoing.overflow;
  diagnostic.awaitingAck=e.awaitingAck;diagnostic.pending=e.pending;diagnostic.paused=e.paused;
  diagnostic.rxAge=session->peerAge(now);diagnostic.stallAge=session->progressAge(now);
 }
 void signal(const Datagram& d){
  if(!d.hasSignal)return;
  const auto* peer=session->pendingPeer();if(peer&&!coopplatform::sameMac(peer->mac,d.source))return;
  unsigned slot=4;for(unsigned i=0;i<4;++i)if(signals[i].value.valid&&coopplatform::sameMac(signals[i].mac,d.source)){slot=i;break;}
  if(slot==4)for(unsigned i=0;i<4;++i)if(!signals[i].value.valid){slot=i;break;}
  if(slot==4){slot=0;for(unsigned i=1;i<4;++i)if(uint32_t(d.receivedAt-signals[i].value.at)>uint32_t(d.receivedAt-signals[slot].value.at))slot=i;}
  if(!coopplatform::sameMac(signals[slot].mac,d.source)){signals[slot]={};signals[slot].mac=d.source;}
  signals[slot].value.observe(d.rssi,d.channel,d.rate,d.receivedAt);
 }
public:
 coopdiag::Snapshot snapshot(uint32_t now)const{
  auto result=diagnostic;result.now=now;result.callbacks=telemetry.snapshot();result.queueDrops=callbackCounters.drops()-dropBase;result.stale=stalePackets;
  const auto* peer=session?session->pendingPeer():nullptr;
  result.selectedPeer=peer!=nullptr;
  const auto* discovered=session?session->peer(0):nullptr;
  const coopplatform::Mac* match=peer?&peer->mac:discovered?&discovered->mac:nullptr;
  for(const auto& s:signals)if(s.value.valid&&(!match||coopplatform::sameMac(s.mac,*match))){
   if(!result.signal.valid||uint32_t(now-s.value.at)<uint32_t(now-result.signal.at))result.signal=s.value;
  }
  return result;
 }
 // Rotate the callback queue epoch without reconfiguring Wi-Fi. Tokens remain
 // the final fence for a driver-delayed frame delivered after this boundary.
 bool returnToLobby(uint32_t now){
  if(!opened||!session||nextGeneration==UINT32_MAX)return false;
  callbackCounters.publish(0);telemetry.publish(0);generation=++nextGeneration;
  for(auto& s:signals)s={};callbackCounters.publish(generation);telemetry.publish(generation);
  const bool result=session->returnToLobby(now);application(now);return result;
 }
 OpenError lastError=OpenError::None;uint32_t stalePackets=0;bool restoreOk=true;
 bool active()const{return opened;}
 OpenError open(coopplatform::Session& target,const rpg::Game& game,bool blocked,uint64_t entropy,uint32_t now){
  if(opened||blocked||owner!=Owner::None)return lastError=OpenError::Busy;
  if(!entropy||!coopplatform::trainingEligible(game))return lastError=OpenError::Profile;
  const auto mode=WiFi.getMode();if(mode!=WIFI_MODE_NULL&&mode!=WIFI_MODE_STA)return lastError=OpenError::WifiMode;
  if(nextGeneration==UINT32_MAX||!acquire(Owner::Training))return lastError=OpenError::Busy;
  lease=true;
  if(!inbox)inbox=xQueueCreateStatic(8,sizeof(Datagram),queueBytes,&queueState);
  if(!inbox)return fail(OpenError::Queue);
  diagnostic={};for(auto& s:signals)s={};dropBase=callbackCounters.drops();hasTick=false;
  oldMode=mode;oldAuto=WiFi.getAutoReconnect();oldConnected=WiFi.status()==WL_CONNECTED;oldChannel=0;
  if(mode!=WIFI_MODE_NULL){if(esp_wifi_get_channel(&oldChannel,&oldSecondary)!=ESP_OK)return fail(OpenError::Channel);if(esp_wifi_set_storage(WIFI_STORAGE_RAM)!=ESP_OK)return fail(OpenError::WifiStorage);}
  wifiTouched=true;WiFi.setAutoReconnect(false);
  // For a previously disabled radio, initialize directly into RAM storage.
  WiFi.persistent(false);const bool started=WiFi.mode(WIFI_STA);WiFi.persistent(true);
  if(!started)return fail(OpenError::WifiMode);
  if(esp_wifi_set_storage(WIFI_STORAGE_RAM)!=ESP_OK)return fail(OpenError::WifiStorage);
  diagnostic.disconnectResult=WiFi.disconnect(false,false,100)?0:1; // Observe; never erase credentials.
  if(esp_wifi_set_channel(6,WIFI_SECOND_CHAN_NONE)!=ESP_OK)return fail(OpenError::Channel);
  coopplatform::Mac own{};if(esp_wifi_get_mac(WIFI_IF_STA,own.bytes)!=ESP_OK||!coopplatform::usableMac(own))return fail(OpenError::Mac);
  diagnostic.initError=esp_now_init();if(diagnostic.initError!=ESP_OK)return fail(OpenError::Radio);radioReady=true;
  generation=++nextGeneration;callbackCounters.publish(generation);telemetry.publish(generation,true);
  diagnostic.callbackError=esp_now_register_recv_cb(receiveCallback);if(diagnostic.callbackError!=ESP_OK)return fail(OpenError::Callback);
  diagnostic.callbackError=esp_now_register_send_cb(sendCallback);if(diagnostic.callbackError!=ESP_OK)return fail(OpenError::Callback);
  opened=true;peerCount=nextEvict=0;session=&target;target.setSender(send,this);
  if(!target.open(game,own,entropy,now))return fail(OpenError::Session);
  poll(now);lastError=OpenError::None;return lastError;
 }
 void tick(uint32_t now){
  if(!opened||!session)return;
  if(hasTick)diagnostic.maxTickGap=std::max(diagnostic.maxTickGap,uint32_t(now-tickedAt));hasTick=true;tickedAt=now;
  if(uint32_t(now-polledAt)>=1000)poll(now);application(now);
  Datagram d{};for(unsigned i=0;i<8&&xQueueReceive(inbox,&d,0)==pdTRUE;++i){
   if(d.generation!=generation){coopdiag::increment(stalePackets);continue;}
   diagnostic.maxQueueAge=std::max(diagnostic.maxQueueAge,coopdiag::elapsed(now,d.receivedAt));
   if(session->receive(d.source,d.bytes,d.size,now)){
    coopdiag::increment(diagnostic.accepted);coopplatform::admission::Header header{};
    if(coopplatform::admission::decode(d.bytes,d.size,header))coopdiag::increment(diagnostic.rxAdmission[unsigned(header.type)]);
    signal(d);
   }else coopdiag::increment(diagnostic.rejected);
   application(now);
  }
  application(now);session->tick(now);application(now);
 }
 void close(){
  // Do not release/alter another owner's radio when open() never acquired it.
  if(!lease||owner!=Owner::Training)return;
  callbackCounters.publish(0);telemetry.publish(0);opened=false;
  if(radioReady){const auto rx=esp_now_unregister_recv_cb(),tx=esp_now_unregister_send_cb(),deinit=esp_now_deinit();diagnostic.teardownError=rx!=ESP_OK?rx:tx!=ESP_OK?tx:deinit;radioReady=false;}
  if(session){session->setSender(nullptr,nullptr);session=nullptr;}
  restoreOk=restoreWifi();generation=0;peerCount=0;release(Owner::Training);lease=false;
 }
};
}
