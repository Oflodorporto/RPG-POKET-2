#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
constexpr int ESP_OK=0;
struct esp_now_recv_info_t {const uint8_t* src_addr;};
inline bool radioFail=false;
using RadioSink=void (*)(const uint8_t*,const uint8_t*,size_t);
inline RadioSink radioSink=nullptr;
inline int esp_now_send(const uint8_t* mac,const uint8_t* bytes,size_t size){if(radioFail)return -1;if(radioSink)radioSink(mac,bytes,size);return ESP_OK;}
