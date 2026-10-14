#pragma once
#include "FakeCore.h"
struct FakeWifi {
 wifi_mode_t getMode(){return fake::mode;}
 bool getAutoReconnect(){return fake::autoReconnect;}
 bool setAutoReconnect(bool x){fake::autoReconnect=x;return true;}
 int status(){return fake::connected?WL_CONNECTED:0;}
 void persistent(bool x){fake::persistent=x;}
 bool mode(wifi_mode_t mode){++fake::mutations;if(fake::fail==1)return false;if(fake::mode==WIFI_MODE_NULL&&mode!=WIFI_MODE_NULL)fake::storage=fake::persistent?WIFI_STORAGE_FLASH:WIFI_STORAGE_RAM;if(mode!=WIFI_MODE_NULL&&fake::mode!=mode&&fake::storage==WIFI_STORAGE_FLASH)++fake::nvsWrites;fake::mode=mode;return true;}
 bool disconnect(bool off,bool erase,unsigned long){++fake::mutations;if(erase){++fake::erases;++fake::nvsWrites;}fake::connected=false;if(off)fake::mode=WIFI_MODE_NULL;return true;}
 bool reconnect(){++fake::reconnects;fake::connected=true;return true;}
};
inline FakeWifi WiFi;
