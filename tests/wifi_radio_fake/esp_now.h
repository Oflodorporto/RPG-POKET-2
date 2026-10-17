#pragma once
#include "FakeCore.h"
struct wifi_pkt_rx_ctrl_t {int8_t rssi=0;uint8_t channel=0,rate=0;};
struct esp_now_recv_info_t {const uint8_t* src_addr=nullptr;const uint8_t* des_addr=nullptr;const wifi_pkt_rx_ctrl_t* rx_ctrl=nullptr;};
struct esp_now_send_info_t {const uint8_t* des_addr=nullptr;};
enum esp_now_send_status_t {ESP_NOW_SEND_SUCCESS,ESP_NOW_SEND_FAIL};
using SendCallback=void(*)(const esp_now_send_info_t*,esp_now_send_status_t);
using Receiver=void(*)(const esp_now_recv_info_t*,const uint8_t*,int);
inline std::atomic<SendCallback> senderCallback{nullptr};inline std::atomic<Receiver> receiver{nullptr};
struct esp_now_peer_info_t {uint8_t peer_addr[6]{};uint8_t channel=0;int ifidx=0;bool encrypt=false;};
inline int esp_now_init(){const int e=fake::operation(fake::Init);if(!e){fake::radio=true;fake::peerCount=0;}return e;}
inline int esp_now_deinit(){const int e=fake::operation(fake::Deinit);if(!e){fake::radio=false;fake::peerCount=0;receiver=nullptr;senderCallback=nullptr;}return e;}
inline int esp_now_register_recv_cb(Receiver f){const int e=fake::operation(fake::RegisterRx);if(!e)receiver=f;return e;}
inline int esp_now_register_send_cb(SendCallback f){const int e=fake::operation(fake::RegisterTx);if(!e)senderCallback=f;return e;}
inline int esp_now_unregister_recv_cb(){const int e=fake::operation(fake::UnregisterRx);if(!e)receiver=nullptr;return e;}
inline int esp_now_unregister_send_cb(){const int e=fake::operation(fake::UnregisterTx);if(!e)senderCallback=nullptr;return e;}
inline bool esp_now_is_peer_exist(const uint8_t* p){for(unsigned i=0;i<fake::peerCount;++i)if(!std::memcmp(fake::peers[i],p,6))return true;return false;}
inline int esp_now_add_peer(const esp_now_peer_info_t* p){const int e=fake::operation(fake::AddPeer);if(e)return e;if(fake::peerCount==8)return ESP_FAIL;std::memcpy(fake::peers[fake::peerCount++],p->peer_addr,6);return ESP_OK;}
inline int esp_now_del_peer(const uint8_t* p){const int e=fake::operation(fake::DeletePeer);if(e)return e;for(unsigned i=0;i<fake::peerCount;++i)if(!std::memcmp(fake::peers[i],p,6)){std::memcpy(fake::peers[i],fake::peers[--fake::peerCount],6);return ESP_OK;}return ESP_FAIL;}
namespace fake {inline int (*sendObserver)(const uint8_t*,const uint8_t*,size_t)=nullptr;}
inline int esp_now_send(const uint8_t*mac,const uint8_t*bytes,size_t n){const int error=fake::operation(fake::Send);return error?error:fake::sendObserver?fake::sendObserver(mac,bytes,n):ESP_OK;}
