#pragma once
#include <cstdint>
#if defined(ARDUINO_ARCH_ESP32)
#include <sdkconfig.h>
#endif
#ifndef RPG_ENABLE_WIFI_RADIO_TEST
#if defined(ARDUINO)
#define RPG_ENABLE_WIFI_RADIO_TEST 1
#else
#define RPG_ENABLE_WIFI_RADIO_TEST 0
#endif
#endif
#if RPG_ENABLE_WIFI_RADIO_TEST != 0 && RPG_ENABLE_WIFI_RADIO_TEST != 1
#error "RPG_ENABLE_WIFI_RADIO_TEST must be 0 or 1"
#endif
namespace wifi_radio_test {
// Diagnostic presets use IDF 5.5's actual table, not Arduino's misleading
// WIFI_POWER_21dBm label: inputs 80..84 all map to raw 80 (20 dBm).
constexpr uint8_t kDeviceChannel=6;
constexpr unsigned kDeviceInbox=8;
constexpr uint32_t kDeviceMaximumQueueAgeMs=350;
constexpr int8_t kDevicePresetRaw[4]={20,44,60,80};
constexpr int8_t kDeviceMaximumRaw=80;
#if defined(CONFIG_ESP_PHY_ENABLE_USB) && CONFIG_ESP_PHY_ENABLE_USB
constexpr int8_t kUsbPhyCompiled=1;
#else
#if defined(ARDUINO_ARCH_ESP32)
constexpr int8_t kUsbPhyCompiled=0;
#else
constexpr int8_t kUsbPhyCompiled=-1;
#endif
#endif
#if defined(CONFIG_ESP_PHY_REDUCE_TX_POWER) && CONFIG_ESP_PHY_REDUCE_TX_POWER
constexpr int8_t kReduceTxPowerCompiled=1;
#else
#if defined(ARDUINO_ARCH_ESP32)
constexpr int8_t kReduceTxPowerCompiled=0;
#else
constexpr int8_t kReduceTxPowerCompiled=-1;
#endif
#endif
// These flags identify compiled support only; they do not establish the active
// USB PHY state, a runtime reduction, conducted RF output or antenna power.
}
