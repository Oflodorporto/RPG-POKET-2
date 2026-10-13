#pragma once
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include <cJSON.h>
#include <time.h>
#include <cmath>
#include <errno.h>
#include "UpdateModel.h"
#include "ArtPromotion.h"
// Network runs in a worker; only SD/LCD SPI operations share this mutex.
inline SemaphoreHandle_t updateSpi=nullptr,updateGuard=nullptr;
inline updater::Info updateWorker;inline updater::Plan updatePlan;
struct UpdateSpiLock {UpdateSpiLock(){if(updateSpi)xSemaphoreTake(updateSpi,portMAX_DELAY);}~UpdateSpiLock(){if(updateSpi)xSemaphoreGive(updateSpi);}};
inline void initUpdater(){updateSpi=xSemaphoreCreateMutex();updateGuard=xSemaphoreCreateMutex();}
inline void updateReport(updater::State state,const char* msg,unsigned percent=0,bool busy=true){xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.state=state;updateWorker.progress=percent;updateWorker.busy=busy;snprintf(updateWorker.message,sizeof(updateWorker.message),"%s",msg);xSemaphoreGive(updateGuard);}
inline bool tickUpdater(){if(!updateGuard)return false;xSemaphoreTake(updateGuard,portMAX_DELAY);bool changed=memcmp(&updateInfo,&updateWorker,sizeof(updateInfo))!=0;updateInfo=updateWorker;xSemaphoreGive(updateGuard);return changed;}
inline bool updateFail(const char* s){updateReport(updater::State::Error,s,0,false);return false;}
inline bool jsonString(cJSON* o,const char* key,char* out,size_t n){auto v=cJSON_GetObjectItemCaseSensitive(o,key);if(!cJSON_IsString(v)||!v->valuestring||strlen(v->valuestring)>=n)return false;strcpy(out,v->valuestring);return true;}
inline bool jsonNumber(cJSON* o,const char* key,uint32_t& out){auto v=cJSON_GetObjectItemCaseSensitive(o,key);if(!cJSON_IsNumber(v)||!std::isfinite(v->valuedouble)||v->valuedouble<0||v->valuedouble>UINT32_MAX||double(uint32_t(v->valuedouble))!=v->valuedouble)return false;out=uint32_t(v->valuedouble);return true;}
inline bool jsonPackage(cJSON* o,const char* key,updater::Package& p){auto v=cJSON_GetObjectItemCaseSensitive(o,key);return cJSON_IsObject(v)&&jsonString(v,"url",p.url,sizeof(p.url))&&jsonString(v,"sha256",p.sha,sizeof(p.sha))&&jsonNumber(v,"bytes",p.bytes);}
inline bool parseUpdate(const char* body,updater::Plan& p){cJSON* j=cJSON_Parse(body);if(!j)return false;char board[40],partition[40],crc[9];uint32_t schema=0;bool ok=jsonNumber(j,"schema",schema)&&schema==2&&jsonString(j,"board",board,sizeof(board))&&!strcmp(board,"Waveshare-29667")&&jsonString(j,"partition",partition,sizeof(partition))&&!strcmp(partition,"app3M_fat9M_16MB")&&jsonString(j,"version",p.version,sizeof(p.version))&&jsonNumber(j,"build",p.build)&&jsonNumber(j,"save_format",p.saveFormat)&&jsonNumber(j,"writes_save_format",p.writesSaveFormat)&&jsonPackage(j,"firmware",p.firmware)&&jsonPackage(j,"art",p.art)&&jsonNumber(cJSON_GetObjectItemCaseSensitive(j,"art"),"assets",p.assets)&&jsonString(cJSON_GetObjectItemCaseSensitive(j,"art"),"payload_crc32",crc,sizeof(crc));
  // Save29 starts a new, explicitly USB-installed save generation. Future OTA
  // must read the format this firmware actually writes; it need not claim legacy
  // formats1..28, which this firmware deliberately cannot import.
  if(ok){auto reads=cJSON_GetObjectItemCaseSensitive(j,"reads_save_formats");p.readsCurrentSaves=false;if(cJSON_IsArray(reads))for(auto item=reads->child;item;item=item->next)if(cJSON_IsNumber(item)&&item->valuedouble==rpg::CURRENT_SAVE_FORMAT){p.readsCurrentSaves=true;break;}ok=p.readsCurrentSaves;}
  if(ok){if(strlen(crc)!=8)ok=false;else for(unsigned i=0;i<8;++i)if(!((crc[i]>='0'&&crc[i]<='9')||(crc[i]>='a'&&crc[i]<='f')))ok=false;p.artCrc=strtoul(crc,nullptr,16);}cJSON_Delete(j);return ok;}
inline bool tlsUrl(const String& s){return s.startsWith("https://github.com/")||s.startsWith("https://release-assets.githubusercontent.com/")||s.startsWith("https://objects.githubusercontent.com/");}
inline int openUpdateHttp(HTTPClient& h,NetworkClientSecure& tls,const char* url,uint32_t offset=0){tls.useBuiltinCACertBundle();tls.setHandshakeTimeout(25);String next(url);for(unsigned redirects=0;redirects<6;++redirects){if(!tlsUrl(next))return -1;if(!h.begin(tls,next))return -1;h.setConnectTimeout(15000);h.setTimeout(45000);h.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);const char* keys[]={"Location","Content-Range"};h.collectHeaders(keys,2);if(offset){char range[40];snprintf(range,sizeof(range),"bytes=%lu-",(unsigned long)offset);h.addHeader("Range",range);}int rc=h.GET();if(rc==301||rc==302||rc==303||rc==307||rc==308){next=h.header("Location");h.end();continue;}return rc;}return -1;}
inline bool updateTime(){configTzTime("BRT3","pool.ntp.org","time.google.com");uint32_t start=millis();while(time(nullptr)<1767225600){if(WiFi.status()!=WL_CONNECTED||millis()-start>20000)return false;vTaskDelay(pdMS_TO_TICKS(100));}return true;}
inline char downloadReason[39]="";
// Sample the existing preflight result only; do not query a failing card to log an error.
inline storageDiag::Recovery updateRecovery;inline unsigned updateSdHz=4000000;
inline uint64_t updateInitialFree=0;inline bool updateInitialFreeKnown=false;
inline void downloadFile(updater::DownloadFile file){xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.file=file;xSemaphoreGive(updateGuard);}
inline void recordShortWrite(updater::DownloadFile file,uint32_t offset,uint32_t total,size_t requested,size_t written,unsigned percent,bool append,int error){
  updater::WriteFailure failure;failure.valid=true;failure.file=file;failure.offset=offset;failure.total=total;failure.requested=requested;failure.written=written;failure.progress=percent;failure.append=append;failure.error=error;failure.freeKnown=updateInitialFreeKnown;failure.initialFree=updateInitialFree;
  xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.lastWrite=failure;updateWorker.lastVerify={};xSemaphoreGive(updateGuard);
}
inline void downloadError(const char* reason){if(WiFi.status()!=WL_CONNECTED)snprintf(downloadReason,sizeof(downloadReason),"%s",reason);else snprintf(downloadReason,sizeof(downloadReason),"%s / %lddBm",reason,(long)WiFi.RSSI());}
inline bool readNetwork(HTTPClient& http,uint8_t* buffer,size_t n){auto stream=http.getStreamPtr();size_t off=0;uint32_t last=millis();while(off<n){int available=stream->available();if(available>0){int got=stream->read(buffer+off,std::min(n-off,size_t(available)));if(got<=0){downloadError("Rede: leitura falhou");return false;}off+=got;last=millis();}else{if(!http.connected()||WiFi.status()!=WL_CONNECTED||millis()-last>45000){downloadError(WiFi.status()!=WL_CONNECTED?"Wi-Fi desconectou":!http.connected()?"Servidor encerrou":"Rede: tempo esgotado");return false;}vTaskDelay(pdMS_TO_TICKS(2));}}return true;}
inline void recordVerifyFailure(const storageDiag::Failure& failure){if(!updateGuard)return;xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.lastVerify=failure;updateWorker.lastWrite={};xSemaphoreGive(updateGuard);}
inline bool fileHash(const char* path,const char* expected,uint32_t bytes,bool art=false,storageDiag::Failure* detail=nullptr){
  using storageDiag::Issue;if(detail)detail->issue=Issue::None;
  auto fail=[&](Issue why,uint32_t off,uint32_t want,uint32_t got,int code=0){storageDiag::set(detail,why,off,want,got,code);return false;};
  File f;{UpdateSpiLock lock;errno=0;f=SD.open(path,FILE_READ);int code=errno;if(!f||code){if(f)f.close();return fail(Issue::Open,0,0,0,code);}errno=0;unsigned size=f.size();code=errno;if(size!=bytes||code){f.close();return fail(Issue::Size,0,bytes,size,code);}}
  uint8_t buffer[4096],header[16]={},digest[32]={};mbedtls_sha256_context ctx;mbedtls_sha256_init(&ctx);int hashResult=mbedtls_sha256_starts(&ctx,0);
  uint32_t remain=bytes,crc=~0u,offset=0;bool good=hashResult==0;if(!good)fail(Issue::HashApi,0,0,0,hashResult);
  while(good&&remain){size_t n=std::min(remain,uint32_t(sizeof(buffer)));int got,code;{UpdateSpiLock lock;errno=0;got=f.read(buffer,n);code=errno;}
    if(got!=int(n)||code){good=fail(Issue::Read,offset,n,got<0?0:unsigned(got),code);break;}
    hashResult=mbedtls_sha256_update(&ctx,buffer,n);if(hashResult){good=fail(Issue::HashApi,offset,0,0,hashResult);break;}
    if(art){if(offset==0)memcpy(header,buffer,16);for(size_t i=offset==0?16:0;i<n;++i){crc^=buffer[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&uint32_t(-int(crc&1)));}}offset+=n;remain-=n;vTaskDelay(1);
  }
  if(good){hashResult=mbedtls_sha256_finish(&ctx,digest);if(hashResult)good=fail(Issue::HashApi,offset,0,0,hashResult);}
  mbedtls_sha256_free(&ctx);{UpdateSpiLock lock;f.close();}if(!good)return false;
  if(art){
    if(memcmp(header,"PKA1",4))return fail(Issue::Magic,0,0x31414b50,rpg::get32(header,0));
    if(rpg::get32(header,4)!=bytes-16)return fail(Issue::Length,4,bytes-16,rpg::get32(header,4));
    if(rpg::get32(header,8)!=updatePlan.artCrc)return fail(Issue::HeaderCrc,8,updatePlan.artCrc,rpg::get32(header,8));
    if(rpg::get32(header,12)!=updatePlan.assets)return fail(Issue::Catalog,12,updatePlan.assets,rpg::get32(header,12));
    if(~crc!=updatePlan.artCrc)return fail(Issue::Crc,16,updatePlan.artCrc,~crc);
  }
  char hash[65];for(unsigned i=0;i<32;++i)snprintf(hash+2*i,3,"%02x",digest[i]);
  if(strcmp(hash,expected)){char prefix[9]={};memcpy(prefix,expected,8);uint32_t want=strtoul(prefix,nullptr,16);memcpy(prefix,hash,8);return fail(Issue::Sha,0,want,strtoul(prefix,nullptr,16));}return true;
}
inline void recordRecovery(){if(!updateGuard)return;xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.recovery=updateRecovery;xSemaphoreGive(updateGuard);}
inline bool recoverUpdateRead(storageDiag::Failure& f){
 if(!storageDiag::ioFailure(f)||updateRecovery.attempts)return false;
 updateRecovery.note(f);updateRecovery.attempts=1;recordRecovery();
 xSemaphoreTake(updateGuard,portMAX_DELAY);unsigned progress=updateWorker.progress;xSemaphoreGive(updateGuard);
 updateReport(updater::State::Verifying,"SD: unica releitura a 1 MHz",progress);
#ifdef ARDUINO
 Serial.printf("[SD recovery] first issue=%u file=%u offset=%lu requested=%lu got=%lu errno=%d\n",unsigned(f.issue),unsigned(f.file),(unsigned long)f.offset,(unsigned long)f.expected,(unsigned long)f.actual,f.code);
#endif
 bool ok;{UpdateSpiLock lock;SD.end();digitalWrite(41,HIGH);digitalWrite(45,HIGH);vTaskDelay(pdMS_TO_TICKS(150));updateSdHz=1000000;errno=0;ok=SD.begin(41,SPI,updateSdHz,"/sd",1,false);int code=errno;if(!ok)storageDiag::set(&f,storageDiag::Issue::Mount,0,0,0,code);}
 if(!ok){updateRecovery.note(f);recordRecovery();}return ok;
}
inline bool verifiedFile(const char* path,const char* expected,uint32_t bytes,bool art,storageDiag::Failure& detail){
 bool ok=fileHash(path,expected,bytes,art,&detail);
 if(!ok&&recoverUpdateRead(detail))ok=fileHash(path,expected,bytes,art,&detail);
 if(updateRecovery.attempts){if(!ok)updateRecovery.note(detail);updateRecovery.recovered=ok;recordRecovery();}
 return ok;
}
struct PartialUpdate {uint32_t bytes=0;bool uncertain=false;storageDiag::Failure failure;};
inline PartialUpdate inspectPartial(const updater::Package& p,const char* path){
 PartialUpdate out;const bool firmware=!strcmp(path,"/RPGPOKET/firmware.part");out.failure.stage=storageDiag::Stage::Identity;out.failure.file=firmware?storageDiag::File::FirmwareTag:storageDiag::File::ArtTag;
 char tag[64];snprintf(tag,sizeof(tag),"%s.tag",path);UpdateSpiLock lock;errno=0;File identity=SD.open(tag,FILE_READ);int code=errno;
 auto fail=[&](storageDiag::Issue issue,uint32_t wanted,uint32_t got,int e){storageDiag::set(&out.failure,issue,0,wanted,got,e);out.uncertain=storageDiag::ioFailure(out.failure);};
 if(!identity||code){bool opened=bool(identity);if(identity)identity.close();if(opened||code!=ENOENT){fail(storageDiag::Issue::Open,0,0,code);out.uncertain=true;}return out;}
 errno=0;unsigned identitySize=identity.size();code=errno;
 if(identitySize!=64||code){identity.close();fail(storageDiag::Issue::Size,64,identitySize,code);return out;}
 char hash[65]={};errno=0;int got=identity.read(reinterpret_cast<uint8_t*>(hash),64);code=errno;identity.close();
 if(got!=64||code){fail(storageDiag::Issue::Read,64,got<0?0:unsigned(got),code);return out;}
 if(strcmp(hash,p.sha))return out; // Complete foreign identity: not resumable.
 out.failure.file=firmware?storageDiag::File::FirmwarePart:storageDiag::File::ArtPart;
 errno=0;File f=SD.open(path,FILE_READ);code=errno;if(!f||code){bool opened=bool(f);if(f)f.close();if(opened||code!=ENOENT){fail(storageDiag::Issue::Open,0,0,code);out.uncertain=true;}return out;}
 errno=0;unsigned size=f.size();code=errno;f.close();if(code){fail(storageDiag::Issue::Size,p.bytes,size,code);return out;}
 if(size<=p.bytes)out.bytes=size;return out;
}
inline uint32_t partialUpdate(const updater::Package& p,const char* path){return inspectPartial(p,path).bytes;}
inline void deletePartial(const char* path){char tag[64];snprintf(tag,sizeof(tag),"%s.tag",path);SD.remove(path);SD.remove(tag);}
inline bool downloadUpdate(const updater::Package& p,const char* path,unsigned base,unsigned span){
  downloadReason[0]=0;const auto file=!strcmp(path,"/RPGPOKET/firmware.part")?updater::DownloadFile::Firmware:updater::DownloadFile::Art;downloadFile(file);auto partial=inspectPartial(p,path);
  if(partial.uncertain&&recoverUpdateRead(partial.failure))partial=inspectPartial(p,path);
  if(partial.uncertain){recordVerifyFailure(partial.failure);updateRecovery.note(partial.failure);recordRecovery();downloadError("SD: controle ilegivel; preservado");return false;}
  uint32_t downloaded=partial.bytes;char tag[64];snprintf(tag,sizeof(tag),"%s.tag",path);
  {UpdateSpiLock lock;if(!downloaded){deletePartial(path);File identity=SD.open(tag,FILE_WRITE);if(!identity){downloadError("SD: criar controle");return false;}bool ok=identity.write(reinterpret_cast<const uint8_t*>(p.sha),64)==64;identity.flush();identity.close();if(!ok){downloadError("SD: gravar controle");return false;}}}
  // Keep complete blocks on the card. A dropped connection resumes automatically.
  for(unsigned attempt=0;attempt<5&&downloaded<p.bytes;++attempt){
    if(attempt){updateReport(updater::State::Downloading,"Retomando download...",base+uint64_t(downloaded)*span/p.bytes);if(WiFi.status()!=WL_CONNECTED){WiFi.reconnect();uint32_t start=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-start<10000)vTaskDelay(pdMS_TO_TICKS(100));}vTaskDelay(pdMS_TO_TICKS(600));}
    NetworkClientSecure tls;HTTPClient http;int rc=openUpdateHttp(http,tls,p.url,downloaded);
    if(rc==200&&downloaded){downloaded=0;UpdateSpiLock lock;if(!SD.remove(path)){http.end();downloadError("SD: reiniciar arquivo");return false;}}
    bool response=rc==200||rc==206;
    if(rc==206){unsigned long first=0,last=0,total=0;String range=http.header("Content-Range");response=sscanf(range.c_str(),"bytes %lu-%lu/%lu",&first,&last,&total)==3&&first==downloaded&&last==p.bytes-1&&total==p.bytes;}
    if(!response||http.getSize()!=int(p.bytes-downloaded)){if(rc!=200&&rc!=206){char reason[24];snprintf(reason,sizeof(reason),"HTTP %d",rc);downloadError(reason);}else downloadError("Resposta incompativel");http.end();continue;}
    File out;{UpdateSpiLock lock;out=SD.open(path,downloaded?FILE_APPEND:FILE_WRITE);}if(!out){http.end();downloadError("SD: abrir arquivo");return false;}
    const bool append=downloaded!=0;uint8_t buffer[4096];bool diskOK=true;while(downloaded<p.bytes){size_t n=std::min(uint32_t(sizeof(buffer)),p.bytes-downloaded);if(!readNetwork(http,buffer,n))break;
      size_t written=0;int writeError=0;{UpdateSpiLock lock;errno=0;written=out.write(buffer,n);writeError=errno;}
      if(written!=n||writeError){recordShortWrite(file,downloaded,p.bytes,n,written,base+uint64_t(downloaded)*span/p.bytes,append,writeError);diskOK=false;break;}downloaded+=n;updateReport(updater::State::Downloading,"Mantenha a alimentacao",base+uint64_t(downloaded)*span/p.bytes);vTaskDelay(1);}
    {UpdateSpiLock lock;out.flush();out.close();}http.end();if(!diskOK){UpdateSpiLock lock;deletePartial(path);downloadError("SD: gravacao falhou");return false;}
  }return downloaded==p.bytes;
}
inline bool resumablePlan(const updater::Plan& p,bool artOnly){{UpdateSpiLock lock;digitalWrite(45,HIGH);digitalWrite(41,HIGH);if(!SD.begin(41,SPI,4000000,"/sd",1,false)){SD.end();return false;}}auto fw=artOnly?PartialUpdate{}:inspectPartial(p.firmware,"/RPGPOKET/firmware.part");auto art=inspectPartial(p.art,"/RPGPOKET/artes.part");bool have=fw.bytes||fw.uncertain||art.bytes||art.uncertain;{UpdateSpiLock lock;SD.end();}return have;}
inline void checkUpdate(){using namespace updater;if(!updateTime()){updateFail("Sem horario seguro / confira Wi-Fi");return;}NetworkClientSecure tls;HTTPClient http;int rc=openUpdateHttp(http,tls,"https://github.com/Oflodorporto/RPG-POKET-2/releases/latest/download/manifest.json");if(rc==404){http.end();updateFail("Ainda nao ha versao publicada");return;}int length=http.getSize();if(rc!=200||length<=0||length>4096){http.end();updateFail("Nao foi possivel verificar");return;}char body[4097];bool got=readNetwork(http,reinterpret_cast<uint8_t*>(body),length);http.end();body[length]=0;Plan p;const esp_partition_t* target=esp_ota_get_next_update_partition(nullptr);
  if(!got||!parseUpdate(body,p)||!target||!valid(p,target->size,ESP.getPsramSize())||(p.build==FW_BUILD&&(strcmp(p.version,FW_VERSION)||p.artCrc!=ART_CRC||p.assets!=ART_COUNT||p.art.bytes!=ART_BYTES+16))){updateFail("Manifesto incompativel / invalido");return;}
  updatePlan=p;bool resume=resumablePlan(p,p.build==FW_BUILD);xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.canResume=resume;snprintf(updateWorker.version,sizeof(updateWorker.version),"%s",p.version);updateWorker.artOnly=p.build==FW_BUILD;bool needed=p.build>FW_BUILD||menu.art!=ArtStatus::Ready||p.artCrc!=ART_CRC;updateWorker.state=needed?State::Available:State::Current;updateWorker.busy=false;snprintf(updateWorker.message,sizeof(updateWorker.message),"%s",needed?"Confirmar para baixar e instalar":"Voce ja tem a versao atual");xSemaphoreGive(updateGuard);
}
// Return from the C++ scope first so HTTP/TLS destructors free their heap.
inline void updateCheckTask(void*){checkUpdate();vTaskDelete(nullptr);}
inline bool installUpdate(bool artOnly){using namespace updater;
  downloadFile(DownloadFile::None);updateInitialFreeKnown=false;updateInitialFree=0;updateRecovery={};updateSdHz=4000000;
  xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.canResume=false;updateWorker.stagingRetained=false;updateWorker.recovery={};updateWorker.lastVerify={};xSemaphoreGive(updateGuard);
  const char* firmwarePart="/RPGPOKET/firmware.part";const char* artPart="/RPGPOKET/artes.part";
  char finalArt[48],backupArt[48];artPath(finalArt,sizeof(finalArt),updatePlan.artCrc);artBackupPath(backupArt,sizeof(backupArt),updatePlan.artCrc);
  storageDiag::Failure verification;verification.stage=storageDiag::Stage::Preparing;verification.file=storageDiag::File::Card;
  bool mounted;{UpdateSpiLock lock;digitalWrite(45,HIGH);digitalWrite(41,HIGH);errno=0;mounted=SD.begin(41,SPI,updateSdHz,"/sd",1,false);if(!mounted)storageDiag::set(&verification,storageDiag::Issue::Mount,0,0,0,errno);}
  if(!mounted)mounted=recoverUpdateRead(verification);
  if(!mounted){recordVerifyFailure(verification);UpdateSpiLock lock;SD.end();return updateFail("Cartao indisponivel / outro cartao");}
  auto fail=[&](const char* message,bool retain){
    xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.canResume=retain;updateWorker.stagingRetained=retain;xSemaphoreGive(updateGuard);
    {UpdateSpiLock lock;SD.end();digitalWrite(41,HIGH);digitalWrite(45,HIGH);}return updateFail(message);
  };
  {UpdateSpiLock lock;errno=0;bool folder=SD.exists("/RPGPOKET");int folderError=errno;if((!folder&&folderError!=ENOENT)||(!folder&&!SD.mkdir("/RPGPOKET"))){SD.end();return updateFail("SD: preparar pasta / dados salvos");}}
  auto fw=artOnly?PartialUpdate{}:inspectPartial(updatePlan.firmware,firmwarePart);auto ap=inspectPartial(updatePlan.art,artPart);
  if(fw.uncertain||ap.uncertain){auto detail=fw.uncertain?fw.failure:ap.failure;if(recoverUpdateRead(detail)){fw=artOnly?PartialUpdate{}:inspectPartial(updatePlan.firmware,firmwarePart);ap=inspectPartial(updatePlan.art,artPart);}if(fw.uncertain||ap.uncertain){recordVerifyFailure(fw.uncertain?fw.failure:ap.failure);return fail("Falha conferir SD / controle salvo",true);}}
  ArtPresence presence;{UpdateSpiLock lock;presence=artPresence(SD,finalArt);if(presence==ArtPresence::Uncertain){verification.file=storageDiag::File::VersionedArt;storageDiag::set(&verification,storageDiag::Issue::Open,0,0,0,errno);}}
  if(presence==ArtPresence::Uncertain&&recoverUpdateRead(verification)){UpdateSpiLock lock;presence=artPresence(SD,finalArt);if(presence==ArtPresence::Uncertain)storageDiag::set(&verification,storageDiag::Issue::Open,0,0,0,errno);}
  if(presence==ArtPresence::Uncertain){recordVerifyFailure(verification);return fail("Falha conferir SD / pacote guardado",true);}
  bool existing=presence==ArtPresence::Present,reuse=false;
  if(existing){verification.stage=storageDiag::Stage::Installed;verification.file=storageDiag::File::VersionedArt;reuse=verifiedFile(finalArt,updatePlan.art.sha,updatePlan.art.bytes,true,verification);
    // An unreadable prior package is not permission to replace it.
    if(!reuse&&storageDiag::ioFailure(verification)){recordVerifyFailure(verification);return fail("Falha conferir SD / pacote guardado",true);}}
  {UpdateSpiLock lock;uint64_t needed=uint64_t(reuse?0:updatePlan.art.bytes-ap.bytes)+(artOnly?0:updatePlan.firmware.bytes-fw.bytes)+1048576;errno=0;uint64_t total=SD.totalBytes();int totalError=errno;errno=0;uint64_t used=SD.usedBytes();int usedError=errno;updateInitialFreeKnown=total>0&&total>=used&&!totalError&&!usedError;updateInitialFree=updateInitialFreeKnown?total-used:0;if(!updateInitialFreeKnown||updateInitialFree<needed){SD.end();return updateFail(updateInitialFreeKnown?"Sem espaco / arquivos preservados":"SD: espaco nao confirmado");}}
  bool ok=true;if(!artOnly)ok=downloadUpdate(updatePlan.firmware,firmwarePart,0,25);if(ok&&!reuse)ok=downloadUpdate(updatePlan.art,artPart,artOnly?0:25,artOnly?80:55);
  if(!ok){auto f=artOnly?PartialUpdate{}:inspectPartial(updatePlan.firmware,firmwarePart);auto a=inspectPartial(updatePlan.art,artPart);return fail(*downloadReason?downloadReason:"Download pausado / pode retomar",f.bytes||f.uncertain||a.bytes||a.uncertain);}
  verification={};verification.stage=storageDiag::Stage::Downloaded;
  if(!artOnly){downloadFile(DownloadFile::Firmware);updateReport(State::Verifying,"Conferindo firmware no SD",80);verification.file=storageDiag::File::FirmwarePart;ok=verifiedFile(firmwarePart,updatePlan.firmware.sha,updatePlan.firmware.bytes,false,verification);}
  if(ok&&!reuse){downloadFile(DownloadFile::Art);updateReport(State::Verifying,"Conferindo artes no SD",80);verification.file=storageDiag::File::ArtPart;ok=verifiedFile(artPart,updatePlan.art.sha,updatePlan.art.bytes,true,verification);}
  if(!ok){recordVerifyFailure(verification);bool retain=!storageDiag::integrityFailure(verification);
    if(!retain){UpdateSpiLock lock;deletePartial(verification.file==storageDiag::File::FirmwarePart?firmwarePart:artPart);}
    return fail(retain?"Falha conferir SD / arquivo guardado":"Falha conferir SD / baixar de novo",retain);}
  ArtPromotion promotion;
  if(!reuse){UpdateSpiLock lock;ok=promoteArt(SD,artPart,finalArt,backupArt,promotion);if(!ok){verification.stage=storageDiag::Stage::Promotion;verification.file=storageDiag::File::VersionedArt;storageDiag::set(&verification,promotion.issue==PromotionIssue::BackupExists?storageDiag::Issue::BackupExists:promotion.issue==PromotionIssue::Lookup?storageDiag::Issue::Open:storageDiag::Issue::Rename,0,0,0,promotion.error);}}
  if(ok){verification.stage=storageDiag::Stage::Installed;verification.file=storageDiag::File::VersionedArt;updateReport(State::Verifying,"Conferindo artes instaladas",80);ok=verifiedFile(finalArt,updatePlan.art.sha,updatePlan.art.bytes,true,verification);}
  if(!ok){recordVerifyFailure(verification);{UpdateSpiLock lock;restoreArt(SD,artPart,finalArt,backupArt,promotion);}return fail("Falha conferir SD / pacote guardado",true);}
  const esp_partition_t* target=esp_ota_get_next_update_partition(nullptr);esp_ota_handle_t handle=0;bool opened=false;
  if(!artOnly){
    updateReport(State::Installing,"Instalando na particao livre",85);ok=target&&esp_ota_begin(target,updatePlan.firmware.bytes,&handle)==ESP_OK;opened=ok;File f;
    verification={};verification.stage=storageDiag::Stage::Installing;verification.file=storageDiag::File::FirmwarePart;
    if(ok){UpdateSpiLock lock;errno=0;f=SD.open(firmwarePart,FILE_READ);ok=bool(f)&&!errno;if(!ok)storageDiag::set(&verification,storageDiag::Issue::Open,0,0,0,errno);}
    uint8_t buffer[4096],digest[32]={};mbedtls_sha256_context hash;mbedtls_sha256_init(&hash);if(ok){int code=mbedtls_sha256_starts(&hash,0);ok=code==0;if(!ok)storageDiag::set(&verification,storageDiag::Issue::HashApi,0,0,0,code);}uint32_t copied=0;
    while(ok&&copied<updatePlan.firmware.bytes){size_t n=std::min(uint32_t(sizeof(buffer)),updatePlan.firmware.bytes-copied);int got,code;{UpdateSpiLock lock;errno=0;got=f.read(buffer,n);code=errno;}
      ok=got==int(n)&&!code;if(!ok)storageDiag::set(&verification,storageDiag::Issue::Read,copied,n,got<0?0:unsigned(got),code);
      if(ok){int code=mbedtls_sha256_update(&hash,buffer,n);ok=code==0;if(!ok)storageDiag::set(&verification,storageDiag::Issue::HashApi,copied,0,0,code);if(ok)ok=esp_ota_write(handle,buffer,n)==ESP_OK;}
      if(ok)copied+=n;updateReport(State::Installing,"Instalando na particao livre",85+uint64_t(copied)*10/updatePlan.firmware.bytes);vTaskDelay(1);
    }
    if(f){UpdateSpiLock lock;f.close();}
    if(ok){int code=mbedtls_sha256_finish(&hash,digest);ok=code==0;if(!ok)storageDiag::set(&verification,storageDiag::Issue::HashApi,copied,0,0,code);else{char actual[65];for(unsigned i=0;i<32;++i)snprintf(actual+2*i,3,"%02x",digest[i]);if(strcmp(actual,updatePlan.firmware.sha)){ok=false;char prefix[9]={};memcpy(prefix,updatePlan.firmware.sha,8);uint32_t want=strtoul(prefix,nullptr,16);memcpy(prefix,actual,8);storageDiag::set(&verification,storageDiag::Issue::Sha,0,want,strtoul(prefix,nullptr,16));}}}
    mbedtls_sha256_free(&hash);if(ok){ok=esp_ota_end(handle)==ESP_OK;opened=false;}if(opened)esp_ota_abort(handle);
  }
  if(!ok){if(verification.valid())recordVerifyFailure(verification);return fail("Instalacao falhou / arquivos salvos",true);}
  // No boot change until complete validation, including the bytes copied to OTA.
  if(!artOnly&&esp_ota_set_boot_partition(target)!=ESP_OK)return fail("Ativacao falhou / arquivos salvos",true);
  updateReport(State::Cleaning,"Removendo arquivos temporarios",96);{UpdateSpiLock lock;deletePartial(firmwarePart);deletePartial(artPart);SD.end();digitalWrite(41,HIGH);digitalWrite(45,HIGH);}
  updateReport(State::Restarting,updateRecovery.attempts?"Verificado apos releitura / reinicia":"Atualizado / reiniciando",100);vTaskDelay(pdMS_TO_TICKS(1800));ESP.restart();return true;
}
inline void updateInstallTask(void*){bool sleep=WiFi.getSleep();WiFi.setSleep(false);installUpdate(updatePlan.build==FW_BUILD);WiFi.setSleep(sleep);vTaskDelete(nullptr);}
inline bool startUpdate(bool install){if(!updateGuard||!updateSpi||updateInfo.busy)return false;if(menu.scanning||menu.connecting){updateFail("Aguarde a busca ou conexao Wi-Fi");tickUpdater();return false;}if(WiFi.status()!=WL_CONNECTED){updateFail("Conecte o Wi-Fi primeiro");tickUpdater();return false;}if(install&&updateInfo.state!=updater::State::Available&&!updateInfo.canResume)return false;downloadFile(updater::DownloadFile::None);updateReport(install?updater::State::Downloading:updater::State::Checking,install?"Preparando arquivos...":"Consultando GitHub...",0,true);tickUpdater();if(xTaskCreate(install?updateInstallTask:updateCheckTask,"rpg-update",24576,nullptr,1,nullptr)!=pdPASS){updateFail("Memoria insuficiente para atualizar");tickUpdater();return false;}return true;}

inline void resetUpdateScreen(){if(!updateGuard||updateInfo.busy)return;xSemaphoreTake(updateGuard,portMAX_DELAY);const auto last=updateWorker.lastWrite;const auto verify=updateWorker.lastVerify;const auto recovery=updateWorker.recovery;updateWorker=updater::Info{};updateWorker.lastWrite=last;updateWorker.lastVerify=verify;updateWorker.recovery=recovery;updateInfo=updateWorker;xSemaphoreGive(updateGuard);}

inline updater::State networkReturnState=updater::State::Idle;
inline void testNetwork(){unsigned successes=0,drops=0,trials=0;uint32_t totalMs=0;int low=WiFi.RSSI(),high=low;
  for(unsigned i=0;i<3;++i){++trials;bool online=WiFi.status()==WL_CONNECTED;if(!online)++drops;int dbm=online?WiFi.RSSI():-100;low=std::min(low,dbm);high=std::max(high,dbm);xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.netTrials=trials;xSemaphoreGive(updateGuard);
    uint32_t started=millis();bool ok=false;if(online&&updateTime()){NetworkClientSecure tls;HTTPClient http;int rc=openUpdateHttp(http,tls,"https://github.com/Oflodorporto/RPG-POKET-2/releases/latest/download/manifest.json");int length=http.getSize();if(rc==200&&length>0&&length<=4096){char body[4097];ok=readNetwork(http,reinterpret_cast<uint8_t*>(body),length);body[length]=0;updater::Plan p;if(ok)ok=parseUpdate(body,p);}http.end();}totalMs+=millis()-started;if(ok)++successes;if(online&&WiFi.status()!=WL_CONNECTED)++drops;vTaskDelay(pdMS_TO_TICKS(500));
  }xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.tested=true;updateWorker.netTrials=trials;updateWorker.netSuccess=successes;updateWorker.netDrop=drops;updateWorker.minDbm=low;updateWorker.maxDbm=high;updateWorker.avgMs=totalMs/trials;updateWorker.state=networkReturnState;updateWorker.busy=false;xSemaphoreGive(updateGuard);
}
inline void networkTestTask(void*){testNetwork();vTaskDelete(nullptr);}
inline bool startNetworkTest(){if(!updateGuard||updateInfo.busy)return false;if(menu.scanning||menu.connecting){updateFail("Aguarde a busca ou conexao Wi-Fi");tickUpdater();return false;}if(WiFi.status()!=WL_CONNECTED){updateFail("Conecte o Wi-Fi primeiro");tickUpdater();return false;}networkReturnState=updateInfo.state;xSemaphoreTake(updateGuard,portMAX_DELAY);updateWorker.tested=false;updateWorker.netTrials=0;xSemaphoreGive(updateGuard);updateReport(updater::State::Testing,"Testando acesso ao GitHub",0,true);tickUpdater();if(xTaskCreate(networkTestTask,"rpg-nettest",24576,nullptr,1,nullptr)!=pdPASS){updateFail("Memoria insuficiente para testar");tickUpdater();return false;}return true;}
