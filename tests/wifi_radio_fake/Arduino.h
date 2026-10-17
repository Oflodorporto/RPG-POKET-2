#pragma once
#include "FakeCore.h"
inline uint32_t millis(){return fake::timeMs.load();}
