#include "../firmware/RPG_POKET_2/View.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
constexpr int OUTPUT=1,HIGH=1,MALLOC_CAP_SPIRAM=1,MALLOC_CAP_8BIT=2;
uint32_t now=0;uint32_t millis(){return now;}
void pinMode(int pin,int mode){assert(pin==41&&mode==OUTPUT);}void digitalWrite(int pin,int value){assert((pin==41||pin==45)&&value==HIGH);}
unsigned pwm=0;bool ledcAttach(int pin,int hz,int bits){assert(pin==1&&hz==5000&&bits==8);return true;}void ledcWrite(int pin,unsigned v){assert(pin==1&&v>=25&&v<=255);pwm=v;}
void* heap_caps_malloc(unsigned n,int flags){assert(flags==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return malloc(n);}void heap_caps_free(void* p){free(p);}
#include "../firmware/RPG_POKET_2/DeviceSettings.h"
int main(){
 initSettings();assert(menu.brightness==80&&menu.activeSlot==0&&pwm==204);assert(saveBrightness(50)&&pwm==127);settings.failKey="light";assert(!saveBrightness(60)&&menu.brightness==50);settings.failKey="";
 assert(rememberSlot(2)&&menu.activeSlot==2);assert(!rememberSlot(3));initSettings();assert(menu.activeSlot==2&&menu.brightness==50);testMemory();assert(menu.memoryTest==1);
 searchNetworks();assert(menu.scanning);assert(!tickWifi());WiFi.scanResult=2;assert(tickWifi()&&!menu.scanning&&menu.networkCount==2&&!strcmp(menu.network,"Teste1"));
 snprintf(menu.password,sizeof(menu.password),"curta");connectNetwork();assert(!menu.connecting);snprintf(menu.password,sizeof(menu.password),"senha_teste");connectNetwork();assert(menu.connecting&&WiFi.selected=="Teste1");
 WiFi.state=WL_CONNECTED;assert(tickWifi()&&menu.connected&&!menu.connecting&&!*menu.password&&settings.getBool("wifi",false));assert(settings.getString("ssid","")=="Teste1");forgetConnection();assert(!settings.getBool("wifi",true)&&!menu.connected);
 menu.networkIndex=1;networkChoice();assert(menu.openNetwork&&!strcmp(menu.network,"Teste2"));menu.password[0]=0;connectNetwork();WiFi.state=WL_CONNECTED;tickWifi();assert(settings.getString("pass","?")=="");
 forgetConnection();menu.networkIndex=0;networkChoice();snprintf(menu.password,sizeof(menu.password),"senha_teste");connectNetwork();now=20000;assert(tickWifi()&&!menu.connecting&&!menu.connected&&WiFi.disconnected);
 now=30000;connectNetwork();WiFi.state=WL_CONNECTED;settings.failKey="pass";tickWifi();assert(!settings.getBool("wifi",true));settings.failKey="";
 // Only successful complete credential writes enable restart reconnection.
 menu.networkIndex=0;networkChoice();snprintf(menu.password,sizeof(menu.password),"nova_senha");connectNetwork();tickWifi();assert(settings.getBool("wifi",false));WiFi.state=0;initSettings();resumeConnection();assert(menu.connecting&&WiFi.selected=="Teste1"&&WiFi.password=="nova_senha");
 artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);std::ifstream input("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(input);packBytes=std::vector<uint8_t>(std::istreambuf_iterator<char>(input),{});
 assert(loadSdArt()==ArtStatus::Ready&&cardEnds==1&&closes==1&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 cardAvailable=false;assert(loadSdArt()==ArtStatus::Missing&&cardEnds==2);cardAvailable=true;fileExists=false;assert(loadSdArt()==ArtStatus::Missing&&cardEnds==3);fileExists=true;badRead=10;assert(loadSdArt()==ArtStatus::Invalid&&cardEnds==4&&closes==2);badRead=-1;assert(loadSdArt()==ArtStatus::Ready&&cardEnds==5&&closes==3);
 free(artMemory);puts("PASS: actual settings adapter; PWM/persistence failures, async Wi-Fi scan/password/open network/timeout/restart, complete credential commit, volatile PSRAM test, read-only shared SPI art load/absence/read-error/retry/unmount.");
}
