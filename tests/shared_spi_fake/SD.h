#pragma once
#include "SPI.h"
#include <cstring>
inline bool mountSuccess=true;
struct SDFake{bool begin(int cs,SPIClass& spi,unsigned hz,const char* path,unsigned files,bool format){assert(sequence.back()==1&&sdHigh&&lcdHigh&&cs==41&&&spi==&SPI&&hz==4000000&&!strcmp(path,"/sd")&&files==1&&!format);sequence.push_back(2);return mountSuccess;}void end(){assert(sequence.back()==2);sequence.push_back(3);}};inline SDFake SD;
