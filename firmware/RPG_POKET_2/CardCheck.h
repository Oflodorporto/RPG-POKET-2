#pragma once
#include <stdint.h>
#include <stddef.h>
// Optional read-only card check. Independent of Game, NVS and Arduino APIs.
enum class CardStatus:uint8_t {Unchecked,Unavailable,Missing,WrongFile,ReadError,Verified};
struct CardInfo {CardStatus status=CardStatus::Unchecked;uint32_t capacityMiB=0;};
constexpr size_t CARD_PROBE_SIZE=4096;
constexpr const char* CARD_PROBE_PATH="/RPGPOKET/teste.bin";
inline uint32_t cardCrcUpdate(uint32_t crc,const uint8_t* bytes,size_t count){
  for(size_t i=0;i<count;++i){crc^=bytes[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&uint32_t(-int(crc&1)));}return crc;
}
// Reader: mount, capacityMiB, openProbe, size, read, closeProbe, unmount.
// There is deliberately no write, erase, format or save operation in this interface.
template<class Reader> CardInfo checkCard(Reader& io){
  CardInfo result;
  if(!io.mount()){io.unmount();result.status=CardStatus::Unavailable;return result;}
  result.capacityMiB=io.capacityMiB();
  if(!io.openProbe()){io.closeProbe();io.unmount();result.status=CardStatus::Missing;return result;}
  if(io.size()!=CARD_PROBE_SIZE){io.closeProbe();io.unmount();result.status=CardStatus::WrongFile;return result;}
  uint8_t block[512];uint32_t actual=~0u,expected=~0u;size_t position=0;
  while(position<CARD_PROBE_SIZE){
    if(io.read(block,sizeof(block))!=sizeof(block)){io.closeProbe();io.unmount();result.status=CardStatus::ReadError;return result;}
    actual=cardCrcUpdate(actual,block,sizeof(block));
    for(size_t i=0;i<sizeof(block);++i)block[i]=uint8_t((position+i)*73u+19u);
    expected=cardCrcUpdate(expected,block,sizeof(block));position+=sizeof(block);
  }
  io.closeProbe();io.unmount();result.status=actual==expected?CardStatus::Verified:CardStatus::WrongFile;return result;
}
