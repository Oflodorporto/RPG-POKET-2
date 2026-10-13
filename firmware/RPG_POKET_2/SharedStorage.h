#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
// SKU29667 LCD and SD share SPI39/40/38. SD must enter SPI mode before
// any LCD traffic, including LCD initialization (Espressif sdspi_share).
// No files are created here; format-on-failure is explicitly disabled.
inline bool primeSharedSd(){
  pinMode(41,OUTPUT);pinMode(45,OUTPUT);digitalWrite(41,HIGH);digitalWrite(45,HIGH);
  SPI.begin(39,40,38);
  const bool mounted=SD.begin(41,SPI,4000000,"/sd",1,false);
  SD.end();digitalWrite(41,HIGH);digitalWrite(45,HIGH);
  // Unmount does not power-cycle the card; it remains in SPI mode.
  return mounted;
}
