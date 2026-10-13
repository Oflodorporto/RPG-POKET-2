// Diagnostic only for Waveshare ESP32-S3-Touch-LCD-2 (SKU29667).
#include <Arduino.h>
#ifndef ARDUINO_ESP32S3_DEV
#error "Selecione ESP32S3 Dev Module."
#endif
#ifndef BOARD_HAS_PSRAM
#error "Selecione OPI PSRAM."
#endif
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <mbedtls/sha256.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include "src/GFX/databus/Arduino_HWSPI.h"
#include "src/GFX/display/Arduino_ST7789.h"
#include "DiagnosticCore.h"
#include "Fixture.h"
#include "ReadbackTarget.h"
constexpr const char* VERSION="SD-DIAG-2026.10.13-2";
constexpr const char* ART="/RPGPOKET/artes.pak";
constexpr const char* URL=fixture::URL;
Arduino_HWSPI bus(42,45,39,38,40);Arduino_ST7789 lcd(&bus,-1,0,true,240,320);
diag::Report report;diag::Session session;uint8_t buffer[4096];uint32_t hz=4000000;bool lcdReady=false,privateInputFailed=false;unsigned phaseFailures=0;char directory[64]={};
void logLine(const char* format,...){char line[256];va_list a;va_start(a,format);vsnprintf(line,sizeof(line),format,a);va_end(a);report.append(line);report.append("\n");Serial.println(line);}
void screen(const char* title,uint32_t bytes=0){if(!lcdReady)return;lcd.fillScreen(0x0841);lcd.setTextColor(0xffff);lcd.setTextSize(2);lcd.setCursor(10,30);lcd.print("DIAGNOSTICO SD");lcd.setTextSize(1);lcd.setCursor(10,75);lcd.print(title);lcd.setCursor(10,100);lcd.print(bytes);lcd.setCursor(10,135);lcd.print("Relatorio: Serial 115200");lcd.setCursor(10,155);lcd.print("R = repetir relatorio RAM");lcd.setCursor(10,175);lcd.print("Arquivos do jogo: so leitura.");}
void deselect(){digitalWrite(41,HIGH);digitalWrite(45,HIGH);}
bool checkPins(const char* stage){int sd=digitalRead(41),panel=digitalRead(45);logLine("PINS stage=%s SD_CS41=%d LCD_CS45=%d idle_expected=1/1; MISO40=%d",stage,sd,panel,digitalRead(40));return sd==HIGH&&panel==HIGH;}

bool mountCard(const char* stage){deselect();errno=0;bool ok=SD.begin(41,SPI,hz,"/sd",1,false);int e=errno;logLine("MOUNT stage=%s hz=%lu init_hz=400000 ok=%u errno=%d",stage,(unsigned long)hz,ok,e);if(ok)logLine("CARD type=%u bytes=%llu",unsigned(SD.cardType()),(unsigned long long)SD.cardSize());return ok;}
void unmountCard(){SD.end();deselect();}
struct Hash {mbedtls_sha256_context ctx;Hash(){mbedtls_sha256_init(&ctx);}~Hash(){mbedtls_sha256_free(&ctx);}bool begin(){return mbedtls_sha256_starts(&ctx,0)==0;}bool update(const uint8_t* p,size_t n){return mbedtls_sha256_update(&ctx,p,n)==0;}bool finish(char* out){uint8_t b[32];if(mbedtls_sha256_finish(&ctx,b))return false;for(unsigned i=0;i<32;++i)snprintf(out+i*2,3,"%02x",b[i]);return true;}bool snapshot(char* out){Hash clone;mbedtls_sha256_clone(&clone.ctx,&ctx);return clone.finish(out);}};
struct Reader {File& file;uint32_t size(){return file.size();}diag::Read read(uint8_t* b,size_t n){errno=0;size_t got=file.read(b,n);int e=errno;return {got,e};}};
void resultLine(const char* stage,const diag::Result& r){logLine("READ stage=%s ok=%u cause=%s pos=%lu wanted=%lu got=%lu errno=%d",stage,r.ok,r.cause,(unsigned long)r.offset,(unsigned long)r.requested,(unsigned long)r.received,r.error);if(r.sha[0])logLine("SHA stage=%s %s",stage,r.sha);}
bool verifyPath(const char* path,const char* stage,bool draw,uint32_t bytes=diag::FILE_BYTES,const char* expected=diag::SHA,bool header=true){
 errno=0;File f=SD.open(path,FILE_READ);int e=errno;if(!f){logLine("OPEN stage=%s ok=0 errno=%d",stage,e);return false;}Reader reader{f};Hash hash;uint32_t start=millis(),next=0;
 auto progress=[&](uint32_t pos){if(draw&&pos>=next){screen(stage,pos);next=pos+65536;}yield();return uint32_t(millis()-start)<180000;};
 auto r=diag::verify(reader,hash,buffer,progress,bytes,expected,header);f.close();resultLine(stage,r);logLine("TIME stage=%s ms=%lu",stage,(unsigned long)(millis()-start));return r.ok;
}
bool tailProbe(const char* stage){errno=0;File f=SD.open(ART,FILE_READ);if(!f){logLine("TAIL stage=%s open=0 errno=%d",stage,errno);return false;}bool ok=true;
 for(unsigned i=0;i<2;++i){uint32_t pos=i?diag::LAST_SECTOR:diag::TAIL;unsigned request=i?512:1344,expect=i?336:1344;errno=0;bool seek=f.seek(pos);int e=errno;if(!seek){logLine("TAIL stage=%s seek=0 pos=%lu errno=%d",stage,(unsigned long)pos,e);ok=false;break;}errno=0;size_t n=f.read(buffer,request);e=errno;logLine("TAIL stage=%s pos=%lu request=%u expected=%u got=%lu errno=%d",stage,(unsigned long)pos,request,expect,(unsigned long)n,e);if(n!=expect||e){ok=false;break;}}
 f.close();return ok;
}
void storageRun(){logLine("STORAGE BEGIN hz=%lu one_mount_across_phases=1",(unsigned long)hz);if(!checkPins("before-mount")){logLine("STORAGE aborted: CS idle mismatch");return;}bool mounted=mountCard("before-LCD");
 if(mounted){if(!verifyPath(ART,"before-LCD",false))++phaseFailures;if(!tailProbe("before-LCD"))++phaseFailures;}
 else ++phaseFailures;
 // Always bring up display, even if card mount failed; no other SPI user is active.
 deselect();lcdReady=lcd.begin(40000000);if(ledcAttach(1,5000,8))ledcWrite(1,160);screen("Tela iniciada");if(lcdReady){lcd.fillRect(10,210,60,30,0xf800);lcd.fillRect(90,210,60,30,0x07e0);lcd.fillRect(170,210,60,30,0x001f);}checkPins("after-LCD-init");logLine("LCD begin=%u hz=40000000",lcdReady);
 if(mounted){logLine("PHASE after-LCD prior_failures=%u (earlier error may affect later phases)",phaseFailures);if(!verifyPath(ART,"after-LCD",false))++phaseFailures;if(!tailProbe("after-LCD"))++phaseFailures;logLine("PHASE interleaved-LCD prior_failures=%u; draw between reads, never concurrently",phaseFailures);if(!verifyPath(ART,"interleaved-LCD",true))++phaseFailures;}
 checkPins("after-reads");unmountCard();logLine("REMOUNT separate_cycle=1 prior_failures=%u",phaseFailures);bool again=mountCard("after-read-remount");if(again){if(!tailProbe("remounted"))++phaseFailures;}else ++phaseFailures;unmountCard();screen("Leituras concluidas");logLine("STORAGE END failures=%u; compare1MHz only after power-off/new boot",phaseFailures);
}
bool readPrivateLine(char* out,size_t cap){diag::PrivateInput input{out,cap};uint32_t start=millis();while(uint32_t(millis()-start)<90000){if(Serial.available()){auto r=input.feed(Serial.read());if(r==diag::InputResult::Done)return true;if(r==diag::InputResult::Invalid){privateInputFailed=true;return false;}}delay(5);}memset(out,0,cap);privateInputFailed=true;return false;}
bool connectPrivate(bool secureTime){char ssid[33]={},pass[65]={};Serial.println("SSID local (nao entra no relatorio), terminar com Nova linha:");if(!readPrivateLine(ssid,sizeof(ssid))||!ssid[0]){privateInputFailed=true;memset(ssid,0,sizeof(ssid));return false;}Serial.println("Senha local (nao repetida nem salva), terminar com Nova linha; vazia para rede aberta:");if(!readPrivateLine(pass,sizeof(pass))){memset(ssid,0,sizeof(ssid));return false;}
 WiFi.persistent(false);WiFi.setAutoReconnect(false);WiFi.mode(WIFI_STA);esp_err_t e=esp_wifi_set_storage(WIFI_STORAGE_RAM);if(e!=ESP_OK){memset(ssid,0,sizeof(ssid));memset(pass,0,sizeof(pass));logLine("WIFI storage-RAM failure=%d",int(e));return false;}WiFi.begin(ssid,pass);memset(ssid,0,sizeof(ssid));memset(pass,0,sizeof(pass));uint32_t t=millis();while(WiFi.status()!=WL_CONNECTED&&uint32_t(millis()-t)<30000)delay(50);if(WiFi.status()!=WL_CONNECTED){logLine("WIFI connect failed status=%u",unsigned(WiFi.status()));return false;}logLine("WIFI connected RSSI=%ld",long(WiFi.RSSI()));if(!secureTime)return true;configTime(0,0,"pool.ntp.org","time.google.com");t=millis();while(time(nullptr)<1767225600&&uint32_t(millis()-t)<20000)delay(50);if(time(nullptr)<1767225600){logLine("WIFI secure time unavailable");return false;}return true;
}
void wifiOff(){WiFi.disconnect(false,false);WiFi.mode(WIFI_OFF);}
// P is isolated: no LCD initialization, HTTP, write, report-file or NVS calls.
bool radioState(const char* stage,unsigned errors,unsigned cycle,bool on) {
  bool connected=WiFi.status()==WL_CONNECTED;
  unsigned mode=unsigned(WiFi.getMode());
  bool valid=diag::radioMatches(on,mode,connected);
  logLine("P CONDITIONS stage=%s prior_errors=%u cycle=%u hz=%lu LCD=%u expected=%s mode=%u connected=%u valid=%u RSSI=%ld",stage,errors,cycle,(unsigned long)hz,lcdReady,on?"ON":"OFF",mode,connected,valid,connected?long(WiFi.RSSI()):0L);
  return valid;
}
void existingReadbackRun() {
  if(!diag::diagnosticTarget(readbackTarget::PATH)){logLine("P rejected: target path outside fixed diagnostic shape");return;}
  hz=1000000;
  wifiOff();
  delay(250);
  logLine("P BEGIN existing=%s bytes=%lu no_download=1 no_write=1",readbackTarget::PATH,(unsigned long)readbackTarget::BYTES);
  logLine("P fixed LCD-off; one mount for OFF/ON/OFF. Prior I/O errors may affect following phases.");
  if(!radioState("off-before-mount",0,1,false)){logLine("P END radio_OFF_not_confirmed=1; all reads SKIPPED");return;}
  if(!checkPins("P-before-mount")||!mountCard("P-cycle1")){unmountCard();logLine("P END initial_mount_failed=1; radio comparison not performed");return;}
  unsigned errors=0,invalidRadio=0;
  bool attemptedOn=false;
  if(radioState("off-before",errors,1,false)){
    if(!verifyPath(readbackTarget::PATH,"P-off-before",false,readbackTarget::BYTES,readbackTarget::SHA,false))++errors;
  }else {++invalidRadio;logLine("P-off-before SKIPPED radio condition absent");}
  bool connected=connectPrivate(false); // No NTP or HTTP needed for a local read.
  bool on=radioState("on-before",errors,1,true);
  if(connected&&on){
    attemptedOn=true;
    if(!verifyPath(readbackTarget::PATH,"P-connected",false,readbackTarget::BYTES,readbackTarget::SHA,false))++errors;
    if(!radioState("on-after",errors,1,true)){++invalidRadio;logLine("P-connected radio condition changed; invalidate ON/OFF comparison even if SHA passed");}
  }else {++invalidRadio;logLine("P-connected SKIPPED (WiFi/input failed or radio condition absent)");}
  wifiOff();
  delay(250);
  if(radioState("off-after",errors,1,false)){
    if(!verifyPath(readbackTarget::PATH,"P-off-after",false,readbackTarget::BYTES,readbackTarget::SHA,false))++errors;
  }else {++invalidRadio;logLine("P-off-after SKIPPED radio condition absent");}
  unmountCard();
  logLine("P REMOUNT cycle2 separate_control=1 prior_errors=%u",errors);
  if(!radioState("off-remount",errors,2,false)){++invalidRadio;logLine("P-remount SKIPPED radio condition absent");}
  else if(mountCard("P-cycle2")){
    if(!verifyPath(readbackTarget::PATH,"P-off-remount",false,readbackTarget::BYTES,readbackTarget::SHA,false))++errors;
  }else ++errors;
  unmountCard();
  logLine("P END read_or_mount_errors=%u invalid_radio_phases=%u connected_phase_attempted=%u; sequence alone does not prove physical cause",errors,invalidRadio,attemptedOn);
}
bool uniqueDirectory(){if(directory[0])return true;for(unsigned i=0;i<4;++i){char p[64];snprintf(p,sizeof(p),"/RPGDIAG_%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random());if(SD.exists(p))continue;if(SD.mkdir(p)){strcpy(directory,p);logLine("NEW_DIRECTORY %s",directory);return true;}logLine("DIRECTORY create failed errno=%d",errno);return false;}return false;}
int exclusiveFile(const char* name,char* cardPath,size_t cap){if(!uniqueDirectory())return -1;snprintf(cardPath,cap,"%s/%s",directory,name);char full[112];snprintf(full,sizeof(full),"/sd%s",cardPath);errno=0;int fd=::open(full,O_WRONLY|O_CREAT|O_EXCL,0600);logLine("NEW_FILE name=%s fd_ok=%u errno=%d (exclusive, no overwrite)",name,fd>=0,errno);return fd;}
bool closeWritten(int& fd,const char* phase){errno=0;int synced=::fsync(fd);int se=errno;errno=0;int closed=::close(fd);int ce=errno;fd=-1;logLine("SYNC_CLOSE stage=%s sync=%d errno=%d close=%d errno=%d",phase,synced,se,closed,ce);return synced==0&&closed==0;}
int openHttp(HTTPClient& http,NetworkClientSecure& tls){String url(URL);tls.useBuiltinCACertBundle();tls.setHandshakeTimeout(20);for(unsigned i=0;i<6;++i){if(!diag::safeHttps(url.c_str()))return -100;if(!http.begin(tls,url))return -101;http.setConnectTimeout(15000);http.setTimeout(20000);http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);const char* keys[]={"Location"};http.collectHeaders(keys,1);int code=http.GET();if(code==301||code==302||code==303||code==307||code==308){String next=http.header("Location");http.end();url=next;continue;}return code;}return -102;}
void networkRun(bool disk){bool mounted=false;int fd=-1;char path[96]={};bool ok=false;
 if(disk){mounted=mountCard("download");if(!mounted){unmountCard();return;}uint64_t total=SD.totalBytes(),used=SD.usedBytes();if(total<used||total-used<diag::FILE_BYTES+65536){logLine("SPACE insufficient or unknown");unmountCard();return;}}
 if(!connectPrivate(true)){wifiOff();if(mounted)unmountCard();return;}
 NetworkClientSecure tls;HTTPClient http;int code=openHttp(http,tls);logLine("HTTP code=%d length=%d (URL and headers intentionally omitted)",code,http.getSize());
 if(code==200&&http.getSize()==int(diag::FILE_BYTES)){
  if(disk)fd=exclusiveFile("download.pak",path,sizeof(path));
  if(!disk||fd>=0){Hash hash;bool good=hash.begin(),checkpoint=false;uint32_t received=0,start=millis(),last=start,nextDraw=0;auto stream=http.getStreamPtr();
   while(good&&received<diag::FILE_BYTES){if(uint32_t(millis()-start)>240000||uint32_t(millis()-last)>20000){logLine("HTTP time-limit pos=%lu",(unsigned long)received);good=false;break;}
    int available=stream->available();if(available<=0){if(!http.connected()){logLine("HTTP disconnected pos=%lu",(unsigned long)received);good=false;break;}delay(5);continue;}
    size_t n=std::min<size_t>(sizeof(buffer),std::min<uint32_t>(available,diag::FILE_BYTES-received));if(disk&&!checkpoint)n=std::min<size_t>(n,diag::CHECKPOINT-received);
    int got=stream->read(buffer,n);if(got<=0||size_t(got)>n){logLine("HTTP read failure pos=%lu got=%d",(unsigned long)received,got);good=false;break;}last=millis();
    if(!hash.update(buffer,got)){logLine("HASH update failed");good=false;break;}
    if(disk){errno=0;ssize_t written=::write(fd,buffer,got);int e=errno;if(written<0||!diag::exactWrite(got,size_t(written),e)){logLine("WRITE pos=%lu wanted=%d got=%ld errno=%d",(unsigned long)received,got,(long)written,e);good=false;break;}}
    received+=got;
    if(disk&&!checkpoint&&received==diag::CHECKPOINT){checkpoint=true;char expected[65];good=hash.snapshot(expected)&&closeWritten(fd,"checkpoint");if(good)good=verifyPath(path,"checkpoint-readback",false,received,expected,false);if(good){char full[112];snprintf(full,sizeof(full),"/sd%s",path);errno=0;fd=::open(full,O_WRONLY|O_APPEND);struct stat st;good=fd>=0&&::fstat(fd,&st)==0&&st.st_size==received;logLine("CHECKPOINT reopen_append=%u errno=%d",good,errno);}last=millis();}
    if(lcdReady&&received>=nextDraw){screen(disk?"WiFi -> novo arquivo SD":"WiFi sem SD",received);nextDraw=received+262144;}yield();
   }
   char actual[65]={};if(good)good=hash.finish(actual);if(actual[0])logLine("NETWORK_SHA %s",actual);ok=good&&received==diag::FILE_BYTES&&!strcmp(actual,fixture::SHA);logLine("NETWORK bytes=%lu expected=%lu sha_ok=%u",(unsigned long)received,(unsigned long)diag::FILE_BYTES,ok);
   if(fd>=0){bool closed=closeWritten(fd,"final");ok=ok&&closed;}if(disk&&ok)ok=verifyPath(path,"final-readback",true,diag::FILE_BYTES,fixture::SHA,false);
  }
 }
 if(fd>=0)closeWritten(fd,"error-close");http.end();wifiOff();if(mounted)unmountCard();logLine("NETWORK END disk=%u ok=%u; diagnostic file retained even on failure",disk,ok);screen("Teste WiFi concluido");
}
void saveReport(){if(!mountCard("save-report")){unmountCard();return;}char p[96];int fd=exclusiveFile("report.txt",p,sizeof(p));if(fd>=0){size_t bytes=report.used;errno=0;ssize_t n=::write(fd,report.data,bytes);int e=errno;bool good=n>=0&&diag::exactWrite(bytes,size_t(n),e);bool closeOk=closeWritten(fd,"report");logLine("REPORT_FILE requested=%lu written=%ld errno=%d closed=%u ok=%u",(unsigned long)bytes,(long)n,e,closeOk,good&&closeOk);}unmountCard();}
void setup(){Serial.begin(115200);Serial.setTxTimeoutMs(100);pinMode(1,OUTPUT);digitalWrite(1,LOW);pinMode(41,OUTPUT);pinMode(45,OUTPUT);deselect();SPI.begin(39,40,38);WiFi.persistent(false);logLine("%s flash=%lu PSRAM=%lu",VERSION,(unsigned long)ESP.getFlashChipSize(),(unsigned long)ESP.getPsramSize());logLine("PINS MOSI38 SCLK39 MISO40 SDCS41 LCDCS45 DC42 BL1; no reset GPIO (-1). Software checks are not electrical continuity tests.");logLine("Sketch has no direct save/NVS/OTA calls. Standard core startup can affect NVS. Use new board. No automatic tests.");logLine("P: existing 1MiB read OFF/ON/OFF, LCD off (fresh boot only)");logLine("4: storage4MHz | 1: storage1MHz (choose ONE per cold boot)");logLine("N: WiFi without SD | D: WiFi new SD file (after4/1)");logLine("R: RAM report | S: save new report file | H: commands");}
void command(char c){if(!diag::commandAllowed(privateInputFailed,c)){Serial.println("Entrada privada cancelada. Reinicie para novos testes; R recupera relatorio.");return;}if(c=='R'){Serial.println("--- RAM REPORT ---");Serial.print(report.data);Serial.printf("--- END truncated=%u ---\n",report.truncated);return;}if(c=='H'){Serial.println("P existing-file comparison (fresh boot); 4/1 storage; N network; D new SD download; R report; S new report file");return;}
 if(c=='D'&&!session.storage){Serial.println("Primeiro execute 4 ou 1.");return;}if(!session.take(c)){Serial.println("Comando invalido ou ja executado. Sem repeticao automatica.");return;}if(c=='P'){existingReadbackRun();}else if(c=='4'||c=='1'){hz=c=='4'?4000000:1000000;storageRun();}else if(c=='N'||c=='D')networkRun(c=='D');else if(c=='S')saveReport();}
void loop() {
  static char c = 0;
  static bool bad = false;
  while (Serial.available()) {
    int v = Serial.read();
    if (v == '\r') continue;
    if (v == '\n') {
      char pending = c;
      bool invalid = bad;
      c = 0;
      bad = false;
      if (!invalid && pending) command(pending);
      else if (invalid) Serial.println("Envie uma letra e Nova linha.");
      return;
    }
    if (c) bad = true;
    else c = char(v);
  }
  delay(10);
}
