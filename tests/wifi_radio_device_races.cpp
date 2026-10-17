// True pthread callback/close/reopen race tests. Static fake SDK, no RF claim.
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <pthread.h>
#include <sched.h>
#include <new>
static std::atomic<unsigned> allocations{0};
void* operator new(std::size_t n){++allocations;if(void*p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
#include "WifiRadioTestDevice.h"
using namespace wifi_radio_test;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort();}}while(0)
static uint8_t peer[6]={2,0,0,0,0,2};
static std::atomic<bool> entered{false},releaseCallback{false};
static thread_local bool pauseThisThread=false;
static void pauseAfterEntry(){if(!pauseThisThread)return;pauseThisThread=false;entered=true;while(!releaseCallback.load())sched_yield();}
static void receiveOne(){WireFrame f{};f.nonce=2;uint8_t bytes[kMaxFrame];const auto n=encode(f,bytes,sizeof bytes);wifi_pkt_rx_ctrl_t ctrl{};ctrl.rssi=-60;ctrl.channel=6;esp_now_recv_info_t info{peer,nullptr,&ctrl};device_detail::receiveCallback(&info,bytes,int(n));}
static void* blockedReceive(void*){pauseThisThread=true;receiveOne();return nullptr;}
static void* blockedSend(void*){pauseThisThread=true;esp_now_send_info_t info{peer};device_detail::sendCallback(&info,ESP_NOW_SEND_SUCCESS);return nullptr;}
static void reset(){CHECK(coopradio::owner==coopradio::Owner::None);CHECK(device_detail::mailbox.idle());device_detail::mailbox.fence();fake::reset();WiFi.persistent(true);entered=false;releaseCallback=false;}
static void entryCloseRace(bool send){
 reset();Device d;CHECK(d.open(false,101,100)==OpenError::None);const auto epoch=device_detail::nextGeneration;
 fakeAfterCritical=pauseAfterEntry;pthread_t callback{};CHECK(!pthread_create(&callback,nullptr,send?blockedSend:blockedReceive,nullptr));while(!entered.load())sched_yield();
 CHECK(!d.close());CHECK(d.active()&&d.cleanupPending());CHECK(fake::calls[fake::Deinit]==0);CHECK(coopradio::owner==coopradio::Owner::WifiTest);
 Device intruder;CHECK(intruder.open()==OpenError::Busy);CHECK(device_detail::nextGeneration==epoch);CHECK(!d.retryRestore());
 releaseCallback=true;CHECK(!pthread_join(callback,nullptr));fakeAfterCritical=nullptr;CHECK(device_detail::mailbox.idle());
 const auto old=d.snapshot(100).callbacks;CHECK(old.received==0&&old.macSuccess==0&&old.macFailure==0&&old.queueDrops==0);
 CHECK(d.retryRestore());CHECK(d.open(false,102,101)==OpenError::None);CHECK(device_detail::nextGeneration==epoch+1);d.tick(101);CHECK(d.flow().peerCount()==0);CHECK(d.snapshot(101).callbacks.received==0);CHECK(d.close());
}
struct Load {unsigned count=0;bool invalid=false;};
static void* flood(void*arg){const auto load=*static_cast<Load*>(arg);esp_now_send_info_t info{peer};for(unsigned i=0;i<load.count;++i){if(load.invalid)device_detail::receiveCallback(nullptr,nullptr,0);else receiveOne();device_detail::sendCallback(&info,i&1?ESP_NOW_SEND_FAIL:ESP_NOW_SEND_SUCCESS);}return nullptr;}
static void contention(){
 reset();Device d;CHECK(d.open()==OpenError::None);Load load[4]={{10000,false},{10000,false},{10000,true},{10000,true}};pthread_t thread[4]{};
 for(unsigned i=0;i<4;++i)CHECK(!pthread_create(&thread[i],nullptr,flood,&load[i]));
 for(auto&t:thread)CHECK(!pthread_join(t,nullptr));
 const auto s=d.snapshot(100).callbacks;CHECK(s.received==40000&&s.invalid==20000&&s.queueDrops==20000-kDeviceInbox);CHECK(s.macSuccess==20000&&s.macFailure==20000);
 device_detail::Datagram frame{};unsigned count=0;while(device_detail::mailbox.pop(frame)){CHECK(frame.generation==device_detail::nextGeneration&&frame.at==100);++count;}CHECK(count==kDeviceInbox);CHECK(d.close());
 const auto fenced=d.snapshot(100).callbacks;receiveOne();device_detail::sendCallback(nullptr,ESP_NOW_SEND_FAIL);CHECK(d.snapshot(100).callbacks.received==fenced.received);
 uint32_t saturated=UINT32_MAX-1;device_detail::increment(saturated);device_detail::increment(saturated);CHECK(saturated==UINT32_MAX);
}
static std::atomic<bool> stop{false};
static void* concurrentProducer(void*){esp_now_send_info_t info{peer};while(!stop.load()){auto cb=receiver.load();if(cb){WireFrame f{};f.nonce=2;uint8_t bytes[kMaxFrame];size_t n=encode(f,bytes,sizeof bytes);esp_now_recv_info_t incoming{peer,nullptr,nullptr};cb(&incoming,bytes,int(n));}auto sent=senderCallback.load();if(sent)sent(&info,ESP_NOW_SEND_SUCCESS);}return nullptr;}
static void repeatedCloseReopen(){
 reset();Device d;stop=false;pthread_t producer{};CHECK(!pthread_create(&producer,nullptr,concurrentProducer,nullptr));
 for(unsigned cycle=0;cycle<100;++cycle){CHECK(d.open(false,cycle+1,100)==OpenError::None);for(unsigned i=0;i<5;++i)d.tick(100);while(!d.close())sched_yield();CHECK(!d.active());CHECK(coopradio::owner==coopradio::Owner::None);}
 stop=true;CHECK(!pthread_join(producer,nullptr));CHECK(device_detail::mailbox.idle());CHECK(fakeCriticalEntries==fakeCriticalExits);CHECK(fake::nvsWrites==0&&fake::countrySets==0&&fake::erases==0);
}
int main(){entryCloseRace(false);std::puts("PASS race: blocked receive entry fences close, lease and reopen until callback exits; late generation never reaches Flow");entryCloseRace(true);std::puts("PASS race: blocked MAC completion entry cannot contaminate the next session or release resources in flight");contention();std::puts("PASS race: 4 pthreads, 40,000 receives + 40,000 completions, exact bounded 8-frame queue drops and saturating counters");repeatedCloseReopen();CHECK(allocations.load()==0);std::puts("PASS wifi_radio_device_races: 100 concurrent close/reopen cycles, balanced SMP critical sections, zero C++ allocations, no app NVS/credentials/erase");}
