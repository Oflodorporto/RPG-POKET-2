#pragma once
#include <cstdint>
#include <cstring>
#include "Faults.h"
constexpr int WIFI_STA=1,WIFI_IF_STA=0,WIFI_SECOND_CHAN_NONE=0;
constexpr int WIFI_MODE_NULL=0,WIFI_MODE_STA=1,WL_CONNECTED=3,WIFI_STORAGE_RAM=0,WIFI_STORAGE_FLASH=1;
using wifi_mode_t=int;
using wifi_second_chan_t=int;
inline constexpr uint8_t clubRadioOwnMac[6]={2,0,0,0,0,1};
inline int esp_wifi_get_channel(uint8_t* channel,wifi_second_chan_t* second){if(clubFake::openFail==7)return -1;*channel=uint8_t(clubFake::channel);*second=WIFI_SECOND_CHAN_NONE;return ESP_OK;}
inline int esp_wifi_set_storage(int storage){if(clubFake::openFail==8||(storage==WIFI_STORAGE_FLASH&&clubFake::closeFail==4))return -1;clubFake::storage=storage;return ESP_OK;}
inline int esp_wifi_set_channel(int channel,int){if((channel==6&&clubFake::openFail==3)||(channel!=6&&clubFake::closeFail==3))return -1;clubFake::channel=channel;return ESP_OK;}
inline int esp_wifi_get_mac(int,uint8_t* out){if(clubFake::openFail==4)return -1;memcpy(out,clubRadioOwnMac,6);return ESP_OK;}
struct ClubWifiFake {
 bool disconnect(bool,bool){clubFake::connected=false;return clubFake::openFail!=9;}
 void scanDelete(){}
 wifi_mode_t getMode(){return clubFake::mode;}
 bool getAutoReconnect(){return clubFake::autoReconnect;}
 bool setAutoReconnect(bool value){if((!value&&clubFake::openFail==10)||(value==clubFake::autoReconnect&&clubFake::closeFail==7))return false;clubFake::autoReconnect=value;return true;}
 int status(){return clubFake::connected?WL_CONNECTED:0;}
 void persistent(bool value){clubFake::persistent=value;}
 bool mode(int value){if((value==WIFI_STA&&clubFake::openFail==2)||(value==WIFI_MODE_NULL&&clubFake::closeFail==5))return false;clubFake::mode=value;return true;}
 bool reconnect(){++clubFake::reconnectCalls;if(clubFake::closeFail==6)return false;clubFake::connected=true;return true;}
};
inline ClubWifiFake WiFi;
