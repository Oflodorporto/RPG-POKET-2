#!/usr/bin/env python3
"""Prove the isolated installer is a mechanical extraction of the CI4 source.

Reverse only the declared linkage/include moves and require the exact original
UpdateDevice.h SHA-256. This works in a shallow checkout without fetching history.
No normalization of bodies, strings, whitespace, arguments or comments is allowed.
"""
from pathlib import Path
import hashlib
import re

ROOT = Path(__file__).resolve().parents[1]
BASELINE = '7c9e0db7276558772d2cd6df556590ca6b9e3921'
BASELINE_SHA256 = '1ef88bec8ffeb9315851e4a41f722310b423fb36247e6972d6045eaaa599b9ed'
HELPERS = ('updateReport', 'updateFail', 'downloadFile', 'recordVerifyFailure',
           'recoverUpdateRead', 'verifiedFile', 'inspectPartial', 'deletePartial',
           'downloadUpdate')
STATES = ('updateSpi', 'updateGuard', 'updateWorker', 'updatePlan', 'downloadReason',
          'updateRecovery', 'updateSdHz', 'updateInitialFree', 'updateInitialFreeKnown')
STATE_DEFINITIONS = (
    'SemaphoreHandle_t updateSpi=nullptr,updateGuard=nullptr;',
    'updater::Info updateWorker;updater::Plan updatePlan;',
    'char downloadReason[39]="";',
    'storageDiag::Recovery updateRecovery;unsigned updateSdHz=4000000;',
    'uint64_t updateInitialFree=0;bool updateInitialFreeKnown=false;',
)
LOCK = 'struct UpdateSpiLock {UpdateSpiLock(){if(updateSpi)xSemaphoreTake(updateSpi,portMAX_DELAY);}~UpdateSpiLock(){if(updateSpi)xSemaphoreGive(updateSpi);}};'
PARTIAL = 'struct PartialUpdate {uint32_t bytes=0;bool uncertain=false;storageDiag::Failure failure;};'
MUTEX_COMMENT = '// Network runs in a worker; only SD/LCD SPI operations share this mutex.'
FREE_COMMENT = '// Sample the existing preflight result only; do not query a failing card to log an error.'
BRIDGE = '''#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "UpdateModel.h"
'''+MUTEX_COMMENT+'''
extern SemaphoreHandle_t updateSpi,updateGuard;
extern updater::Info updateWorker;extern updater::Plan updatePlan;
extern char downloadReason[39];
'''+FREE_COMMENT+'''
extern storageDiag::Recovery updateRecovery;extern unsigned updateSdHz;
extern uint64_t updateInitialFree;extern bool updateInitialFreeKnown;
'''+LOCK+'\n'+PARTIAL+'''
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
'''
INSTALL_PREFIX = '''#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include <algorithm>
#include <errno.h>
#include "UpdateInstallBridge.h"
#include "ArtPromotion.h"

'''+ '\n'.join(STATE_DEFINITIONS)+'\n\n'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def replace_once(text, old, new):
    require(text.count(old) == 1, f'Expected exactly one boundary: {old}')
    return text.replace(old, new, 1)


def verify(device, install, bridge):
    require(bridge == BRIDGE, 'Bridge declarations, defaults or types changed')
    require(install.startswith(INSTALL_PREFIX), 'Installer includes or state initialization changed')
    function = install[len(INSTALL_PREFIX):]
    require(function.startswith('bool installUpdate(bool artOnly){'), 'Installer must be a strong definition')
    require('installUpdate(' not in device.replace('installUpdate(updatePlan.build==FW_BUILD)', ''),
            'Installer body/declaration must not remain in the sketch-owned header')
    original = replace_once(device, '#include "UpdateInstallBridge.h"', '#include "UpdateModel.h"')
    original = replace_once(original,
        '// Sketch-owned helper definitions; UpdateInstall.cpp uses the declared bridge.',
        MUTEX_COMMENT+'\ninline '+STATE_DEFINITIONS[0]+'\ninline '+
        STATE_DEFINITIONS[1].replace(';updater::Plan', ';inline updater::Plan')+'\n'+LOCK)
    original = replace_once(original, 'void downloadFile(updater::DownloadFile file){',
        'inline '+STATE_DEFINITIONS[2]+'\n'+FREE_COMMENT+'\ninline '+
        STATE_DEFINITIONS[3].replace(';unsigned', ';inline unsigned')+'\ninline '+
        STATE_DEFINITIONS[4].replace(';bool', ';inline bool')+'\nvoid downloadFile(updater::DownloadFile file){')
    original = replace_once(original, 'PartialUpdate inspectPartial(', PARTIAL+'\nPartialUpdate inspectPartial(')
    for name in HELPERS:
        original, count = re.subn(r'^(\w+ '+name+r'\()', r'inline \1', original, flags=re.M)
        require(count == 1, f'Expected one strong sketch helper: {name}')
    original = replace_once(original,
        'unsigned percent,bool busy){', 'unsigned percent=0,bool busy=true){')
    original = replace_once(original, 'inline void updateInstallTask(',
        'inline '+function+'inline void updateInstallTask(')
    require(hashlib.sha256(original.encode()).hexdigest() == BASELINE_SHA256,
            'Reconstructed UpdateDevice.h differs from the pinned CI4 source')
    return hashlib.sha256(function.split('{', 1)[1].encode()).hexdigest()


def main():
    folder = ROOT / 'firmware/RPG_POKET_2'
    device = (folder / 'UpdateDevice.h').read_text()
    install = (folder / 'UpdateInstall.cpp').read_text()
    bridge = (folder / 'UpdateInstallBridge.h').read_text()
    body_sha = verify(device, install, bridge)
    # Check that the proof rejects representative drift without modifying files.
    mutations = (
        (device, install.replace('vTaskDelay(1);', 'vTaskDelay(2);', 1), bridge),
        (device, install.replace('updateSdHz=4000000;', 'updateSdHz=1000000;', 1), bridge),
        (device, install, bridge.replace('percent=0', 'percent=1', 1)),
        (device.replace('WiFi.setSleep(false)', 'WiFi.setSleep(true)', 1), install, bridge),
        (device.replace('bool updateFail(', 'inline bool updateFail(', 1), install, bridge),
        (device, install+'\nint unexpected_state=0;\n', bridge),
    )
    for candidate in mutations:
        try:
            verify(*candidate)
        except ValueError:
            continue
        raise ValueError('Equivalence proof accepted a negative mutation')
    print(f'PASS: reconstructed exact UpdateDevice.h from {BASELINE}; install body SHA256 {body_sha}')
    print('PASS: 9 strong helpers, 9 extern/single-definition states, exact types/defaults/initializers; unchanged task/Wi-Fi policy; 6 negative mutations rejected')


if __name__ == '__main__':
    main()
