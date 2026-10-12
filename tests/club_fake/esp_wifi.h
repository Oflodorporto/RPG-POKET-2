#pragma once
#include <cstdint>
#include <cstring>
constexpr int WIFI_STA=1,WIFI_IF_STA=0,WIFI_SECOND_CHAN_NONE=0;
inline constexpr uint8_t clubRadioOwnMac[6]={2,0,0,0,0,1};
inline int esp_wifi_set_channel(int,int){return 0;}
inline int esp_wifi_get_mac(int,uint8_t* out){memcpy(out,clubRadioOwnMac,6);return 0;}
struct ClubWifiFake {void disconnect(bool,bool){}void scanDelete(){}void mode(int){}void setAutoReconnect(bool){}};
inline ClubWifiFake WiFi;
