#pragma once
#include "FakeCore.h"
inline int64_t esp_timer_get_time(){return int64_t(fake::timeMs.load())*1000;}
