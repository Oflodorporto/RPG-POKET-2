#pragma once
#include "Arduino.h"
struct SPIClass{void begin(int sck,int miso,int mosi){assert(sdHigh&&lcdHigh&&sck==39&&miso==40&&mosi==38);sequence.push_back(1);}};inline SPIClass SPI;
