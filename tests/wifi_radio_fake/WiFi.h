#pragma once
#include "FakeCore.h"
class WiFiGenericClass {protected: inline static bool _persistent=true;public:void persistent(bool x){_persistent=x;fake::persistent=x;}};
struct FakeWifi:WiFiGenericClass {
 wifi_mode_t getMode(){return fake::mode;}
 bool getAutoReconnect(){return fake::autoReconnect;}
 bool setAutoReconnect(bool x){if(fake::operation(fake::Auto))return false;fake::autoReconnect=x;return true;}
 int status(){return fake::connected?WL_CONNECTED:0;}
 bool mode(wifi_mode_t value){if(fake::operation(fake::Mode))return false;++fake::mutations;if(fake::mode==WIFI_MODE_NULL&&value!=WIFI_MODE_NULL){fake::storage=fake::persistent?WIFI_STORAGE_FLASH:WIFI_STORAGE_RAM;fake::secondary=WIFI_SECOND_CHAN_NONE;}if(value!=fake::mode&&value!=WIFI_MODE_NULL&&fake::storage==WIFI_STORAGE_FLASH)++fake::nvsWrites;fake::mode=value;if(value==WIFI_MODE_NULL)fake::connected=false;return true;}
 bool disconnect(bool off,bool erase,unsigned long){if(fake::operation(fake::Disconnect))return false;++fake::mutations;if(erase){++fake::erases;++fake::nvsWrites;}fake::connected=false;if(fake::changeCountryOnDisconnect)fake::country=fake::disconnectedCountry;if(off)fake::mode=WIFI_MODE_NULL;return true;}
 bool reconnect(){if(fake::operation(fake::Connect))return false;fake::connected=fake::reconnectImmediate;return true;}
};inline FakeWifi WiFi;
