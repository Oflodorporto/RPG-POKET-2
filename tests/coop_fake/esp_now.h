#pragma once
#include "FakeCore.h"
struct wifi_pkt_rx_ctrl_t {int8_t rssi=0;uint8_t channel=0,rate=0;};
struct esp_now_recv_info_t {const uint8_t* src_addr=nullptr;const uint8_t* des_addr=nullptr;const wifi_pkt_rx_ctrl_t* rx_ctrl=nullptr;};
struct esp_now_send_info_t {const uint8_t* des_addr=nullptr;};
enum esp_now_send_status_t {ESP_NOW_SEND_SUCCESS,ESP_NOW_SEND_FAIL};
using SendCallback=void(*)(const esp_now_send_info_t*,esp_now_send_status_t);
inline SendCallback senderCallback=nullptr;
inline int esp_now_register_send_cb(SendCallback cb){if(fake::fail==9)return -1;senderCallback=cb;return ESP_OK;}
inline int esp_now_unregister_send_cb(){senderCallback=nullptr;return ESP_OK;}
struct esp_now_peer_info_t {uint8_t peer_addr[6]{};uint8_t channel=0;int ifidx=0;bool encrypt=false;};
using Receiver=void(*)(const esp_now_recv_info_t*,const uint8_t*,int);
inline Receiver receiver=nullptr;
inline int esp_now_init(){++fake::initCount;if(fake::fail==5)return -1;fake::radio=true;fake::peerCount=0;return ESP_OK;}
inline int esp_now_deinit(){++fake::deinitCount;fake::radio=false;fake::peerCount=0;return ESP_OK;}
inline int esp_now_register_recv_cb(Receiver callback){if(fake::fail==6)return -1;receiver=callback;fake::callbackSet=true;return ESP_OK;}
inline int esp_now_unregister_recv_cb(){receiver=nullptr;fake::callbackSet=false;return ESP_OK;}
inline bool esp_now_is_peer_exist(const uint8_t* mac){for(unsigned i=0;i<fake::peerCount;++i)if(!std::memcmp(mac,fake::peers[i],6))return true;return false;}
inline int esp_now_add_peer(const esp_now_peer_info_t* p){if(fake::peerCount==8||fake::fail==7)return -1;std::memcpy(fake::peers[fake::peerCount++],p->peer_addr,6);return ESP_OK;}
inline int esp_now_del_peer(const uint8_t* mac){for(unsigned i=0;i<fake::peerCount;++i)if(!std::memcmp(mac,fake::peers[i],6)){std::memcpy(fake::peers[i],fake::peers[--fake::peerCount],6);++fake::peerDeletes;return ESP_OK;}return -1;}
inline int esp_now_send(const uint8_t*,const uint8_t*,size_t){++fake::sendCount;return fake::fail==8?-1:ESP_OK;}
