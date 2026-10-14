#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "UpdateModel.h"
// Network runs in a worker; only SD/LCD SPI operations share this mutex.
extern SemaphoreHandle_t updateSpi,updateGuard;
extern updater::Info updateWorker;extern updater::Plan updatePlan;
extern char downloadReason[39];
// Sample the existing preflight result only; do not query a failing card to log an error.
extern storageDiag::Recovery updateRecovery;extern unsigned updateSdHz;
extern uint64_t updateInitialFree;extern bool updateInitialFreeKnown;
struct UpdateSpiLock {UpdateSpiLock(){if(updateSpi)xSemaphoreTake(updateSpi,portMAX_DELAY);}~UpdateSpiLock(){if(updateSpi)xSemaphoreGive(updateSpi);}};
struct PartialUpdate {uint32_t bytes=0;bool uncertain=false;storageDiag::Failure failure;};
void updateReport(updater::State state,const char* msg,unsigned percent=0,bool busy=true);
bool updateFail(const char* s);
void downloadFile(updater::DownloadFile file);
void recordVerifyFailure(const storageDiag::Failure& failure);
bool recoverUpdateRead(storageDiag::Failure& f);
bool verifiedFile(const char* path,const char* expected,uint32_t bytes,bool art,storageDiag::Failure& detail);
PartialUpdate inspectPartial(const updater::Package& p,const char* path);
void deletePartial(const char* path);
bool downloadUpdate(const updater::Package& p,const char* path,unsigned base,unsigned span);
bool installUpdate(bool artOnly);
