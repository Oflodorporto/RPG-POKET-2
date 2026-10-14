#pragma once
#include "FakeCore.h"
inline int esp_wifi_set_storage(wifi_storage_t storage){if(fake::fail==2)return -1;fake::storage=storage;return ESP_OK;}
inline int esp_wifi_get_channel(uint8_t* c,wifi_second_chan_t* s){*c=fake::channel;*s=WIFI_SECOND_CHAN_NONE;return ESP_OK;}
inline int esp_wifi_set_channel(uint8_t c,wifi_second_chan_t){++fake::mutations;if(fake::fail==3)return -1;fake::channel=c;return ESP_OK;}
inline int esp_wifi_get_mac(int,uint8_t* out){if(fake::fail==4)return -1;std::memcpy(out,fake::own,6);return ESP_OK;}
