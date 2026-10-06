#pragma once
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "CardCheck.h"
// Waveshare SKU29667: SD and LCD share SPI39/38/40 with separate CS41/45.
// Invoke only between display transfers. Do not call SPI.end or change LCD settings.
struct SdReader {
  File probe;
  bool mount(){
    SD.end();pinMode(41,OUTPUT);digitalWrite(41,HIGH);digitalWrite(45,HIGH);
    // false disables formatting on mount failure. The same SPI object serves LCD.
    return SD.begin(41,SPI,4000000,"/sd",1,false)&&SD.cardType()!=CARD_NONE;
  }
  uint32_t capacityMiB(){return uint32_t(SD.cardSize()/(1024ull*1024ull));}
  bool openProbe(){probe=SD.open(CARD_PROBE_PATH,FILE_READ);return probe&&!probe.isDirectory();}
  size_t size(){return probe.size();}
  size_t read(uint8_t* out,size_t count){return probe.read(out,count);}
  void closeProbe(){if(probe)probe.close();}
  void unmount(){SD.end();digitalWrite(41,HIGH);digitalWrite(45,HIGH);}
};
