#pragma once
#include "CoopTraining.h"
#include <atomic>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
namespace coopplatform {
// This is a PORT, not a radio owner. It never initializes ESP-NOW, sets Wi-Fi,
// registers a global callback, adds peers, or calls any ClubRadio routine.
// A future exclusive radio coordinator must provide those lifecycle operations.
// Keep this object/queue alive for the entire firmware lifetime. attach/detach,
// Training, polling and flushing are main-loop only; enqueue is callback-safe.
class RadioPort {
 struct Datagram {uint32_t generation=0;Mac source{};coop::Packet packet{};};
 StaticQueue_t queueState{};uint8_t queueBytes[8*sizeof(Datagram)]{};QueueHandle_t queue=nullptr;
 std::atomic<uint32_t> generation{0};uint32_t serial=0,sessionGeneration=0;Training* training=nullptr;
public:
 std::atomic<uint32_t> queueDrops{0};uint32_t rejected=0,stale=0,sendErrors=0;
 static_assert(std::atomic<uint32_t>::is_always_lock_free,"callback counters require lock-free 32-bit atomics");
 bool initializeQueue(){if(queue)return true;queue=xQueueCreateStatic(8,sizeof(Datagram),queueBytes,&queueState);return queue!=nullptr;}
 // Caller initializes queue BEFORE enabling callback; it is never destroyed.
 bool attach(Training& session){
  if(!queue||training||!session.active()||serial==UINT32_MAX)return false;
  training=&session;sessionGeneration=session.generationId();generation.store(++serial,std::memory_order_release);return true;
 }
 void detach(){generation.store(0,std::memory_order_release);training=nullptr;sessionGeneration=0;}
 void enqueue(const esp_now_recv_info_t* info,const uint8_t* bytes,int length){
  const uint32_t token=generation.load(std::memory_order_acquire);
  if(!token||!info||!info->src_addr||!bytes||length<int(coop::kHeaderBytes+4)||length>int(coop::kMaxPacket))return;
  Datagram d{};d.generation=token;std::memcpy(d.source.bytes,info->src_addr,6);d.packet.size=uint16_t(length);std::memcpy(d.packet.bytes,bytes,length);
  if(xQueueSend(queue,&d,0)!=pdTRUE)queueDrops.fetch_add(1,std::memory_order_relaxed);
 }
 void poll(uint32_t now){
  if(!queue)return;
  Datagram d{};for(unsigned i=0;i<8&&xQueueReceive(queue,&d,0)==pdTRUE;++i){
   if(!attached()||d.generation!=generation.load(std::memory_order_acquire)){++stale;continue;}
   if(!training->receive(d.source,d.packet,now))++rejected;
  }
  if(attached())training->tick(now);
 }
 // ESP_OK means queued by the API, NOT delivered. Only protocol ACK confirms.
 // Dropped submission is recovered by Endpoint's bounded reliable retry/sync.
 bool attached()const{return training&&training->active()&&training->generationId()==sessionGeneration;}
 void flush(){
  if(!attached())return;
  coop::Packet p{};for(unsigned i=0;i<8&&training->endpoint.outgoing.pop(p);++i){
   if(esp_now_send(training->binding().remote.bytes,p.bytes,p.size)!=ESP_OK)++sendErrors;
  }
 }
};
}
