#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
constexpr int ESP_OK=0;
enum wifi_mode_t {WIFI_MODE_NULL,WIFI_MODE_STA,WIFI_MODE_AP,WIFI_MODE_APSTA};
constexpr auto WIFI_STA=WIFI_MODE_STA;constexpr int WIFI_IF_STA=0,WL_CONNECTED=3;
enum wifi_second_chan_t {WIFI_SECOND_CHAN_NONE};
enum wifi_storage_t {WIFI_STORAGE_FLASH,WIFI_STORAGE_RAM};
namespace fake {
inline wifi_mode_t mode=WIFI_MODE_NULL;inline wifi_storage_t storage=WIFI_STORAGE_FLASH;
inline bool autoReconnect=true,connected=false,persistent=true,radio=false,callbackSet=false;
inline int fail=0,mutations=0,nvsWrites=0,erases=0,sendCount=0,initCount=0,deinitCount=0,reconnects=0,peerDeletes=0;
inline uint8_t channel=11,own[6]={2,0,0,0,0,1},peers[8][6]{};inline unsigned peerCount=0;
inline void reset(){mode=WIFI_MODE_NULL;storage=WIFI_STORAGE_FLASH;autoReconnect=true;connected=false;persistent=true;radio=callbackSet=false;fail=mutations=nvsWrites=erases=sendCount=initCount=deinitCount=reconnects=peerDeletes=0;channel=11;peerCount=0;}
}
