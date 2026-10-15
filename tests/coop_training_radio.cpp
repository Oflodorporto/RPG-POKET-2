#include <freertos/FreeRTOS.h>
#define xQueueCreateStatic uncheckedQueueCreateStatic
#define xQueueSend uncheckedQueueSend
#define xQueueReceive uncheckedQueueReceive
#include <freertos/queue.h>
#undef xQueueCreateStatic
#undef xQueueSend
#undef xQueueReceive
inline QueueHandle_t xQueueCreateStatic(unsigned n,unsigned size,uint8_t* bytes,StaticQueue_t* q){assert(!fakeCriticalDepth);return uncheckedQueueCreateStatic(n,size,bytes,q);}
inline int xQueueSend(QueueHandle_t q,const void* p,unsigned wait){assert(!fakeCriticalDepth);return uncheckedQueueSend(q,p,wait);}
inline int xQueueReceive(QueueHandle_t q,void* p,unsigned wait){assert(!fakeCriticalDepth);return uncheckedQueueReceive(q,p,wait);}
#include "../firmware/RPG_POKET_2/Coop/CoopRadioDevice.h"
#include "../firmware/RPG_POKET_2/Coop/CoopEligibilityBridge.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <new>
static unsigned allocations=0,checks=0;
#define CHECK(x) do {++checks;assert(x);}while(0)
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
using namespace coopplatform;
static rpg::Game hero(){auto g=rpg::create(0,123);CHECK(trainingEligible(g));return g;}
void normal(bool connected){
 fake::reset();if(connected){fake::mode=WIFI_MODE_STA;fake::connected=true;}auto g=hero(),before=g;Session s;coopradio::Device d,other;
 CHECK(d.open(s,g,false,12345,100)==coopradio::OpenError::None);CHECK(d.active()&&fake::radio&&fake::callbackSet&&coopradio::owner==coopradio::Owner::Training);CHECK(fake::storage==WIFI_STORAGE_RAM&&!fake::autoReconnect&&fake::channel==6);
 other.close();CHECK(d.active()&&fake::radio&&coopradio::owner==coopradio::Owner::Training);CHECK(!coopradio::acquire(coopradio::Owner::Pvp));
 CHECK(d.open(s,g,false,999,100)==coopradio::OpenError::Busy);
 s.leave(200);d.close();CHECK(!d.active()&&!fake::radio&&!fake::callbackSet&&coopradio::owner==coopradio::Owner::None);CHECK(fake::mode==(connected?WIFI_MODE_STA:WIFI_MODE_NULL)&&fake::autoReconnect&&fake::persistent);CHECK(fake::connected==connected);CHECK(!fake::erases&&!fake::nvsWrites);CHECK(!std::memcmp(&g,&before,sizeof(g)));
 CHECK(coopradio::acquire(coopradio::Owner::Pvp));auto calls=fake::mutations;CHECK(d.open(s,g,false,55,300)==coopradio::OpenError::Busy&&fake::mutations==calls);d.close();CHECK(coopradio::owner==coopradio::Owner::Pvp);coopradio::release(coopradio::Owner::Pvp);
 CHECK(d.open(s,g,false,77,400)==coopradio::OpenError::None);s.leave(410);d.close();CHECK(fake::initCount==2&&fake::deinitCount==2);
}
void failures(){
 for(int fault=1;fault<=6;++fault){fake::reset();fake::fail=fault;Session s;coopradio::Device d;auto g=hero();CHECK(d.open(s,g,false,1,0)!=coopradio::OpenError::None);CHECK(!d.active()&&!fake::radio&&!fake::callbackSet&&coopradio::owner==coopradio::Owner::None);CHECK(!fake::nvsWrites&&!fake::erases&&fake::persistent);}
 fake::reset();fake::mode=WIFI_MODE_AP;Session s;coopradio::Device d;auto g=hero();CHECK(d.open(s,g,false,1,0)==coopradio::OpenError::WifiMode&&fake::mutations==0);
 fake::reset();CHECK(d.open(s,g,true,1,0)==coopradio::OpenError::Busy&&fake::mutations==0);g.phase=rpg::Phase::Hero;CHECK(d.open(s,g,false,1,0)==coopradio::OpenError::Profile&&fake::mutations==0);
}
void callbackAndPeers(){
 fake::reset();auto g=hero();Session s;coopradio::Device d;CHECK(d.open(s,g,false,123,100)==coopradio::OpenError::None);auto savedReceiver=receiver;CHECK(s.host(100));
 coop::Actor guest{};CHECK(coop::playerProfile(g,1,guest));Mac source{{2,0,0,0,1,1}};
 auto makeJoin=[&](uint64_t token){admission::Packet p{};auto w=admission::begin(p,admission::Type::Join,s.sessionToken(),token);coop::writeActor(w,guest);CHECK(admission::finish(p,w));return p;};
 auto join=makeJoin(123);esp_now_recv_info_t info{source.bytes};auto count=fake::sendCount;
 receiver(&info,join.bytes,join.size);CHECK(s.state()==SessionState::Hosting&&fake::sendCount==count&&!esp_now_is_peer_exist(source.bytes));d.tick(110);CHECK(s.state()==SessionState::AwaitingAccept); // callback itself never runs session
 for(unsigned i=0;i<12;++i){source.bytes[5]=uint8_t(10+i);auto p=makeJoin(500+i);info.src_addr=source.bytes;receiver(&info,p.bytes,p.size);d.tick(200+i);}
 CHECK(fake::peerCount<=8&&fake::peerDeletes>0);CHECK(esp_now_is_peer_exist(s.pendingPeer()->mac.bytes)); // Waiting reply adds and protects the pending guest on the loop only
 auto bad=s.malformed;for(unsigned i=0;i<12;++i)receiver(&info,join.bytes,join.size);CHECK(coopradio::callbackCounters.drops()>=4&&s.malformed==bad);d.tick(400);
 receiver(nullptr,join.bytes,join.size);receiver(&info,nullptr,4);receiver(&info,join.bytes,-1);receiver(&info,join.bytes,241);
 // Queue old bytes, close/reopen, then prove a late callback is either inactive
 // or carries only the new generation and is rejected by new session tokens.
 receiver(&info,join.bytes,join.size);s.leave(500);d.close();savedReceiver(&info,join.bytes,join.size);
 CHECK(d.open(s,g,false,456,600)==coopradio::OpenError::None);d.tick(610);CHECK(d.stalePackets>0&&s.state()==SessionState::Browsing);s.leave(620);d.close();CHECK(!fake::nvsWrites&&!fake::erases);
}
struct Pause {
 pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
 bool captured=false,released=false;esp_now_recv_info_t info{};uint8_t bytes[4]={1,2,3,4};
};
static Pause* pauseState=nullptr;
static thread_local bool pauseCallback=false;
static void afterUnlock(){
 assert(!fakeCriticalDepth);
 if(!pauseCallback)return;pauseCallback=false;auto& p=*pauseState;
 assert(!pthread_mutex_lock(&p.mutex));p.captured=true;assert(!pthread_cond_broadcast(&p.changed));
 while(!p.released)assert(!pthread_cond_wait(&p.changed,&p.mutex));
 assert(!pthread_mutex_unlock(&p.mutex));
}
static void* delayedCallback(void* arg){auto& p=*static_cast<Pause*>(arg);pauseCallback=true;coopradio::receiveCallback(&p.info,p.bytes,4);return nullptr;}
static void interleavedCallback(bool reopen){
 fake::reset();auto g=hero(),before=g;Session s;coopradio::Device d;
 CHECK(d.open(s,g,false,123,100)==coopradio::OpenError::None);d.tick(100);
 const uint32_t old=coopradio::callbackCounters.generation();CHECK(old);
 uint8_t source[6]={2,0,0,0,0,9};Pause p;p.info.src_addr=source;pauseState=&p;fakeAfterCritical=afterUnlock;
 pthread_t thread;CHECK(!pthread_create(&thread,nullptr,delayedCallback,&p));
 CHECK(!pthread_mutex_lock(&p.mutex));while(!p.captured)assert(!pthread_cond_wait(&p.changed,&p.mutex));CHECK(!pthread_mutex_unlock(&p.mutex));
 // Callback has captured old generation and released the portMUX, but has not
 // touched the queue. Main loop may close/reopen without waiting on its body.
 s.leave(150);const auto state=s.state();d.close();CHECK(!d.active()&&coopradio::callbackCounters.generation()==0);
 if(reopen){CHECK(d.open(s,g,false,456,200)==coopradio::OpenError::None);CHECK(coopradio::callbackCounters.generation()!=old);}
 const auto sent=fake::sendCount;
 CHECK(!pthread_mutex_lock(&p.mutex));p.released=true;CHECK(!pthread_cond_broadcast(&p.changed));CHECK(!pthread_mutex_unlock(&p.mutex));
 CHECK(!pthread_join(thread,nullptr));fakeAfterCritical=nullptr;pauseState=nullptr;
 CHECK(!fakeCriticalDepth&&fakeCriticalEntries==fakeCriticalExits);CHECK(fake::sendCount==sent);
 if(reopen){auto rejected=d.stalePackets;d.tick(210);CHECK(d.stalePackets==rejected+1&&s.state()==SessionState::Browsing);d.close();}
 else {CHECK(s.state()==state);coopradio::Datagram packet{};CHECK(xQueueReceive(coopradio::inbox,&packet,0)==pdTRUE&&packet.generation==old);}
 CHECK(!std::memcmp(&g,&before,sizeof(g)));CHECK(!pthread_cond_destroy(&p.changed));CHECK(!pthread_mutex_destroy(&p.mutex));
}
struct CounterRace {coopradio::CallbackCounters counters;std::atomic<bool> start{false};};
static void* counterWorker(void* arg){auto& r=*static_cast<CounterRace*>(arg);while(!r.start.load(std::memory_order_acquire)){}
 for(unsigned i=0;i<20000;++i){auto v=r.counters.generation();assert(v==0||v==0xaaaaaaaau||v==0x55555555u);r.counters.drop();}return nullptr;}
static void concurrentCounters(){
 CounterRace r;pthread_t thread;CHECK(!pthread_create(&thread,nullptr,counterWorker,&r));r.start.store(true,std::memory_order_release);
 for(unsigned i=0;i<20000;++i){r.counters.publish(i&1?0xaaaaaaaau:0x55555555u);r.counters.drop();}
 CHECK(!pthread_join(thread,nullptr));CHECK(r.counters.drops()==40000);CHECK(fakeCriticalEntries==fakeCriticalExits&&!fakeCriticalDepth);
 coopradio::CallbackCounters saturating(UINT32_MAX-1);saturating.drop();saturating.drop();CHECK(saturating.drops()==UINT32_MAX);
 fake::reset();auto g=hero();Session s;coopradio::Device d;auto saved=coopradio::nextGeneration;coopradio::nextGeneration=UINT32_MAX;
 CHECK(d.open(s,g,false,1,100)==coopradio::OpenError::Busy&&fake::mutations==0&&coopradio::owner==coopradio::Owner::None);coopradio::nextGeneration=saved;
 std::puts("PASS: two-thread callback/close/reopen fences, scalar publication/drop contention, saturation and exhausted-generation refusal; locks released before queue work");
}
int main(){normal(false);normal(true);failures();callbackAndPeers();interleavedCallback(false);interleavedCallback(true);concurrentCounters();CHECK(!allocations);std::printf("PASS: %u checks; radio ownership, rollback, bounded callback/peer queues, reentry; no app/driver-config NVS writes in fake; Device=%zu static inbox=%zu; zero C++ allocations\n",checks,sizeof(coopradio::Device),sizeof(coopradio::queueBytes)+sizeof(coopradio::queueState));}
