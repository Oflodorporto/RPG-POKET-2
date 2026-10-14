#include "../firmware/RPG_POKET_2/Coop/CoopRadioDevice.h"
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
 receiver(&info,join.bytes,join.size);CHECK(s.state()==SessionState::Hosting&&fake::sendCount==count);d.tick(110);CHECK(s.state()==SessionState::AwaitingAccept); // callback itself never runs session
 for(unsigned i=0;i<12;++i){source.bytes[5]=uint8_t(10+i);auto p=makeJoin(500+i);info.src_addr=source.bytes;receiver(&info,p.bytes,p.size);d.tick(200+i);}
 CHECK(fake::peerCount<=8&&fake::peerDeletes>0);CHECK(esp_now_is_peer_exist(s.pendingPeer()->mac.bytes)==false); // awaiting guest not transmitted to yet
 auto bad=s.malformed;for(unsigned i=0;i<12;++i)receiver(&info,join.bytes,join.size);CHECK(coopradio::queueDrops>=4&&s.malformed==bad);d.tick(400);
 receiver(nullptr,join.bytes,join.size);receiver(&info,nullptr,4);receiver(&info,join.bytes,-1);receiver(&info,join.bytes,241);
 // Queue old bytes, close/reopen, then prove a late callback is either inactive
 // or carries only the new generation and is rejected by new session tokens.
 receiver(&info,join.bytes,join.size);s.leave(500);d.close();savedReceiver(&info,join.bytes,join.size);
 CHECK(d.open(s,g,false,456,600)==coopradio::OpenError::None);d.tick(610);CHECK(d.stalePackets>0&&s.state()==SessionState::Browsing);s.leave(620);d.close();CHECK(!fake::nvsWrites&&!fake::erases);
}
int main(){normal(false);normal(true);failures();callbackAndPeers();CHECK(!allocations);std::printf("PASS: %u checks; radio ownership, rollback, bounded callback/peer queues, reentry; no app/driver-config NVS writes in fake; Device=%zu static inbox=%zu; zero C++ allocations\n",checks,sizeof(coopradio::Device),sizeof(coopradio::queueBytes)+sizeof(coopradio::queueState));}
