#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
constexpr int ESP_OK = 0;
struct esp_now_recv_info_t { const uint8_t* src_addr; };
struct esp_now_peer_info_t { uint8_t peer_addr[6]{}; int channel=0,ifidx=0; };
struct ClubRadioSent { uint8_t mac[6]; std::vector<uint8_t> bytes; };
inline std::vector<ClubRadioSent> clubRadioSent;
using ClubReceiveCallback = void (*)(const esp_now_recv_info_t*,const uint8_t*,int);
inline ClubReceiveCallback clubRadioReceiver=nullptr;
inline int esp_now_init(){return ESP_OK;}
inline int esp_now_deinit(){return ESP_OK;}
inline int esp_now_register_recv_cb(ClubReceiveCallback cb){clubRadioReceiver=cb;return ESP_OK;}
inline int esp_now_unregister_recv_cb(){clubRadioReceiver=nullptr;return ESP_OK;}
inline bool esp_now_is_peer_exist(const uint8_t*){return true;}
inline int esp_now_add_peer(const esp_now_peer_info_t*){return ESP_OK;}
inline int esp_now_send(const uint8_t* mac,const uint8_t* bytes,unsigned size){ClubRadioSent sent;memcpy(sent.mac,mac,6);sent.bytes={bytes,bytes+size};clubRadioSent.push_back(sent);return ESP_OK;}
