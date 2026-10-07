#pragma once
#include <WiFi.h>
#include <Preferences.h>
#include <SD.h>
#include <time.h>
#include "MenuView.h"
#include "UpdateModel.h"
inline worldClock::Clock localClock;
inline Preferences settings;inline bool settingsReady=false,pwmReady=false;
inline uint32_t wifiAt=0,signalAt=0;inline bool resumeWifi=false;
inline void applyBrightness(){if(pwmReady)ledcWrite(1,(menu.clockIdle?10u:menu.brightness)*255u/100);}
inline bool saveBrightness(uint8_t value){if(!settingsReady||settings.putUChar("light",value)!=1)return false;menu.brightness=value;applyBrightness();return true;}
inline void initSettings(){settingsReady=settings.begin("pkt2_ui",false);menu.activeSlot=settingsReady?settings.getUChar("slot",0):0;if(menu.activeSlot>2)menu.activeSlot=0;
  menu.brightness=settingsReady?settings.getUChar("light",80):80;if(menu.brightness<10||menu.brightness>100)menu.brightness=80;
  int automatic=1,offset=-3,cycle=1;auto record=settings.getString("clockcfg","");if(sscanf(record.c_str(),"%d,%d,%d",&automatic,&offset,&cycle)==3&&(automatic==0||automatic==1)&&offset>=-12&&offset<=14&&(cycle==0||cycle==1)){menu.clockAutomatic=automatic;menu.utcOffset=offset;menu.dayCycle=cycle;}else {menu.clockAutomatic=true;menu.utcOffset=-3;menu.dayCycle=true;}localClock=worldClock::Clock{};if(!menu.clockAutomatic)localClock.manual=true;
  pwmReady=ledcAttach(1,5000,8);applyBrightness();resumeWifi=settingsReady&&settings.getBool("wifi",false);
}
inline bool rememberSlot(uint8_t slot){if(slot>2||!settingsReady||settings.putUChar("slot",slot)!=1)return false;menu.activeSlot=slot;return true;}
inline ArtStatus loadSdArt(){
  if(!artMemory)return ArtStatus::Memory;makeFallback();pinMode(41,OUTPUT);digitalWrite(41,HIGH);digitalWrite(45,HIGH);
  if(!SD.begin(41,SPI,4000000,"/sd",1,false)){SD.end();digitalWrite(41,HIGH);return ArtStatus::Missing;}
  char named[48];updater::artPath(named,sizeof(named),ART_CRC);bool versioned=SD.exists(named);File f=SD.open(versioned?named:"/RPGPOKET/artes.pak",FILE_READ);ArtStatus status=ArtStatus::Missing;
  if(f){struct Reader{File& f;unsigned size(){return f.size();}int read(uint8_t* b,unsigned n){return f.read(b,n);}} reader{f};status=readArt(reader);f.close();}
  if(versioned&&status==ArtStatus::Ready){
    // Incomplete downloads may belong to a newer release: preserve them for resume.
    // FAT has no journal: close each file before deleting it; never format or touch saves.
    File dir=SD.open("/RPGPOKET");if(dir){File entry=dir.openNextFile();while(entry){char candidate[48];snprintf(candidate,sizeof(candidate),"/RPGPOKET/%s",entry.name());bool regular=!entry.isDirectory();entry.close();const char* name=strrchr(candidate,'/');name=name?name+1:candidate;
      bool ours=regular&&strlen(name)==18&&!strncmp(name,"artes_",6)&&!strcmp(name+14,".pak");for(unsigned i=6;ours&&i<14;++i)ours=(name[i]>='0'&&name[i]<='9')||(name[i]>='a'&&name[i]<='f');
      if(ours&&strcmp(candidate,named))SD.remove(candidate);entry=dir.openNextFile();}dir.close();}
    // The original installation pack is obsolete only after the versioned pack is valid.
    if(SD.exists("/RPGPOKET/artes.pak"))SD.remove("/RPGPOKET/artes.pak");
  }
  SD.end();digitalWrite(41,HIGH);digitalWrite(45,HIGH);return status;
}
// Each profile is one atomic NVS string: decimal SSID length + SSID + password.
// Credentials stay on the device and never enter logs or public source packages.
struct WifiProfile {char ssid[33]="",pass[64]="";};
inline WifiProfile wifiProfiles[5];inline uint8_t profileSlots[5];
inline void profileKey(char* key,unsigned i){snprintf(key,8,"net%u",i);}
inline bool decodeProfile(const char* raw,WifiProfile& p){size_t n=strlen(raw);if(n<3||raw[0]<'0'||raw[0]>'9'||raw[1]<'0'||raw[1]>'9')return false;unsigned len=(raw[0]-'0')*10+raw[1]-'0';if(!len||len>32||n<2+len||n-2-len>63)return false;memcpy(p.ssid,raw+2,len);p.ssid[len]=0;snprintf(p.pass,sizeof(p.pass),"%s",raw+2+len);return true;}
inline void loadWifiProfiles(){menu.savedCount=0;for(unsigned i=0;i<5;++i){wifiProfiles[i]=WifiProfile{};char key[8];profileKey(key,i);if(settingsReady&&decodeProfile(settings.getString(key,"").c_str(),wifiProfiles[i]))profileSlots[menu.savedCount++]=i;}if(menu.savedIndex>=menu.savedCount)menu.savedIndex=0;}
inline bool rememberWifiProfile(const char* ssid,const char* pass){if(!settingsReady||!*ssid||strlen(ssid)>32||strlen(pass)>63)return false;loadWifiProfiles();int slot=-1;for(unsigned i=0;i<5;++i)if(!strcmp(wifiProfiles[i].ssid,ssid)){slot=i;break;}if(slot<0)for(unsigned i=0;i<5;++i)if(!*wifiProfiles[i].ssid){slot=i;break;}if(slot<0)return false;
  char key[8],record[100];profileKey(key,slot);snprintf(record,sizeof(record),"%02u%s%s",unsigned(strlen(ssid)),ssid,pass);bool ok=settings.putString(key,record)==strlen(record)&&settings.getString(key,"?")==record;loadWifiProfiles();return ok;}
inline void networkChoice(){if(menu.savedNetworks){if(!menu.savedCount){menu.network[0]=0;return;}menu.savedIndex%=menu.savedCount;const auto& p=wifiProfiles[profileSlots[menu.savedIndex]];snprintf(menu.network,sizeof(menu.network),"%s",p.ssid);menu.openNetwork=!*p.pass;menu.networkChannel=0;return;}
  if(menu.networkCount<=0){menu.network[0]=0;return;}menu.networkIndex=(menu.networkIndex+menu.networkCount)%menu.networkCount;
  snprintf(menu.network,sizeof(menu.network),"%s",WiFi.SSID(menu.networkIndex).c_str());menu.openNetwork=WiFi.encryptionType(menu.networkIndex)==WIFI_AUTH_OPEN;menu.networkDbm=WiFi.RSSI(menu.networkIndex);menu.networkChannel=WiFi.channel(menu.networkIndex);
}
inline void showSavedNetworks(){if(menu.scanning||menu.connecting)return;loadWifiProfiles();menu.savedNetworks=true;menu.savedIndex=0;networkChoice();menu.notice="";}
inline void searchNetworks(){if(menu.scanning||menu.connecting)return;WiFi.mode(WIFI_STA);WiFi.scanDelete();menu.savedNetworks=false;menu.network[0]=0;menu.networkCount=0;menu.networkIndex=0;int rc=WiFi.scanNetworks(true,false,false,500,0);menu.scanning=rc==WIFI_SCAN_RUNNING;menu.notice=rc==WIFI_SCAN_FAILED?"Busca falhou; tente novamente":"";}
inline void connectNetwork(){
  if(menu.scanning||menu.connecting)return;if(!*menu.network){menu.notice="Escolha uma rede primeiro";return;}
  size_t n=strlen(menu.password);if(!menu.openNetwork&&(n<8||n>63)){menu.notice="Senha: entre 8 e 63 caracteres";return;}
  WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(true);WiFi.begin(menu.network,menu.openNetwork?nullptr:menu.password);menu.connecting=true;wifiAt=millis();menu.notice="";
}
inline void connectSavedNetwork(){if(!menu.savedCount||menu.scanning||menu.connecting)return;const auto& p=wifiProfiles[profileSlots[menu.savedIndex]];snprintf(menu.password,sizeof(menu.password),"%s",p.pass);connectNetwork();}
inline void forgetConnection(){resumeWifi=false;if(settingsReady)settings.putBool("wifi",false);WiFi.setAutoReconnect(false);WiFi.disconnect(false,false);menu.connecting=false;menu.connected=false;menu.signalBars=0;menu.password[0]=0;menu.notice="Wi-Fi desligado";}
inline bool deleteSavedNetwork(){if(!menu.savedNetworks||!menu.savedCount||!settingsReady)return false;char key[8];profileKey(key,profileSlots[menu.savedIndex]);if(menu.network[0]&&settings.getString("ssid","")==menu.network){if(settings.putBool("wifi",false)!=1)return false;resumeWifi=false;WiFi.setAutoReconnect(false);WiFi.disconnect(false,false);menu.connected=menu.connecting=false;menu.signalBars=0;}
  settings.putString(key,"");if(!(settings.getString(key,"?")=="")){menu.notice="Nao foi possivel esquecer";return false;}loadWifiProfiles();networkChoice();menu.notice="Rede removida";return true;}
inline bool tickWifi(){bool changed=false;
  if(menu.scanning){int n=WiFi.scanComplete();if(n!=WIFI_SCAN_RUNNING){menu.scanning=false;menu.networkCount=std::max(0,n);menu.networkIndex=0;networkChoice();menu.notice=n<=0?"Nenhuma rede encontrada":"";changed=true;}}
  bool connected=WiFi.status()==WL_CONNECTED;if(connected!=menu.connected){menu.connected=connected;menu.signalBars=connected?wifiBars(WiFi.RSSI()):0;changed=true;}
  if(connected&&(changed||uint32_t(millis()-signalAt)>=2000)){signalAt=millis();int dbm=WiFi.RSSI();uint8_t bars=wifiBars(dbm);if(bars!=menu.signalBars||dbm!=menu.signalDbm){menu.signalBars=bars;menu.signalDbm=dbm;changed=true;}snprintf(menu.ip,sizeof(menu.ip),"%s",WiFi.localIP().toString().c_str());}
  if(menu.connecting&&connected&&WiFi.SSID()==menu.network){menu.connecting=false;configTzTime("UTC0","pool.ntp.org","time.google.com");bool saved=settingsReady&&settings.putBool("wifi",false)==1&&settings.putString("ssid",menu.network)>0;
    if(saved){settings.putString("pass",menu.password);saved=settings.getString("pass","?")==menu.password;}
    if(saved)saved=settings.putBool("wifi",true)==1;if(saved)resumeWifi=true;bool profile=saved&&rememberWifiProfile(menu.network,menu.password);menu.notice=profile?"Conexao salva":saved?"Conectado; lista cheia ou erro":"Conectado; ajustes nao salvos";memset(menu.password,0,sizeof(menu.password));changed=true;}
  else if(menu.connecting&&uint32_t(millis()-wifiAt)>=20000){menu.connecting=false;WiFi.setAutoReconnect(false);WiFi.disconnect(false,false);menu.notice="Falhou; confira rede e senha";memset(menu.password,0,sizeof(menu.password));changed=true;}
  return changed;
}
inline void resumeConnection(){loadWifiProfiles();if(!resumeWifi){menu.savedNetworks=true;networkChoice();return;}snprintf(menu.network,sizeof(menu.network),"%s",settings.getString("ssid","").c_str());snprintf(menu.password,sizeof(menu.password),"%s",settings.getString("pass","").c_str());menu.openNetwork=!*menu.password;if(*menu.network){rememberWifiProfile(menu.network,menu.password);connectNetwork();}menu.savedNetworks=true;}

inline void testMemory(){uint32_t* allocated=static_cast<uint32_t*>(heap_caps_malloc(65536,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));volatile uint32_t* sample=allocated;menu.memoryTest=0;if(!sample)return;for(unsigned i=0;i<16384;++i)sample[i]=i^0x5a9695a5u;menu.memoryTest=1;for(unsigned i=0;i<16384;++i)if(sample[i]!=(i^0x5a9695a5u)){menu.memoryTest=0;break;}heap_caps_free(allocated);}

inline bool saveClockConfig(bool automatic,int offset,bool cycle){if(offset<-12||offset>14||!settingsReady)return false;char record[24];snprintf(record,sizeof(record),"%u,%d,%u",automatic,offset,cycle);if(settings.putString("clockcfg",record)!=strlen(record)||!(settings.getString("clockcfg","?")==record))return false;bool switchMode=menu.clockAutomatic!=automatic;menu.clockAutomatic=automatic;menu.utcOffset=offset;menu.dayCycle=cycle;if(switchMode){localClock.manual=!automatic;if(!automatic)localClock.set(time(nullptr),millis());}return true;}
inline void prepareClockEdit(){menu.clockField=0;int64_t stamp=localClock.read(time(nullptr),millis());if(localClock.valid(stamp)){time_t value=stamp+int(menu.utcOffset)*3600;struct tm local;gmtime_r(&value,&local);menu.clockDraft[0]=local.tm_year+1900;menu.clockDraft[1]=local.tm_mon+1;menu.clockDraft[2]=local.tm_mday;menu.clockDraft[3]=local.tm_hour;menu.clockDraft[4]=local.tm_min;}}
inline void adjustClockDraft(int direction){int& value=menu.clockDraft[menu.clockField];int low=menu.clockField==0?2026:menu.clockField<=2?1:0;int high=menu.clockField==0?2099:menu.clockField==1?12:menu.clockField==2?worldClock::days(menu.clockDraft[0],menu.clockDraft[1]):menu.clockField==3?23:59;value+=direction;if(value<low)value=high;if(value>high)value=low;menu.clockDraft[2]=std::min(menu.clockDraft[2],int(worldClock::days(menu.clockDraft[0],menu.clockDraft[1])));}
inline bool applyClockDraft(){int64_t value=worldClock::epoch(menu.clockDraft[0],menu.clockDraft[1],menu.clockDraft[2],menu.clockDraft[3],menu.clockDraft[4],menu.utcOffset);if(!value||!saveClockConfig(false,menu.utcOffset,menu.dayCycle))return false;localClock.set(value,millis());return true;}
inline bool tickClock(bool allowPeriod=true){int64_t value=localClock.read(time(nullptr),millis());bool valid=localClock.valid(value);char hours[9]="--:--:--",date[11]="--/--/----";bool changed=valid!=menu.clockValid;
 if(valid){time_t stamp=value+int(menu.utcOffset)*3600;struct tm local;gmtime_r(&stamp,&local);strftime(hours,sizeof(hours),"%H:%M:%S",&local);strftime(date,sizeof(date),"%d/%m/%Y",&local);auto p=worldClock::period(local.tm_hour);if(allowPeriod&&p!=menu.worldPeriod){menu.worldPeriod=p;changed=true;}}
 changed=changed||strcmp(hours,menu.clockTime)||strcmp(date,menu.clockDate);snprintf(menu.clockTime,sizeof(menu.clockTime),"%s",hours);snprintf(menu.clockDate,sizeof(menu.clockDate),"%s",date);menu.clockValid=valid;return changed;}
