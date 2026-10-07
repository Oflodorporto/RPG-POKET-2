#pragma once
#include <string>
#include <vector>
struct String {std::string value;String(const char* s=""):value(s){}String(std::string s):value(s){}const char* c_str()const{return value.c_str();}bool operator==(const char* s)const{return value==s;}};
constexpr int WIFI_STA=1,WIFI_SCAN_RUNNING=-1,WIFI_SCAN_FAILED=-2,WIFI_AUTH_OPEN=0,WL_CONNECTED=3;
struct WifiFake {
 int signal=-60;bool autoReconnect=false;int state=0,scanResult=WIFI_SCAN_RUNNING;bool disconnected=false;std::string selected,password;std::vector<std::string> networks={"Teste1","Teste2"};
 void setAutoReconnect(bool b){autoReconnect=b;}bool mode(int){return true;}void scanDelete(){}int scanNetworks(bool async,bool hidden,bool passive,unsigned dwell,int channel){assert(async&&!hidden&&!passive&&dwell==500&&channel==0);scanResult=WIFI_SCAN_RUNNING;return scanResult;}int scanComplete(){return scanResult;}
 int RSSI(){return signal;}int RSSI(int i){return -50-i*20;}int channel(int i){return 1+i*5;}String SSID(){return String(selected);}String SSID(int i){return String(networks[i]);}int encryptionType(int i){return i==1?WIFI_AUTH_OPEN:1;}int status(){return state;}
 void begin(const char* s,const char* p){selected=s;password=p?p:"";disconnected=false;}
 void disconnect(bool,bool){state=0;disconnected=true;}
 struct IP {String toString(){return String("192.168.1.23");}};IP localIP(){return {};}
};inline WifiFake WiFi;

inline void configTzTime(const char* zone,const char*,const char*){assert(!strcmp(zone,"UTC0"));}
