#pragma once
#include <stdint.h>
// Release requires 30 ms of consecutive valid, untouched samples.
// An I2C error cannot re-arm an ongoing press.
struct TouchGate {
  static constexpr uint32_t releaseMs=30;
  bool armed = false, releasing = false;
  uint32_t since = 0;
  bool update(bool valid, bool down, uint32_t now) {
    if (!valid) { releasing = false; return false; }
    if (down) {
      releasing = false;
      if (!armed) return false;
      armed = false;
      return true;
    }
    if (!releasing) { releasing = true; since = now; }
    if (uint32_t(now - since) >= releaseMs) armed = true;
    return false;
  }
};
