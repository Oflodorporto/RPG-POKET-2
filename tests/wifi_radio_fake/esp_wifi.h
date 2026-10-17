#pragma once
#include "FakeCore.h"
inline int esp_wifi_set_storage(wifi_storage_t v){const int e=fake::powered(fake::Storage);if(!e)fake::storage=v;return e;}
inline int esp_wifi_get_channel(uint8_t* p,wifi_second_chan_t* s){const int e=fake::powered(fake::GetChannel);if(!e){*p=fake::channel;*s=fake::secondary;}return e;}
inline int esp_wifi_set_channel(uint8_t p,wifi_second_chan_t s){const int e=fake::powered(fake::SetChannel);if(!e){fake::channel=p;fake::secondary=s;}return e;}
inline int esp_wifi_get_ps(wifi_ps_type_t* p){const int e=fake::powered(fake::GetPS);if(!e)*p=fake::ps;return e;}
inline int esp_wifi_set_ps(wifi_ps_type_t p){const int e=fake::powered(fake::SetPS);if(!e)fake::ps=p;return e;}
inline int esp_wifi_get_max_tx_power(int8_t* p){const int e=fake::powered(fake::GetPower);if(!e)*p=fake::power;return e;}
inline int esp_wifi_set_max_tx_power(int8_t p){const int e=fake::powered(fake::SetPower);if(e)return e;if(p<8||p>84)return ESP_ERR_INVALID_ARG;const int8_t levels[]={8,20,28,34,44,52,56,60,66,72,80};int8_t v=8;for(auto level:levels)if(level<=p)v=level;fake::power=fake::forcedActual?fake::forcedActual:v;fake::country.max_tx_power=fake::power/4;return ESP_OK;}
inline int esp_wifi_get_protocol(int,uint8_t* p){const int e=fake::powered(fake::GetProtocol);if(!e)*p=fake::protocol;return e;}
inline int esp_wifi_set_protocol(int,uint8_t p){const int e=fake::powered(fake::SetProtocol);if(!e){fake::protocol=p;if(fake::storage==WIFI_STORAGE_FLASH)++fake::nvsWrites;}return e;}
inline int esp_wifi_get_country(wifi_country_t* p){const int e=fake::powered(fake::GetCountry);if(!e)*p=fake::country;return e;}
inline int esp_wifi_set_country(const wifi_country_t* p){++fake::countrySets;++fake::nvsWrites;fake::country=*p;return ESP_OK;}
inline int esp_wifi_get_mode(wifi_mode_t* p){const int e=fake::operation(fake::GetMode);if(!e)*p=fake::mode;return e;}
inline int esp_wifi_get_mac(int,uint8_t* p){const int e=fake::powered(fake::GetMac);if(!e)std::memcpy(p,fake::own,6);return e;}
inline int esp_wifi_sta_get_ap_info(wifi_ap_record_t* p){const int e=fake::powered(fake::GetAP);if(e)return e;if(!fake::connected)return ESP_FAIL;std::memcpy(p->bssid,fake::bssid,6);return ESP_OK;}
inline int esp_wifi_connect(){const int e=fake::powered(fake::Connect);if(!e)fake::connected=fake::reconnectImmediate;return e;}
