#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <atomic>
constexpr int ESP_OK=0,ESP_FAIL=-1,ESP_ERR_INVALID_ARG=0x102,ESP_ERR_INVALID_STATE=0x103,ESP_ERR_WIFI_NOT_INIT=0x3001,ESP_ERR_WIFI_NOT_STARTED=0x3002;
using esp_err_t=int;
enum wifi_mode_t {WIFI_MODE_NULL,WIFI_MODE_STA,WIFI_MODE_AP,WIFI_MODE_APSTA};
constexpr auto WIFI_STA=WIFI_MODE_STA;constexpr int WIFI_IF_STA=0,WL_CONNECTED=3;
enum wifi_second_chan_t {WIFI_SECOND_CHAN_NONE,WIFI_SECOND_CHAN_ABOVE,WIFI_SECOND_CHAN_BELOW};
enum wifi_storage_t {WIFI_STORAGE_FLASH,WIFI_STORAGE_RAM};
enum wifi_ps_type_t {WIFI_PS_NONE,WIFI_PS_MIN_MODEM,WIFI_PS_MAX_MODEM};
enum wifi_country_policy_t {WIFI_COUNTRY_POLICY_AUTO,WIFI_COUNTRY_POLICY_MANUAL};
struct wifi_country_t {char cc[3];uint8_t schan,nchan;int8_t max_tx_power;wifi_country_policy_t policy;};
struct wifi_ap_record_t {uint8_t bssid[6]{};};
namespace fake {
enum Op {None,Mode,Storage,GetChannel,SetChannel,GetPS,SetPS,GetPower,SetPower,GetProtocol,SetProtocol,GetCountry,GetMode,GetMac,Disconnect,Connect,Auto,Init,RegisterRx,RegisterTx,UnregisterRx,UnregisterTx,Deinit,AddPeer,DeletePeer,Send,GetAP,Count};
inline unsigned calls[Count]{}, failAt[Count]{}, failures[Count]{};
inline wifi_mode_t mode=WIFI_MODE_STA;inline wifi_storage_t storage=WIFI_STORAGE_FLASH;
inline wifi_ps_type_t ps=WIFI_PS_MIN_MODEM;inline wifi_second_chan_t secondary=WIFI_SECOND_CHAN_ABOVE;
inline wifi_country_t country={{'U','S',0},1,11,20,WIFI_COUNTRY_POLICY_AUTO},disconnectedCountry=country;
inline bool autoReconnect=true,connected=true,persistent=true,radio=false,reconnectImmediate=true,changeCountryOnDisconnect=false;
inline uint8_t channel=11,protocol=7,own[6]={2,0,0,0,0,1},bssid[6]={2,0,0,0,9,9},peers[8][6]{};inline unsigned peerCount=0;
inline int8_t power=80,forcedActual=0;inline unsigned mutations=0,nvsWrites=0,erases=0,countrySets=0,credentialsWrites=0;
inline std::atomic<uint32_t> timeMs{100};
inline int operation(Op op){++calls[op];if(failAt[op]==calls[op]||failures[op]){if(failures[op])--failures[op];return -1000-int(op);}return ESP_OK;}
inline void reset(){std::memset(calls,0,sizeof calls);std::memset(failAt,0,sizeof failAt);std::memset(failures,0,sizeof failures);mode=WIFI_MODE_STA;storage=WIFI_STORAGE_FLASH;ps=WIFI_PS_MIN_MODEM;secondary=WIFI_SECOND_CHAN_ABOVE;country={{'U','S',0},1,11,20,WIFI_COUNTRY_POLICY_AUTO};disconnectedCountry=country;autoReconnect=connected=persistent=reconnectImmediate=true;radio=changeCountryOnDisconnect=false;channel=11;protocol=7;power=80;forcedActual=0;peerCount=mutations=nvsWrites=erases=countrySets=credentialsWrites=0;timeMs=100;}
inline int powered(Op op){const int e=operation(op);return e?e:mode==WIFI_MODE_NULL?ESP_ERR_WIFI_NOT_STARTED:ESP_OK;}
}
