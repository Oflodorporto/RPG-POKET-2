#pragma once
#include <WiFi.h>
#include <Preferences.h>
#include <SD.h>
#include "MenuView.h"
#include "UpdateModel.h"
inline Preferences settings;inline bool settingsReady=false,pwmReady=false;
inline uint32_t wifiAt=0;inline bool resumeWifi=false;
inline void applyBrightness(){if(pwmReady)ledcWrite(1,menu.brightness*255u/100);}
inline bool saveBrightness(uint8_t value){if(!settingsReady||settings.putUChar("light",value)!=1)return false;menu.brightness=value;applyBrightness();return true;}
inline void initSettings(){settingsReady=settings.begin("pkt2_ui",false);menu.activeSlot=settingsReady?settings.getUChar("slot",0):0;if(menu.activeSlot>2)menu.activeSlot=0;
  menu.brightness=settingsReady?settings.getUChar("light",80):80;if(menu.brightness<10||menu.brightness>100)menu.brightness=80;
  pwmReady=ledcAttach(1,5000,8);applyBrightness();resumeWifi=settingsReady&&settings.getBool("wifi",false);
}
inline bool rememberSlot(uint8_t slot){if(slot>2||!settingsReady||settings.putUChar("slot",slot)!=1)return false;menu.activeSlot=slot;return true;}
inline ArtStatus loadSdArt(){
  if(!artMemory)return ArtStatus::Memory;makeFallback();pinMode(41,OUTPUT);digitalWrite(41,HIGH);digitalWrite(45,HIGH);
  if(!SD.begin(41,SPI,4000000,"/sd",1,false)){SD.end();digitalWrite(41,HIGH);return ArtStatus::Missing;}
  char named[48];updater::artPath(named,sizeof(named),ART_CRC);bool versioned=SD.exists(named);File f=SD.open(versioned?named:"/RPGPOKET/artes.pak",FILE_READ);ArtStatus status=ArtStatus::Missing;
  if(f){struct Reader{File& f;unsigned size(){return f.size();}int read(uint8_t* b,unsigned n){return f.read(b,n);}} reader{f};status=readArt(reader);f.close();}
  if(versioned&&status==ArtStatus::Ready){
    SD.remove("/RPGPOKET/firmware.part");SD.remove("/RPGPOKET/artes.part");
    // FAT has no journal: close each file before deleting it; never format or touch saves.
    File dir=SD.open("/RPGPOKET");if(dir){File entry=dir.openNextFile();while(entry){char candidate[48];snprintf(candidate,sizeof(candidate),"/RPGPOKET/%s",entry.name());bool regular=!entry.isDirectory();entry.close();const char* name=strrchr(candidate,'/');name=name?name+1:candidate;
      bool ours=regular&&strlen(name)==18&&!strncmp(name,"artes_",6)&&!strcmp(name+14,".pak");for(unsigned i=6;ours&&i<14;++i)ours=(name[i]>='0'&&name[i]<='9')||(name[i]>='a'&&name[i]<='f');
      if(ours&&strcmp(candidate,named))SD.remove(candidate);entry=dir.openNextFile();}dir.close();}
    // The original installation pack is obsolete only after the versioned pack is valid.
    if(SD.exists("/RPGPOKET/artes.pak"))SD.remove("/RPGPOKET/artes.pak");
  }
  SD.end();digitalWrite(41,HIGH);digitalWrite(45,HIGH);return status;
}
inline void networkChoice(){if(menu.networkCount<=0){menu.network[0]=0;return;}menu.networkIndex=(menu.networkIndex+menu.networkCount)%menu.networkCount;
  snprintf(menu.network,sizeof(menu.network),"%s",WiFi.SSID(menu.networkIndex).c_str());menu.openNetwork=WiFi.encryptionType(menu.networkIndex)==WIFI_AUTH_OPEN;
}
inline void searchNetworks(){if(menu.scanning||menu.connecting)return;WiFi.mode(WIFI_STA);WiFi.scanDelete();menu.networkCount=0;menu.networkIndex=0;int rc=WiFi.scanNetworks(true);menu.scanning=rc==WIFI_SCAN_RUNNING;menu.notice=rc==WIFI_SCAN_FAILED?"Busca falhou; tente novamente":"";}
inline void connectNetwork(){
  if(!*menu.network){menu.notice="Escolha uma rede primeiro";return;}
  size_t n=strlen(menu.password);if(!menu.openNetwork&&(n<8||n>63)){menu.notice="Senha: entre 8 e 63 caracteres";return;}
  WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(false);WiFi.begin(menu.network,menu.openNetwork?nullptr:menu.password);menu.connecting=true;wifiAt=millis();menu.notice="";
}
inline void forgetConnection(){resumeWifi=false;if(settingsReady)settings.putBool("wifi",false);WiFi.disconnect(false,false);menu.connecting=false;menu.connected=false;menu.password[0]=0;menu.notice="Wi-Fi desligado";}
inline bool tickWifi(){bool changed=false;
  if(menu.scanning){int n=WiFi.scanComplete();if(n!=WIFI_SCAN_RUNNING){menu.scanning=false;menu.networkCount=std::max(0,n);menu.networkIndex=0;networkChoice();menu.notice=n<=0?"Nenhuma rede encontrada":"";changed=true;}}
  bool connected=WiFi.status()==WL_CONNECTED;if(connected!=menu.connected){menu.connected=connected;changed=true;}
  if(menu.connecting&&connected){menu.connecting=false;snprintf(menu.ip,sizeof(menu.ip),"%s",WiFi.localIP().toString().c_str());bool saved=settingsReady&&settings.putBool("wifi",false)==1&&settings.putString("ssid",menu.network)>0;
    if(saved){settings.putString("pass",menu.password);saved=settings.getString("pass","?")==menu.password;}
    if(saved)saved=settings.putBool("wifi",true)==1;if(saved)resumeWifi=true;menu.notice=saved?"Conexao salva":"Conectado; ajustes nao salvos";memset(menu.password,0,sizeof(menu.password));changed=true;}
  else if(menu.connecting&&uint32_t(millis()-wifiAt)>=20000){menu.connecting=false;WiFi.disconnect(false,false);menu.notice="Falhou; confira rede e senha";changed=true;}
  return changed;
}
inline void resumeConnection(){if(!resumeWifi)return;snprintf(menu.network,sizeof(menu.network),"%s",settings.getString("ssid","").c_str());snprintf(menu.password,sizeof(menu.password),"%s",settings.getString("pass","").c_str());menu.openNetwork=!*menu.password;if(*menu.network)connectNetwork();}

inline void testMemory(){uint32_t* allocated=static_cast<uint32_t*>(heap_caps_malloc(65536,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));volatile uint32_t* sample=allocated;menu.memoryTest=0;if(!sample)return;for(unsigned i=0;i<16384;++i)sample[i]=i^0x5a9695a5u;menu.memoryTest=1;for(unsigned i=0;i<16384;++i)if(sample[i]!=(i^0x5a9695a5u)){menu.memoryTest=0;break;}heap_caps_free(allocated);}
