#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cassert>
constexpr int OUTPUT=1,HIGH=1;
inline int pin41=0,pin45=0;
inline void pinMode(int pin,int mode){assert(pin==41&&mode==OUTPUT);}
inline void digitalWrite(int pin,int value){assert((pin==41||pin==45)&&value==HIGH);(pin==41?pin41:pin45)=value;}
