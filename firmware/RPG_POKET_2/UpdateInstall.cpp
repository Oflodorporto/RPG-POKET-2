#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include <algorithm>
#include <errno.h>
#include "UpdateInstallBridge.h"
#include "ArtPromotion.h"

SemaphoreHandle_t updateSpi=nullptr,updateGuard=nullptr;
updater::Info updateWorker;updater::Plan updatePlan;
char downloadReason[39]="";
storageDiag::Recovery updateRecovery;unsigned updateSdHz=4000000;
uint64_t updateInitialFree=0;bool updateInitialFreeKnown=false;

bool installUpdate(bool artOnly){using namespace updater;
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
