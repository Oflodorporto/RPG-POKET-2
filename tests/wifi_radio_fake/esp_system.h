#pragma once
#include "FakeCore.h"
inline int esp_reset_reason(){return 1;}
inline uint32_t esp_random(){return 0x12345678;}
