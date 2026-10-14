#pragma once
#include "CoopSession.h"
#include "RadioOwner.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <atomic>
namespace coopradio {
struct Datagram {uint32_t generation=0;coopplatform::Mac source{};uint16_t size=0;uint8_t bytes[coop::kMaxPacket]{};};
// Lifetime is the entire firmware, not a session. A late callback can only
// append a stale-generation item; no callback touches Session or campaign Game.
inline StaticQueue_t queueState;
inline uint8_t queueBytes[8*sizeof(Datagram)]{};
inline QueueHandle_t inbox=nullptr;
inline std::atomic<uint32_t> activeGeneration{0},queueDrops{0};
inline uint32_t nextGeneration=0;
static_assert(std::atomic<uint32_t>::is_always_lock_free,"32-bit callback counters must be lock-free");
inline void receiveCallback(const esp_now_recv_info_t* info,const uint8_t* bytes,int length){
 const uint32_t token=activeGeneration.load(std::memory_order_acquire);
 if(!token||!info||!info->src_addr||!bytes||length<4||length>int(coop::kMaxPacket))return;
 Datagram d{};d.generation=token;d.size=uint16_t(length);std::memcpy(d.source.bytes,info->src_addr,6);std::memcpy(d.bytes,bytes,length);
 if(xQueueSend(inbox,&d,0)!=pdTRUE)queueDrops.fetch_add(1,std::memory_order_relaxed);
}
enum class OpenError:uint8_t {None,Busy,Profile,Queue,WifiMode,WifiStorage,Channel,Mac,Radio,Callback,Session};
class Device {
 coopplatform::Session* session=nullptr;bool opened=false,radioReady=false,wifiTouched=false,lease=false;
 wifi_mode_t oldMode=WIFI_MODE_NULL;bool oldAuto=false,oldConnected=false;uint8_t oldChannel=0;
 wifi_second_chan_t oldSecondary=WIFI_SECOND_CHAN_NONE;uint32_t generation=0;
 uint8_t peerCount=0,nextEvict=0;coopplatform::Mac peerCache[8]{};
 static bool send(void* context,const coopplatform::Mac& destination,const uint8_t* bytes,size_t size){
  auto& self=*static_cast<Device*>(context);
  if(!self.opened||owner!=Owner::Training||!bytes||!size||size>coop::kMaxPacket)return false;
  if(!esp_now_is_peer_exist(destination.bytes)){
   // Bound ESP-NOW peer records even under unsolicited admission traffic.
   if(self.peerCount>=8){
    bool evicted=false;const auto* keep=self.session?self.session->pendingPeer():nullptr;
    for(unsigned i=0;i<8;++i){const uint8_t slot=(self.nextEvict+i)%8;
     if(self.peerCache[slot].bytes[0]==255||(keep&&coopplatform::sameMac(self.peerCache[slot],keep->mac)))continue;
     if(esp_now_del_peer(self.peerCache[slot].bytes)!=ESP_OK)return false;
     self.peerCache[slot]=self.peerCache[--self.peerCount];self.nextEvict=uint8_t((slot+1)%8);evicted=true;break;
    }
    if(!evicted)return false;
   }
   esp_now_peer_info_t peer{};std::memcpy(peer.peer_addr,destination.bytes,6);peer.channel=6;peer.ifidx=WIFI_IF_STA;peer.encrypt=false;
   if(esp_now_add_peer(&peer)!=ESP_OK)return false;self.peerCache[self.peerCount++]=destination;
  }
  return esp_now_send(destination.bytes,bytes,size)==ESP_OK;
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
public:
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
  oldMode=mode;oldAuto=WiFi.getAutoReconnect();oldConnected=WiFi.status()==WL_CONNECTED;oldChannel=0;
  if(mode!=WIFI_MODE_NULL){if(esp_wifi_get_channel(&oldChannel,&oldSecondary)!=ESP_OK)return fail(OpenError::Channel);if(esp_wifi_set_storage(WIFI_STORAGE_RAM)!=ESP_OK)return fail(OpenError::WifiStorage);}
  wifiTouched=true;WiFi.setAutoReconnect(false);
  // For a previously disabled radio, initialize directly into RAM storage.
  WiFi.persistent(false);const bool started=WiFi.mode(WIFI_STA);WiFi.persistent(true);
  if(!started)return fail(OpenError::WifiMode);
  if(esp_wifi_set_storage(WIFI_STORAGE_RAM)!=ESP_OK)return fail(OpenError::WifiStorage);
  WiFi.disconnect(false,false,100); // Existing credentials are never erased.
  if(esp_wifi_set_channel(6,WIFI_SECOND_CHAN_NONE)!=ESP_OK)return fail(OpenError::Channel);
  coopplatform::Mac own{};if(esp_wifi_get_mac(WIFI_IF_STA,own.bytes)!=ESP_OK||!coopplatform::usableMac(own))return fail(OpenError::Mac);
  if(esp_now_init()!=ESP_OK)return fail(OpenError::Radio);radioReady=true;
  generation=++nextGeneration;activeGeneration.store(generation,std::memory_order_release);
  if(esp_now_register_recv_cb(receiveCallback)!=ESP_OK)return fail(OpenError::Callback);
  opened=true;peerCount=nextEvict=0;session=&target;target.setSender(send,this);
  if(!target.open(game,own,entropy,now))return fail(OpenError::Session);
  lastError=OpenError::None;return lastError;
 }
 void tick(uint32_t now){
  if(!opened||!session)return;
  Datagram d{};for(unsigned i=0;i<8&&xQueueReceive(inbox,&d,0)==pdTRUE;++i){
   if(d.generation!=generation){++stalePackets;continue;}
   session->receive(d.source,d.bytes,d.size,now);
  }
  session->tick(now);
 }
 void close(){
  // Do not release/alter another owner's radio when open() never acquired it.
  if(!lease||owner!=Owner::Training)return;
  activeGeneration.store(0,std::memory_order_release);opened=false;
  if(radioReady){esp_now_unregister_recv_cb();esp_now_deinit();radioReady=false;}
  if(session){session->setSender(nullptr,nullptr);session=nullptr;}
  restoreOk=restoreWifi();generation=0;peerCount=0;release(Owner::Training);lease=false;
 }
};
}
