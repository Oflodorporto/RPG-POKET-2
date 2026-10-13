#include "../firmware/RPG_POKET_2/SharedStorage.h"
#include <fstream>
#include <string>
#include <cstdio>
int main(){for(bool success:{false,true}){sequence.clear();sdHigh=lcdHigh=false;mountSuccess=success;assert(primeSharedSd()==success);assert(sequence==std::vector<int>({41,45,141,145,1,2,3,141,145}));}
 std::ifstream f("firmware/RPG_POKET_2/RPG_POKET_2.ino");assert(f);std::string text{std::istreambuf_iterator<char>(f),{}};auto setup=text.substr(text.find("void setup(){"));auto sd=setup.find("primeSharedSd();"),lcd=setup.find("lcd.begin(");assert(sd!=std::string::npos&&sd<lcd&&setup.find("lcd.")==lcd);
 puts("PASS: actual shared-SPI startup helper, both mount outcomes; CS41/45 high, explicit39/40/38 bus, 4MHz/no format, closed mount before first LCD initialization; no write/erase APIs in fake.");}
