#pragma once
#include "WifiRadioTestConfig.h"
#include "WifiRadioTestFlow.h"
#include "Coop/RadioOwner.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <cstring>
#include <cstdint>

namespace wifi_radio_test {
enum class OpenError:uint8_t {None,Busy,Profile,Country,WifiMode,WifiStorage,Channel,Mac,Radio,Callback,Flow,Restore,CallbackBusy};
struct DeviceStatus {
 OpenError error=OpenError::None;int32_t apiError=0,restoreError=0;
 bool cleanupPending=false,opened=false,baselineValid=false,currentValid=false,originalProfileValid=false;
 bool originalConnected=false,originalPersistent=true,originalAutoReconnect=false;
 bool associationRequested=false,associationConfirmed=false,countryRestorePending=false;
 bool storageFlashAssumed=true;int8_t usbPhyCompiled=kUsbPhyCompiled,reduceTxPowerCompiled=kReduceTxPowerCompiled;
 uint8_t originalMode=0,channel=0,secondary=0,ps=0,protocol=0,mac[6]={};
 int8_t baselineRaw=0,currentRaw=0,ceilingRaw=0,baselineCountryMaxDbm=0;char country[3]={};uint32_t resetReason=0;
};
struct CallbackStats {uint32_t received=0,invalid=0,queueDrops=0,macSuccess=0,macFailure=0;};
struct DeviceSnapshot {DeviceStatus status;CallbackStats callbacks;uint32_t now=0,stale=0;};
namespace device_detail {
inline void increment(uint32_t&v){if(v!=UINT32_MAX)++v;}
struct Datagram {uint32_t generation=0,at=0;uint16_t size=0;uint8_t mac[6]={},bytes[kMaxFrame]={};int8_t rssi=0;bool rssiValid=false;};
// Entire callback lifetime is static. No callback touches a Device/Flow, heap,
// Arduino String, driver peer table or task queue. Critical sections perform
// bounded memory/scalar work only. Main-loop teardown fences first, then waits
// non-blockingly for in-flight copies before calling unregister/deinit.
class Mailbox {
 portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
 uint32_t generation=0,inFlight=0;unsigned head=0,count=0;Datagram frames[kDeviceInbox]{};CallbackStats stats{};
public:
 uint32_t enter(){portENTER_CRITICAL(&mux);const uint32_t token=generation;if(token)++inFlight;portEXIT_CRITICAL(&mux);return token;}
 void leave(uint32_t token){if(!token)return;portENTER_CRITICAL(&mux);if(inFlight)--inFlight;portEXIT_CRITICAL(&mux);}
 void receive(uint32_t token,const Datagram* d){portENTER_CRITICAL(&mux);if(token&&generation==token){increment(stats.received);if(!d)increment(stats.invalid);else if(count==kDeviceInbox)increment(stats.queueDrops);else{frames[(head+count)%kDeviceInbox]=*d;++count;}}portEXIT_CRITICAL(&mux);}
 void sent(uint32_t token,bool success){portENTER_CRITICAL(&mux);if(token&&generation==token)increment(success?stats.macSuccess:stats.macFailure);portEXIT_CRITICAL(&mux);}
 bool pop(Datagram&out){portENTER_CRITICAL(&mux);const bool yes=count!=0;if(yes){out=frames[head];head=(head+1)%kDeviceInbox;--count;}portEXIT_CRITICAL(&mux);return yes;}
 void fence(){portENTER_CRITICAL(&mux);generation=0;portEXIT_CRITICAL(&mux);}
 bool idle(){portENTER_CRITICAL(&mux);const bool yes=inFlight==0;portEXIT_CRITICAL(&mux);return yes;}
 bool publish(uint32_t token){portENTER_CRITICAL(&mux);const bool yes=inFlight==0&&!generation;if(yes){head=count=0;stats={};generation=token;}portEXIT_CRITICAL(&mux);return yes;}
 CallbackStats snapshot(){portENTER_CRITICAL(&mux);const auto copy=stats;portEXIT_CRITICAL(&mux);return copy;}
};
static_assert(sizeof(Mailbox)<=2048,"Diagnostic callback inbox exceeded fixed RAM budget");
static_assert(sizeof(Datagram)<=256,"Diagnostic datagram exceeded fixed frame budget");
inline Mailbox mailbox;
inline uint32_t nextGeneration=0;
inline void receiveCallback(const esp_now_recv_info_t*info,const uint8_t*bytes,int size){
 const uint32_t token=mailbox.enter();if(!token)return;
 if(!info||!info->src_addr||!bytes||size<6||size>int(kMaxFrame)||
    bytes[0]!=uint8_t(kMagic)||bytes[1]!=uint8_t(kMagic>>8)||bytes[2]!=uint8_t(kMagic>>16)||bytes[3]!=uint8_t(kMagic>>24)||bytes[4]!=kVersion){mailbox.receive(token,nullptr);mailbox.leave(token);return;}
 Datagram d{};d.generation=token;d.at=millis();d.size=uint16_t(size);std::memcpy(d.mac,info->src_addr,6);std::memcpy(d.bytes,bytes,size);
 if(info->rx_ctrl){d.rssi=info->rx_ctrl->rssi;d.rssiValid=true;}
 mailbox.receive(token,&d);mailbox.leave(token);
}
inline void sendCallback(const esp_now_send_info_t*info,esp_now_send_status_t status){
 const uint32_t token=mailbox.enter();if(!token)return;
 if(info&&info->des_addr)mailbox.sent(token,status==ESP_NOW_SEND_SUCCESS);
 mailbox.leave(token);
}
// Arduino 3.3.12 exposes persistence as protected static state but no getter.
// A derived accessor samples it without changing the WiFi singleton or ABI.
struct Persistence:WiFiGenericClass {static bool get(){return _persistent;}};
inline bool sameCountryIdentity(const wifi_country_t&a,const wifi_country_t&b){return std::memcmp(a.cc,b.cc,3)==0&&a.schan==b.schan&&a.nchan==b.nchan&&a.policy==b.policy;}
inline bool sameCountry(const wifi_country_t&a,const wifi_country_t&b){return sameCountryIdentity(a,b)&&a.max_tx_power==b.max_tx_power;}
struct Profile {wifi_mode_t mode=WIFI_MODE_NULL;uint8_t channel=0,protocol=0;wifi_second_chan_t secondary=WIFI_SECOND_CHAN_NONE;wifi_ps_type_t ps=WIFI_PS_NONE;int8_t power=0;wifi_country_t country{};};
enum class ProfileField:uint8_t {None,Mode,Channel,PowerSave,Protocol,Power,Country};
inline int32_t readProfile(Profile&p,ProfileField*failed=nullptr){
 auto result=[&](int32_t error,ProfileField field){if(failed)*failed=error?field:ProfileField::None;return error;};
 int32_t e=esp_wifi_get_mode(&p.mode);if(e)return result(e,ProfileField::Mode);
 e=esp_wifi_get_channel(&p.channel,&p.secondary);if(e)return result(e,ProfileField::Channel);
 e=esp_wifi_get_ps(&p.ps);if(e)return result(e,ProfileField::PowerSave);
 e=esp_wifi_get_protocol(WIFI_IF_STA,&p.protocol);if(e)return result(e,ProfileField::Protocol);
 e=esp_wifi_get_max_tx_power(&p.power);if(e)return result(e,ProfileField::Power);
 return result(esp_wifi_get_country(&p.country),ProfileField::Country);
}
}

class Device {
 Flow flow_{};DeviceStatus status_{};device_detail::Profile original_{},baseline_{};
 bool lease_=false,operational_=false,touched_=false,profileValid_=false,radio_=false,rx_=false,tx_=false,flowBegun_=false;
 int8_t expectedPower_=0;bool expectedPowerValid_=false;
 bool reconnectRequested_=false,countryDrift_=false,countryPowerDrift_=false;uint32_t generation_=0,polledAt_=0,stale_=0;
 uint8_t originalBssid_[6]={},peers_[8][6]={};unsigned peerCount_=0,evict_=0;CallbackStats consumed_{};
 static uint32_t nowHook(void*){return millis();}
 static int32_t sendHook(void*context,const uint8_t*mac,const uint8_t*bytes,size_t size){return static_cast<Device*>(context)->send(mac,bytes,size);}
 static PowerResult applyHook(void*context,int8_t raw){return static_cast<Device*>(context)->applyPower(raw);}
 static PowerResult restoreHook(void*context){return static_cast<Device*>(context)->restoreBaseline();}
 int32_t send(const uint8_t*mac,const uint8_t*bytes,size_t size){
  if(!operational_||coopradio::owner!=coopradio::Owner::WifiTest||!mac||!bytes||!size||size>kMaxFrame)return ESP_ERR_INVALID_STATE;
  if(!esp_now_is_peer_exist(mac)){
   if(peerCount_==8){unsigned slot=evict_++%8;if(peers_[slot][0]&1)slot=(slot+1)%8;const int e=esp_now_del_peer(peers_[slot]);if(e)return e;std::memcpy(peers_[slot],peers_[--peerCount_],6);}
   esp_now_peer_info_t peer{};std::memcpy(peer.peer_addr,mac,6);peer.channel=kDeviceChannel;peer.ifidx=WIFI_IF_STA;peer.encrypt=false;const int e=esp_now_add_peer(&peer);if(e)return e;std::memcpy(peers_[peerCount_++],mac,6);
  }
  return esp_now_send(mac,bytes,size);
 }
 PowerResult readPower(int8_t requested,int32_t setterError=0){PowerResult r{};r.requestedRaw=requested;const int e=esp_wifi_get_max_tx_power(&r.actualRaw);r.actualValid=e==ESP_OK;r.error=setterError?setterError:e;status_.currentValid=r.actualValid;status_.currentRaw=r.actualRaw;return r;}
 PowerResult setPower(int8_t raw,bool restoring){
  PowerResult bad{};bad.requestedRaw=raw;bad.error=ESP_ERR_INVALID_STATE;
  if(!lease_||!profileValid_||coopradio::owner!=coopradio::Owner::WifiTest)return bad;
  wifi_country_t current{};const int e=esp_wifi_get_country(&current);if(e)return readPower(raw,e);
  if(!device_detail::sameCountryIdentity(current,baseline_.country)||(countryPowerDrift_&&!device_detail::sameCountry(current,baseline_.country))){
   // AUTO country follows the AP. Never override it or increase the cap with a
   // country setter. Diagnostic entry/operation fails closed on that change.
   countryDrift_=true;status_.error=OpenError::Country;return readPower(raw,ESP_ERR_INVALID_STATE);
  }
  bool supported=raw==baseline_.power;for(auto level:kDevicePresetRaw)if(raw==level)supported=true;
  if(!restoring&&!supported)return readPower(raw,ESP_ERR_INVALID_ARG);
  // Country's integer field is lossy: documented raw34 -> 8 and raw66 -> 16.
  // Preserve Atual exactly, but never infer a higher ceiling for N1..N4.
  const bool currentBaseline=!restoring&&raw==baseline_.power&&flow_.state()==State::Armed&&flow_.selectedLevel()==Level::Baseline;
  const int8_t applied=!restoring&&!currentBaseline&&raw>status_.ceilingRaw?status_.ceilingRaw:raw;
  int8_t previous=0;const int query=esp_wifi_get_max_tx_power(&previous);if(query)return readPower(raw,query);
  const int set=previous==applied?ESP_OK:esp_wifi_set_max_tx_power(applied);PowerResult result=readPower(raw,set);
  if(result.actualValid){expectedPower_=result.actualRaw;expectedPowerValid_=true;}
  if(result.actualValid&&!result.error&&((restoring&&result.actualRaw!=raw)||(!restoring&&!currentBaseline&&result.actualRaw>status_.ceilingRaw)))result.error=ESP_ERR_INVALID_STATE;
  return result;
 }
 OpenError fail(OpenError error,int32_t api=ESP_FAIL){status_.error=error;status_.apiError=api;close();return error;}
 void runtimeFailure(OpenError error,Reason reason,int32_t code,int8_t actual,bool valid){
  PowerResult observed{};observed.requestedRaw=flow_.powerArmed()?flow_.current().localPower.requestedRaw:baseline_.power;
  observed.actualRaw=actual;observed.actualValid=valid;observed.error=code;
  status_.currentRaw=actual;status_.currentValid=valid;
  flow_.deviceFailure(reason,observed,millis());
  // The failure path has already ended the measurement and attempted its
  // baseline restoration. Full teardown must not relabel it as a user Cancel.
  flowBegun_=false;fail(error,code);
 }
 void exposeProfile(const device_detail::Profile&p){status_.channel=p.channel;status_.secondary=uint8_t(p.secondary);status_.ps=uint8_t(p.ps);status_.protocol=p.protocol;status_.currentRaw=p.power;status_.currentValid=true;std::memcpy(status_.country,p.country.cc,3);}
 bool restoreWifi(){
  if(!touched_)return true;
  // A previous unsuccessful cleanup may already have restored this Arduino
  // preference. Re-enter RAM initialization before any retry can start Wi-Fi.
  WiFi.persistent(false);
  int32_t error=0;auto record=[&](int32_t e){if(e&&!error)error=e;};
  // No IDF storage getter exists. This application's radio-ownership boundary
  // is FLASH storage; only this adapter owns its temporary RAM interval. Never
  // call begin(credentials), set_config, erase, resumeConnection or prefs/save.
  if(original_.mode==WIFI_MODE_NULL){
   if(WiFi.getMode()!=WIFI_MODE_NULL&&!WiFi.mode(WIFI_MODE_NULL))record(ESP_FAIL);
   if(WiFi.getMode()!=WIFI_MODE_NULL)record(ESP_ERR_INVALID_STATE);
   else status_.currentValid=false;
  }else if(profileValid_){
   if(WiFi.getMode()!=original_.mode&&!WiFi.mode(original_.mode))record(ESP_FAIL);
   const int ram=esp_wifi_set_storage(WIFI_STORAGE_RAM);
   if(ram){status_.restoreError=ram;return false;}
   device_detail::Profile current{};int32_t e=device_detail::readProfile(current);record(e);
   if(!e&&(!device_detail::sameCountryIdentity(current.country,original_.country)||(countryPowerDrift_&&!device_detail::sameCountry(current.country,original_.country)))){
    countryDrift_=true;status_.countryRestorePending=true;
    // Do not raise the old power under a newly reverted AUTO country. The only
    // supported restoration route is the original association, never a country
    // setter. An accepted reconnect is not evidence this profile is restored.
    if(status_.originalConnected&&!reconnectRequested_&&WiFi.status()!=WL_CONNECTED){const int ce=esp_wifi_connect();record(ce);if(!ce)reconnectRequested_=status_.associationRequested=true;}
    if(!WiFi.setAutoReconnect(status_.originalAutoReconnect))record(ESP_FAIL);
    const int ce=device_detail::readProfile(current);record(ce);
    if(ce||!device_detail::sameCountryIdentity(current.country,original_.country)||(countryPowerDrift_&&!device_detail::sameCountry(current.country,original_.country))){
     status_.restoreError=error?error:ESP_ERR_INVALID_STATE;return false;
    }
    status_.countryRestorePending=false;
   }
   if(!e){
    if(current.ps!=original_.ps)record(esp_wifi_set_ps(original_.ps));
    if(current.protocol!=original_.protocol)record(esp_wifi_set_protocol(WIFI_IF_STA,original_.protocol));
    if(current.channel!=original_.channel||current.secondary!=original_.secondary)record(esp_wifi_set_channel(original_.channel,original_.secondary));
    if(current.power!=original_.power)record(esp_wifi_set_max_tx_power(original_.power));
   }
   e=device_detail::readProfile(current);record(e);
   if(!e){
    exposeProfile(current);
    if(current.mode!=original_.mode||current.channel!=original_.channel||current.secondary!=original_.secondary||current.ps!=original_.ps||current.protocol!=original_.protocol||current.power!=original_.power)record(ESP_ERR_INVALID_STATE);
    if(!device_detail::sameCountry(current.country,original_.country)){
     status_.countryRestorePending=true;record(ESP_ERR_INVALID_STATE);
    }else status_.countryRestorePending=false;
   }
   if(status_.originalConnected&&!reconnectRequested_&&WiFi.status()!=WL_CONNECTED){const int e2=esp_wifi_connect();record(e2);if(!e2)reconnectRequested_=status_.associationRequested=true;}
   if(status_.originalConnected){wifi_ap_record_t ap{};status_.associationConfirmed=esp_wifi_sta_get_ap_info(&ap)==ESP_OK&&std::memcmp(ap.bssid,originalBssid_,6)==0;}
   record(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
  }
  if(!WiFi.setAutoReconnect(status_.originalAutoReconnect))record(ESP_FAIL);
  if(WiFi.getAutoReconnect()!=status_.originalAutoReconnect)record(ESP_ERR_INVALID_STATE);
  WiFi.persistent(status_.originalPersistent);
  if(device_detail::Persistence::get()!=status_.originalPersistent)record(ESP_ERR_INVALID_STATE);
  status_.restoreError=error;if(error)return false;touched_=false;return true;
 }
public:
 Device()=default;Device(const Device&)=delete;Device&operator=(const Device&)=delete;
 Flow& flow(){return flow_;}const Flow&flow()const{return flow_;}
 const DeviceStatus& status()const{return status_;}
 bool active()const{return lease_;}bool cleanupPending()const{return status_.cleanupPending;}
 const uint8_t*localMac()const{return status_.mac;}int8_t baselineRaw()const{return status_.baselineRaw;}int8_t ceilingRaw()const{return status_.ceilingRaw;}
 DeviceSnapshot snapshot(uint32_t now)const{DeviceSnapshot s{};s.status=status_;s.callbacks=device_detail::mailbox.snapshot();s.now=now;s.stale=stale_;return s;}
 PowerResult applyPower(int8_t raw){return setPower(raw,false);}
 PowerResult restoreBaseline(){return setPower(baseline_.power,true);}
 OpenError open(bool blocked=false,uint32_t nonce=0,uint32_t now=0){
  if(lease_)return OpenError::Busy;
  if(blocked||coopradio::owner!=coopradio::Owner::None){status_.error=OpenError::Busy;return status_.error;}
  const auto mode=WiFi.getMode();if(mode!=WIFI_MODE_NULL&&mode!=WIFI_MODE_STA){status_.error=OpenError::WifiMode;return status_.error;}
  if(device_detail::nextGeneration==UINT32_MAX||!device_detail::mailbox.idle()||!coopradio::acquire(coopradio::Owner::WifiTest))return OpenError::Busy;
  lease_=true;status_={};status_.resetReason=uint32_t(esp_reset_reason());status_.originalMode=uint8_t(mode);status_.originalConnected=WiFi.status()==WL_CONNECTED;status_.originalAutoReconnect=WiFi.getAutoReconnect();status_.originalPersistent=device_detail::Persistence::get();
  original_={};original_.mode=mode;baseline_={};profileValid_=touched_=operational_=radio_=rx_=tx_=flowBegun_=false;reconnectRequested_=countryDrift_=countryPowerDrift_=false;peerCount_=evict_=0;stale_=0;consumed_={};
  if(mode!=WIFI_MODE_NULL){const int e=device_detail::readProfile(original_);if(e)return fail(OpenError::Profile,e);profileValid_=status_.originalProfileValid=true;baseline_=original_;if(status_.originalConnected){wifi_ap_record_t ap{};const int ae=esp_wifi_sta_get_ap_info(&ap);if(ae)return fail(OpenError::Profile,ae);std::memcpy(originalBssid_,ap.bssid,6);}}
  touched_=true;WiFi.persistent(false);if(!WiFi.setAutoReconnect(false))return fail(OpenError::Profile);
  if(mode!=WIFI_MODE_NULL){const int e=esp_wifi_set_storage(WIFI_STORAGE_RAM);if(e)return fail(OpenError::WifiStorage,e);}
  if(!WiFi.mode(WIFI_MODE_STA))return fail(OpenError::WifiMode);
  {const int e=esp_wifi_set_storage(WIFI_STORAGE_RAM);if(e)return fail(OpenError::WifiStorage,e);}
  if(mode==WIFI_MODE_NULL){const int e=device_detail::readProfile(baseline_);if(e)return fail(OpenError::Profile,e);profileValid_=true;}
  status_.baselineRaw=baseline_.power;status_.baselineValid=true;status_.baselineCountryMaxDbm=baseline_.country.max_tx_power;expectedPower_=baseline_.power;expectedPowerValid_=true;exposeProfile(baseline_);
  // Some legitimate driver/regulatory profiles cannot be reconstructed by the
  // allowed setters. Reject this diagnostic profile before changing its power,
  // rather than guessing a country cap or later requiring set_country/NVS.
  const int8_t restorableRaw[]={8,20,28,34,44,52,56,60,66,72,80};bool roundTrips=false;
  for(int8_t raw:restorableRaw)if(raw==baseline_.power&&baseline_.country.max_tx_power==raw/4)roundTrips=true;
  if(!roundTrips)return fail(OpenError::Profile,ESP_ERR_INVALID_STATE);
  int cap=baseline_.country.max_tx_power*4;if(cap>kDeviceMaximumRaw)cap=kDeviceMaximumRaw;
#ifdef CONFIG_ESP_PHY_MAX_WIFI_TX_POWER
  if(cap>CONFIG_ESP_PHY_MAX_WIFI_TX_POWER*4)cap=CONFIG_ESP_PHY_MAX_WIFI_TX_POWER*4;
#endif
  if(cap<8||kDeviceChannel<baseline_.country.schan||kDeviceChannel>=unsigned(baseline_.country.schan)+baseline_.country.nchan)return fail(OpenError::Country,ESP_ERR_INVALID_ARG);
  status_.ceilingRaw=int8_t(cap);
  if(!WiFi.disconnect(false,false,100))return fail(OpenError::Profile);
  wifi_country_t after{};{const int e=esp_wifi_get_country(&after);if(e)return fail(OpenError::Country,e);}
  if(!device_detail::sameCountry(after,baseline_.country)){countryDrift_=true;return fail(OpenError::Country,ESP_ERR_INVALID_STATE);}
  {const int e=esp_wifi_set_channel(kDeviceChannel,WIFI_SECOND_CHAN_NONE);if(e)return fail(OpenError::Channel,e);}
  device_detail::Profile check{};{const int e=device_detail::readProfile(check);if(e)return fail(OpenError::Profile,e);}
  if(check.mode!=WIFI_MODE_STA||check.channel!=kDeviceChannel||check.secondary!=WIFI_SECOND_CHAN_NONE||check.ps!=baseline_.ps||check.protocol!=baseline_.protocol||check.power!=baseline_.power)return fail(OpenError::Profile,ESP_ERR_INVALID_STATE);
  exposeProfile(check);{const int e=esp_wifi_get_mac(WIFI_IF_STA,status_.mac);if(e||!validMac(status_.mac))return fail(OpenError::Mac,e?e:ESP_ERR_INVALID_STATE);}
  {const int e=esp_now_init();if(e)return fail(OpenError::Radio,e);}radio_=true;
  generation_=++device_detail::nextGeneration;if(!device_detail::mailbox.publish(generation_))return fail(OpenError::CallbackBusy);
  {const int e=esp_now_register_recv_cb(device_detail::receiveCallback);if(e)return fail(OpenError::Callback,e);}rx_=true;
  {const int e=esp_now_register_send_cb(device_detail::sendCallback);if(e)return fail(OpenError::Callback,e);}tx_=true;
  operational_=true;flow_.setRadioConfig(kDeviceChannel,baseline_.protocol,uint8_t(baseline_.ps));flow_.setHooks(Hooks{this,nowHook,sendHook,applyHook,restoreHook});PowerResult baseline{};baseline.requestedRaw=baseline.actualRaw=baseline_.power;baseline.actualValid=true;
  if(!nonce){nonce=esp_random();if(!nonce)nonce=1;}if(!now)now=millis();
  if(!flow_.begin(status_.mac,nonce,baseline,status_.ceilingRaw,now))return fail(OpenError::Flow);
  flowBegun_=true;status_.opened=true;polledAt_=millis();status_.error=OpenError::None;return OpenError::None;
 }
 void tick(uint32_t /*outerNow*/){
  if(!operational_||!lease_)return;
  const uint32_t beforeDrain=millis();
  // Verify power before processing samples or sending the next probe. The
  // readback is the configured maximum, not a measurement of radiated output.
  int8_t actual=0;const int pe=esp_wifi_get_max_tx_power(&actual);
  if(pe||(expectedPowerValid_&&actual!=expectedPower_)){runtimeFailure(OpenError::Profile,Reason::PowerFailure,pe?pe:ESP_ERR_INVALID_STATE,actual,pe==ESP_OK);return;}
  status_.currentRaw=actual;status_.currentValid=true;
  if(uint32_t(beforeDrain-polledAt_)>=1000){
   polledAt_=beforeDrain;device_detail::Profile current{};device_detail::ProfileField field{};const int e=device_detail::readProfile(current,&field);
   if(e){const bool powerFailed=field==device_detail::ProfileField::Power;runtimeFailure(OpenError::Profile,powerFailed?Reason::PowerFailure:Reason::ConfigMismatch,e,powerFailed?0:actual,!powerFailed);return;}exposeProfile(current);
   if(expectedPowerValid_&&current.power!=expectedPower_){runtimeFailure(OpenError::Profile,Reason::PowerFailure,ESP_ERR_INVALID_STATE,current.power,true);return;}
   if(current.mode!=WIFI_MODE_STA||current.channel!=kDeviceChannel||current.secondary!=WIFI_SECOND_CHAN_NONE||current.ps!=baseline_.ps||current.protocol!=baseline_.protocol){runtimeFailure(OpenError::Profile,Reason::ConfigMismatch,ESP_ERR_INVALID_STATE,current.power,true);return;}
   if(!device_detail::sameCountryIdentity(current.country,baseline_.country)){countryDrift_=true;runtimeFailure(OpenError::Country,Reason::ConfigMismatch,ESP_ERR_INVALID_STATE,current.power,true);return;}
   // A preset changes these two readbacks together under the documented IDF
   // mapping. An isolated country maximum change is an unexplained regulatory
   // profile change, not permission to raise power using the frozen ceiling.
   if(current.country.max_tx_power!=current.power/4){countryPowerDrift_=true;runtimeFailure(OpenError::Country,Reason::ConfigMismatch,ESP_ERR_INVALID_STATE,current.power,true);return;}
  }
  auto consumeCounters=[&](){const auto counters=device_detail::mailbox.snapshot();
   if(counters.queueDrops==UINT32_MAX||counters.queueDrops!=consumed_.queueDrops)flow_.noteQueueOverflow();
   flow_.onMacSendCounts(counters.macSuccess-consumed_.macSuccess,counters.macFailure-consumed_.macFailure,millis());consumed_=counters;
  };
  consumeCounters();device_detail::Datagram d{};
  for(unsigned i=0;i<kDeviceInbox&&device_detail::mailbox.pop(d);++i){
   if(d.generation!=generation_){device_detail::increment(stale_);continue;}
   const uint32_t drainedAt=millis();
   if(timeAtOrAfter(drainedAt,d.at)&&uint32_t(drainedAt-d.at)>kDeviceMaximumQueueAgeMs){device_detail::increment(stale_);flow_.noteQueueOverflow();continue;}
   flow_.receive(d.mac,d.bytes,d.size,d.rssi,d.rssiValid,d.at);
  }
  consumeCounters();
  // A Wi-Fi callback can occur one millisecond after the outer loop sampled its
  // clock. A fresh post-drain timestamp prevents underflow/immediate timeouts.
  flow_.tick(millis());
 }

 bool close(){
  if(!lease_)return true;
  if(coopradio::owner!=coopradio::Owner::WifiTest)return false;
  if(operational_&&flowBegun_){flow_.cancel(millis());flowBegun_=false;}
  operational_=false;status_.opened=false;status_.cleanupPending=true;device_detail::mailbox.fence();
  if(!device_detail::mailbox.idle()){status_.restoreError=ESP_ERR_INVALID_STATE;return false;}
  int32_t e=0;if(rx_){const int r=esp_now_unregister_recv_cb();if(!r)rx_=false;else e=r;}if(tx_){const int r=esp_now_unregister_send_cb();if(!r)tx_=false;else if(!e)e=r;}
  if(radio_){const int r=esp_now_deinit();if(!r){radio_=rx_=tx_=false;}else if(!e)e=r;}
  if(e){status_.restoreError=e;return false;}
  if(!restoreWifi())return false;
  flow_.acknowledgeCleanup(true);flow_.setHooks(Hooks{});generation_=0;peerCount_=0;status_.cleanupPending=false;status_.restoreError=0;coopradio::release(coopradio::Owner::WifiTest);lease_=false;return true;
 }
 bool retryRestore(){return close();}
};
static_assert(sizeof(Device)<=8192,"Diagnostic Device exceeded fixed RAM budget");
} // namespace wifi_radio_test
