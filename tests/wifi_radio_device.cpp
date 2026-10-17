// Host-only independent adapter lifecycle/driver model; no RF/hardware claims.
#include <cstdio>
#include <cstdlib>
#include <new>
#include <atomic>
#include <initializer_list>
static std::atomic<unsigned> allocations{0};
void* operator new(std::size_t n){++allocations;if(void*p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
#include "WifiRadioTestDevice.h"
using namespace wifi_radio_test;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort();}}while(0)
static unsigned scenarios=0;
static void reset(){CHECK(coopradio::owner==coopradio::Owner::None);CHECK(device_detail::mailbox.idle());device_detail::mailbox.fence();fake::reset();WiFi.persistent(true);receiver=nullptr;senderCallback=nullptr;}
static void noWrites(){CHECK(fake::nvsWrites==0&&fake::erases==0&&fake::countrySets==0&&fake::credentialsWrites==0);}
static void restored(const Device&d,bool connected=true){CHECK(!d.active()&&!d.cleanupPending());CHECK(coopradio::owner==coopradio::Owner::None);CHECK(!fake::radio&&!receiver.load()&&!senderCallback.load());CHECK(fake::mode==WIFI_MODE_STA&&fake::channel==11&&fake::secondary==WIFI_SECOND_CHAN_ABOVE);CHECK(fake::ps==WIFI_PS_MIN_MODEM&&fake::protocol==7&&fake::power==80);CHECK(fake::storage==WIFI_STORAGE_FLASH&&fake::persistent&&fake::autoReconnect);CHECK(fake::connected==connected);noWrites();}
static void happy(){
 for(bool associated:{false,true})for(bool persistent:{false,true}){
  reset();fake::connected=associated;WiFi.persistent(persistent);Device d;const unsigned a=allocations.load();CHECK(d.open(false,123,100)==OpenError::None);CHECK(d.active()&&d.status().opened&&!d.cleanupPending());CHECK(d.status().originalConnected==associated&&d.status().originalPersistent==persistent&&d.status().originalProfileValid);CHECK(fake::channel==6&&fake::secondary==WIFI_SECOND_CHAN_NONE&&!fake::connected&&!fake::autoReconnect&&!fake::persistent&&fake::storage==WIFI_STORAGE_RAM);CHECK(d.baselineRaw()==80&&d.ceilingRaw()==80);CHECK(d.applyPower(20).actualRaw==20);CHECK(d.applyPower(80).actualRaw==80);CHECK(d.close());CHECK(allocations.load()==a);CHECK(fake::persistent==persistent);WiFi.persistent(true);restored(d,associated);++scenarios;
 }
 reset();fake::mode=WIFI_MODE_NULL;fake::connected=false;Device off;CHECK(off.open()==OpenError::None);CHECK(!off.status().originalProfileValid&&off.status().baselineValid);CHECK(off.applyPower(44).actualRaw==44);CHECK(off.close());CHECK(fake::mode==WIFI_MODE_NULL&&!off.status().currentValid&&fake::persistent&&fake::autoReconnect);noWrites();++scenarios;
 std::puts("PASS device: connected/disconnected/off snapshots, PS/PHY/current baseline, exact restoration and zero adapter C++ allocations");
}
static void exclusion(){
 for(auto owner:{coopradio::Owner::Pvp,coopradio::Owner::Training,coopradio::Owner::WifiTest}){reset();coopradio::owner=owner;Device d;CHECK(d.open()==OpenError::Busy);CHECK(!d.active());CHECK(d.close());CHECK(coopradio::owner==owner);CHECK(fake::calls[fake::Init]==0&&fake::mutations==0);coopradio::owner=coopradio::Owner::None;++scenarios;}
 reset();Device d;CHECK(d.open(true)==OpenError::Busy);CHECK(fake::mutations==0);for(auto mode:{WIFI_MODE_AP,WIFI_MODE_APSTA}){fake::mode=mode;CHECK(d.open()==OpenError::WifiMode);CHECK(!d.active()&&fake::mutations==0);++scenarios;}
 std::puts("PASS device: existing PvP/training/diagnostic owner, caller block and unsupported modes never replace callbacks or mutate Wi-Fi");
}
static void openFailures(){
 const fake::Op ops[]={fake::GetMode,fake::GetChannel,fake::GetPS,fake::GetProtocol,fake::GetPower,fake::GetCountry,fake::GetAP,fake::Auto,fake::Storage,fake::Mode,fake::Disconnect,fake::SetChannel,fake::GetMac,fake::Init,fake::RegisterRx,fake::RegisterTx};
 for(auto op:ops){reset();Device d;fake::failAt[op]=1;const auto err=d.open(false,44,100);CHECK(err!=OpenError::None);if(d.active())CHECK(d.retryRestore());CHECK(!d.active());CHECK(coopradio::owner==coopradio::Owner::None);noWrites();++scenarios;}
 // Failure after ownership followed by rollback failure must retain the lease.
 reset();Device d;fake::failAt[fake::RegisterTx]=1;fake::failures[fake::Deinit]=2;CHECK(d.open()==OpenError::Callback);CHECK(d.active()&&d.cleanupPending());CHECK(coopradio::owner==coopradio::Owner::WifiTest);CHECK(!d.retryRestore());CHECK(d.active());CHECK(d.retryRestore());restored(d);++scenarios;
 // Readback failure after starting initially-off Wi-Fi remains recoverable.
 reset();fake::mode=WIFI_MODE_NULL;fake::connected=false;fake::failAt[fake::GetPower]=1;Device off;CHECK(off.open()==OpenError::Profile);CHECK(!off.active()&&fake::mode==WIFI_MODE_NULL);noWrites();++scenarios;
 std::puts("PASS device: every open-stage API failure, post-lease partial-open/failed rollback and retry, off prestart unknown fields");
}
static void everyOpenCall(){
 for(bool initiallyOff:{false,true}){
  unsigned counts[fake::Count]{};reset();if(initiallyOff){fake::mode=WIFI_MODE_NULL;fake::connected=false;}Device reference;CHECK(reference.open()==OpenError::None);std::memcpy(counts,fake::calls,sizeof counts);CHECK(reference.close());
  for(unsigned op=1;op<fake::Count;++op)for(unsigned call=1;call<=counts[op];++call){
   reset();if(initiallyOff){fake::mode=WIFI_MODE_NULL;fake::connected=false;}fake::failAt[op]=call;Device d;CHECK(d.open()!=OpenError::None);if(d.active())CHECK(d.retryRestore());CHECK(!d.active()&&coopradio::owner==coopradio::Owner::None);CHECK(fake::mode==(initiallyOff?WIFI_MODE_NULL:WIFI_MODE_STA));noWrites();++scenarios;
  }
 }
 std::puts("PASS device: exhaustive failure injection at every successful open API call index, including post-disconnect/profile readbacks and cold-start paths");
}
static void closeFailures(){
 for(auto op:{fake::UnregisterRx,fake::UnregisterTx,fake::Deinit,fake::Storage,fake::GetMode,fake::GetChannel,fake::GetPS,fake::GetProtocol,fake::GetPower,fake::GetCountry,fake::SetChannel,fake::Connect,fake::Auto}){
  reset();Device d;CHECK(d.open()==OpenError::None);fake::failAt[op]=fake::calls[op]+1;CHECK(!d.close());CHECK(d.active()&&d.cleanupPending()&&coopradio::owner==coopradio::Owner::WifiTest);CHECK(d.retryRestore());restored(d);++scenarios;
 }
 for(auto op:{fake::SetPS,fake::SetProtocol,fake::SetPower}){reset();Device d;CHECK(d.open()==OpenError::None);fake::ps=WIFI_PS_NONE;fake::protocol=3;fake::power=20;fake::country.max_tx_power=5;fake::failAt[op]=fake::calls[op]+1;CHECK(!d.close());CHECK(d.active()&&d.cleanupPending());CHECK(d.retryRestore());restored(d);++scenarios;}
 reset();Device d;CHECK(d.open()==OpenError::None);CHECK(d.applyPower(20).actualRaw==20);fake::forcedActual=44;CHECK(!d.close());CHECK(d.status().restoreError!=0&&d.active());fake::forcedActual=0;CHECK(d.retryRestore());restored(d);++scenarios;
 reset();fake::mode=WIFI_MODE_NULL;fake::connected=false;Device off;CHECK(off.open()==OpenError::None);fake::failAt[fake::Mode]=fake::calls[fake::Mode]+1;CHECK(!off.close());CHECK(off.active());CHECK(off.retryRestore());CHECK(fake::mode==WIFI_MODE_NULL);noWrites();++scenarios;
 reset();Device pending;CHECK(pending.open()==OpenError::None);fake::reconnectImmediate=false;CHECK(pending.close());CHECK(pending.status().associationRequested&&!pending.status().associationConfirmed&&!pending.active());restored(pending,false);++scenarios;
 std::puts("PASS device: teardown/profile/storage/autoreconnect/power readback failures retain exclusive lease; retries verify cleanup; async AP reconnect reported honestly");
}
static void pairDevice(Device&d);
static void countryAndCaps(){
 reset();Device d;CHECK(d.open()==OpenError::None);for(int8_t raw:{20,44,60,80}){const auto p=d.applyPower(raw);CHECK(p.requestedRaw==raw&&p.actualRaw==raw&&p.actualValid&&!p.error);CHECK(d.ceilingRaw()==80);}CHECK(d.applyPower(84).error);CHECK(d.applyPower(81).error);CHECK(d.applyPower(80).actualRaw==80);CHECK(d.close());restored(d);++scenarios;
 reset();fake::power=60;fake::country.max_tx_power=15;Device capped;CHECK(capped.open()==OpenError::None);CHECK(capped.ceilingRaw()==60);CHECK(capped.applyPower(80).requestedRaw==80&&capped.applyPower(80).actualRaw==60);CHECK(capped.applyPower(20).actualRaw==20);CHECK(capped.applyPower(60).actualRaw==60);CHECK(capped.ceilingRaw()==60);CHECK(capped.close());CHECK(fake::power==60&&fake::country.max_tx_power==15);noWrites();++scenarios;
 reset();fake::power=52;fake::country.max_tx_power=13;Device cap52;CHECK(cap52.open()==OpenError::None);CHECK(cap52.ceilingRaw()==52);CHECK(cap52.applyPower(80).requestedRaw==80&&cap52.applyPower(80).actualRaw==52);CHECK(cap52.applyPower(20).actualRaw==20);CHECK(cap52.applyPower(80).actualRaw==52);CHECK(cap52.close());CHECK(fake::power==52);noWrites();++scenarios;
 for(int8_t baseline:{34,66}){reset();fake::power=baseline;fake::country.max_tx_power=baseline/4;Device mapped;CHECK(mapped.open()==OpenError::None);CHECK(mapped.ceilingRaw()==baseline/4*4);pairDevice(mapped);const unsigned setters=fake::calls[fake::SetPower];CHECK(mapped.flow().continueAtDistance(Distance::TwoPalms,millis()));CHECK(fake::power==baseline&&fake::calls[fake::SetPower]==setters);CHECK(mapped.flow().current().localPower.actualRaw==baseline);const auto preset=mapped.applyPower(80);CHECK(preset.requestedRaw==80&&preset.actualValid&&preset.actualRaw<=mapped.ceilingRaw());CHECK(mapped.restoreBaseline().actualRaw==baseline);CHECK(mapped.close());CHECK(fake::power==baseline);noWrites();++scenarios;}
 reset();Device actual;CHECK(actual.open()==OpenError::None);fake::forcedActual=44;auto p=actual.applyPower(60);CHECK(p.requestedRaw==60&&p.actualRaw==44&&p.actualValid&&!p.error);fake::forcedActual=0;fake::failAt[fake::SetPower]=fake::calls[fake::SetPower]+1;p=actual.applyPower(20);CHECK(p.error&&p.actualValid&&p.actualRaw==44);fake::failAt[fake::GetPower]=fake::calls[fake::GetPower]+2;p=actual.applyPower(20);CHECK(p.error&&!p.actualValid);CHECK(actual.close());restored(actual);++scenarios;
 reset();fake::changeCountryOnDisconnect=true;fake::disconnectedCountry={{'0','1',0},1,11,20,WIFI_COUNTRY_POLICY_AUTO};fake::reconnectImmediate=false;Device drift;CHECK(drift.open()==OpenError::Country);CHECK(drift.active()&&drift.cleanupPending());CHECK(drift.status().countryRestorePending&&drift.status().associationRequested&&!drift.status().associationConfirmed);CHECK(fake::countrySets==0);fake::country={{'U','S',0},1,11,20,WIFI_COUNTRY_POLICY_AUTO};CHECK(drift.retryRestore());CHECK(!drift.active());noWrites();++scenarios;
 reset();fake::changeCountryOnDisconnect=true;fake::disconnectedCountry={{'0','1',0},1,11,13,WIFI_COUNTRY_POLICY_AUTO};Device lowerCountry;CHECK(lowerCountry.open()==OpenError::Country);CHECK(lowerCountry.active()&&lowerCountry.cleanupPending());CHECK(fake::calls[fake::SetPower]==0);CHECK(!lowerCountry.retryRestore());CHECK(fake::calls[fake::SetPower]==0);fake::country={{'U','S',0},1,11,20,WIFI_COUNTRY_POLICY_AUTO};CHECK(lowerCountry.retryRestore());noWrites();++scenarios;
 reset();fake::country.max_tx_power=13;Device independent;CHECK(independent.open()==OpenError::Profile);CHECK(!independent.active());CHECK(independent.status().baselineRaw==80&&independent.status().baselineCountryMaxDbm==13);CHECK(fake::calls[fake::SetPower]==0&&fake::calls[fake::Disconnect]==0&&fake::calls[fake::Connect]==0);CHECK(fake::power==80&&fake::country.max_tx_power==13);noWrites();++scenarios;
 reset();fake::power=32;fake::country.max_tx_power=8;Device nonRoundTrip;CHECK(nonRoundTrip.open()==OpenError::Profile);CHECK(!nonRoundTrip.active()&&fake::calls[fake::SetPower]==0&&fake::power==32);noWrites();++scenarios;
 reset();fake::country.schan=10;fake::country.nchan=4;Device restricted;CHECK(restricted.open()==OpenError::Country);CHECK(!restricted.active());noWrites();++scenarios;
 std::puts("PASS device: frozen original cap survives setter-mutated country max; raw20/44/60/80, capped presets, separate actual/readback errors, AUTO country fail-closed without country setters");
}
static void deliver(const WireFrame&f,const uint8_t*mac,int8_t rssi=-67){uint8_t b[kMaxFrame];const size_t n=encode(f,b,sizeof b);CHECK(n);wifi_pkt_rx_ctrl_t ctrl{};ctrl.rssi=rssi;ctrl.channel=6;esp_now_recv_info_t info{mac,nullptr,&ctrl};const auto cb=receiver.load();CHECK(cb);cb(&info,b,int(n));}
static void pairDevice(Device&d){
 const uint8_t peer[6]={2,0,0,0,0,2};WireFrame f{};f.type=Type::Beacon;f.nonce=2;f.baselineRaw=80;f.powerSave=1;deliver(f,peer);d.tick(millis());CHECK(d.flow().invitePeer(peer,millis()));f.type=Type::Accept;f.session=d.flow().session();deliver(f,peer);d.tick(millis());CHECK(d.flow().state()==State::DistancePause);
}
static void callbacksAndTime(){
 const uint8_t peer[6]={2,0,0,0,0,2};for(uint32_t start:{100u,UINT32_MAX-3u}){reset();fake::timeMs=start;Device d;CHECK(d.open(false,1,start)==OpenError::None);WireFrame b{};b.type=Type::Beacon;b.nonce=2;b.ceilingRaw=80;fake::timeMs=start+1;deliver(b,peer,-77);d.tick(start);CHECK(d.flow().peerCount()==1);CHECK(d.flow().peerAt(0)->lastSeen==start+1);CHECK(d.flow().peerAt(0)->rssi==-77&&d.flow().peerAt(0)->rssiValid);CHECK(d.flow().state()==State::Browsing);CHECK(d.close());++scenarios;}
 reset();Device d;CHECK(d.open()==OpenError::None);uint8_t foreign[kMaxFrame]{};esp_now_recv_info_t foreignInfo{peer,nullptr,nullptr};receiver.load()(&foreignInfo,foreign,sizeof foreign);CHECK(d.snapshot(100).callbacks.invalid==1);device_detail::Datagram foreignOut{};CHECK(!device_detail::mailbox.pop(foreignOut));pairDevice(d);CHECK(d.flow().armManual(Distance::OneMeter,Level::N1,100));WireFrame f{};f.nonce=2;for(unsigned i=0;i<12;++i)deliver(f,peer);esp_now_send_info_t info{peer};senderCallback.load()(&info,ESP_NOW_SEND_SUCCESS);senderCallback.load()(&info,ESP_NOW_SEND_FAIL);d.tick(100);CHECK(d.snapshot(100).callbacks.queueDrops==4);CHECK(d.flow().current().queueOverflow);CHECK(!d.flow().current().telemetryCorrelated);CHECK(d.flow().current().local.macSuccess==1&&d.flow().current().local.macErrors==1);CHECK(d.close());restored(d);++scenarios;
 reset();Device timeout;CHECK(timeout.open()==OpenError::None);pairDevice(timeout);CHECK(timeout.flow().armManual(Distance::TwoPalms,Level::N1,100));CHECK(fake::power==20);fake::timeMs=100+kArmedLimitMs;timeout.tick(100);CHECK(fake::power==80);CHECK(timeout.flow().current().localRestoreAttempted);CHECK(timeout.close());restored(timeout);++scenarios;
 reset();Device stale;CHECK(stale.open()==OpenError::None);pairDevice(stale);CHECK(stale.flow().armManual(Distance::OneMeter,Level::N1,100));WireFrame queued{};queued.nonce=2;deliver(queued,peer);fake::timeMs=100+kDeviceMaximumQueueAgeMs+1;stale.tick(100);CHECK(stale.snapshot(millis()).stale==1);CHECK(stale.flow().current().queueOverflow);CHECK(stale.close());restored(stale);++scenarios;
 reset();Device drift;CHECK(drift.open()==OpenError::None);pairDevice(drift);CHECK(drift.flow().armManual(Distance::OneMeter,Level::N1,100));fake::power=44;fake::country.max_tx_power=11;fake::timeMs=1100;drift.tick(1100);CHECK(drift.status().error==OpenError::Profile);CHECK(!drift.active());restored(drift);++scenarios;
 reset();Device restoreFailed;CHECK(restoreFailed.open()==OpenError::None);pairDevice(restoreFailed);CHECK(restoreFailed.flow().armManual(Distance::OneMeter,Level::N1,100));fake::failures[fake::SetPower]=1;fake::timeMs=100+kHandshakeMs;restoreFailed.tick(millis());CHECK(restoreFailed.flow().state()==State::Fatal);CHECK(restoreFailed.flow().current().reason==Reason::RestoreFailure);CHECK(restoreFailed.active()&&fake::power==20);CHECK(restoreFailed.close());CHECK(fake::power==80);restored(restoreFailed);++scenarios;
 std::puts("PASS device: callback-fresh RSSI/timestamp, outer-loop +1ms and uint32 wrap, bounded inbox overflow invalidation, MAC aggregate telemetry distinct from API/app ACK, independent timeout baseline restoration");
}
static const Report& lastReport(const Device&d){CHECK(d.flow().reportCount());return *d.flow().reportAt(d.flow().reportCount()-1);}
static void failureReports(){
 // An actual user Cancel stays distinct from diagnostic failures.
 reset();Device cancelled;CHECK(cancelled.open()==OpenError::None);pairDevice(cancelled);CHECK(cancelled.flow().armManual(Distance::OneMeter,Level::N1,millis()));CHECK(cancelled.close());CHECK(lastReport(cancelled).reason==Reason::Cancelled&&lastReport(cancelled).localPower.actualRaw==20&&lastReport(cancelled).localPower.error==0);restored(cancelled);++scenarios;
 for(bool cleanupFailure:{false,true}){
  reset();Device d;CHECK(d.open()==OpenError::None);pairDevice(d);CHECK(d.flow().armManual(Distance::OneMeter,Level::N1,millis()));fake::power=44;fake::country.max_tx_power=11;if(cleanupFailure)fake::failures[fake::SetPower]=2;d.tick(millis());
  const auto& saved=lastReport(d);CHECK(saved.reason==Reason::PowerFailure);CHECK(saved.localPower.requestedRaw==20&&saved.localPower.actualRaw==44&&saved.localPower.actualValid&&saved.localPower.error==ESP_ERR_INVALID_STATE);CHECK(!saved.complete&&!saved.local.timingValid&&lossPermille(saved,true)==kUnknown);CHECK(saved.localRestoreAttempted);
  if(cleanupFailure){CHECK(d.active()&&d.cleanupPending()&&saved.localRestore.error);CHECK(d.retryRestore());CHECK(lastReport(d).reason==Reason::PowerFailure&&lastReport(d).localPower.actualRaw==44);}else CHECK(!d.active()&&saved.localRestore.actualRaw==80&&!saved.localRestore.error);
  restored(d);const size_t reports=d.flow().reportCount();CHECK(d.open(false,0xABCD,millis())==OpenError::None);CHECK(d.flow().reportCount()==reports&&lastReport(d).reason==Reason::PowerFailure&&lastReport(d).localPower.actualRaw==44);CHECK(d.close());++scenarios;
 }
 for(unsigned powerReadIndex:{1u,2u}){
  reset();Device d;CHECK(d.open()==OpenError::None);pairDevice(d);CHECK(d.flow().armManual(Distance::OneMeter,Level::N1,millis()));fake::failAt[fake::GetPower]=fake::calls[fake::GetPower]+powerReadIndex;fake::timeMs=1100;d.tick(1100);
  const auto& saved=lastReport(d);CHECK(saved.reason==Reason::PowerFailure&&saved.localPower.requestedRaw==20&&!saved.localPower.actualValid&&saved.localPower.error==-1000-fake::GetPower);CHECK(saved.localRestore.actualRaw==80&&saved.localRestore.actualValid&&!saved.localRestore.error);restored(d);++scenarios;
 }
 for(auto op:{fake::GetMode,fake::GetChannel,fake::GetPS,fake::GetProtocol,fake::GetCountry}){
  reset();Device d;CHECK(d.open()==OpenError::None);pairDevice(d);CHECK(d.flow().armManual(Distance::OneMeter,Level::N1,millis()));fake::failAt[op]=fake::calls[op]+1;fake::timeMs=1100;d.tick(1100);
  const auto& saved=lastReport(d);CHECK(saved.reason==Reason::ConfigMismatch&&saved.configMismatch&&saved.localPower.actualValid&&saved.localPower.actualRaw==20&&saved.localPower.error==-1000-op);CHECK(saved.localRestore.actualValid&&saved.localRestore.actualRaw==80);restored(d);++scenarios;
 }
 for(unsigned field=0;field<3;++field){
  reset();Device d;CHECK(d.open()==OpenError::None);pairDevice(d);CHECK(d.flow().armManual(Distance::OneMeter,Level::N1,millis()));if(field==0)fake::channel=11;else if(field==1)fake::ps=WIFI_PS_NONE;else fake::protocol=3;fake::timeMs=1100;d.tick(1100);const auto& saved=lastReport(d);CHECK(saved.reason==Reason::ConfigMismatch&&saved.configMismatch&&saved.localPower.actualRaw==20&&saved.localPower.error==ESP_ERR_INVALID_STATE);restored(d);++scenarios;
 }
 reset();Device country;CHECK(country.open()==OpenError::None);pairDevice(country);CHECK(country.flow().armManual(Distance::OneMeter,Level::N1,millis()));fake::country.cc[0]='0';fake::country.cc[1]='1';fake::timeMs=1100;country.tick(1100);CHECK(country.active()&&country.cleanupPending());CHECK(lastReport(country).reason==Reason::ConfigMismatch&&lastReport(country).localPower.actualValid&&lastReport(country).localPower.actualRaw==20&&lastReport(country).localPower.error==ESP_ERR_INVALID_STATE);CHECK(lastReport(country).localRestoreAttempted&&lastReport(country).localRestore.error);fake::country.cc[0]='U';fake::country.cc[1]='S';CHECK(country.retryRestore());CHECK(lastReport(country).reason==Reason::ConfigMismatch&&lastReport(country).localPower.actualRaw==20);restored(country);++scenarios;
 reset();Device preflight;CHECK(preflight.open()==OpenError::None);fake::failAt[fake::GetPower]=fake::calls[fake::GetPower]+1;preflight.tick(millis());CHECK(lastReport(preflight).distance==Distance::PairingNear&&lastReport(preflight).reason==Reason::PowerFailure&&!lastReport(preflight).localPower.actualValid);restored(preflight);++scenarios;
 reset();Device maximumDrift;CHECK(maximumDrift.open()==OpenError::None);pairDevice(maximumDrift);CHECK(maximumDrift.flow().armManual(Distance::OneMeter,Level::N1,millis()));const unsigned powerSetters=fake::calls[fake::SetPower];fake::country.max_tx_power=13;fake::timeMs=1100;maximumDrift.tick(1100);CHECK(maximumDrift.status().error==OpenError::Country&&maximumDrift.active()&&maximumDrift.cleanupPending());CHECK(maximumDrift.status().countryRestorePending&&maximumDrift.ceilingRaw()==80);CHECK(lastReport(maximumDrift).reason==Reason::ConfigMismatch&&lastReport(maximumDrift).configMismatch&&lastReport(maximumDrift).localPower.actualRaw==20&&lastReport(maximumDrift).localPower.actualValid&&lastReport(maximumDrift).localPower.error==ESP_ERR_INVALID_STATE);CHECK(fake::power==20&&fake::country.max_tx_power==13&&fake::calls[fake::SetPower]==powerSetters);CHECK(!maximumDrift.retryRestore());CHECK(fake::power==20&&fake::country.max_tx_power==13&&fake::calls[fake::SetPower]==powerSetters);fake::country.max_tx_power=20;CHECK(maximumDrift.retryRestore());CHECK(lastReport(maximumDrift).reason==Reason::ConfigMismatch&&lastReport(maximumDrift).localRestore.error);restored(maximumDrift);++scenarios;
 std::puts("PASS device: persisted PowerFailure/ConfigMismatch causes, fresh actual/error validity, second getter failures, failed cleanup/retry, preflight failures and distinct user Cancel");
}
int main(){happy();exclusion();openFailures();everyOpenCall();closeFailures();countryAndCaps();callbacksAndTime();failureReports();CHECK(allocations.load()==0);std::printf("PASS wifi_radio_device: %u independent scenarios; no C++ allocations; native fake-driver evidence only\n",scenarios);}
