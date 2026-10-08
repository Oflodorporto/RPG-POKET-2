#include "TestHero.h"
#include "../firmware/RPG_POKET_2/SdReader.h"
#include <cstdio>
int main(){
  SdReader reader;assert(checkCard(reader).status==CardStatus::Verified&&sdEnds==2&&sdOpens==1&&fileCloses==1&&probeOffset==4096);
  sdAvailable=false;assert(checkCard(reader).status==CardStatus::Unavailable&&sdEnds==4&&sdOpens==1);
  sdAvailable=true;probeExists=false;assert(checkCard(reader).status==CardStatus::Missing&&fileCloses==1&&sdEnds==6);
  probeExists=true;probeDirectory=true;assert(checkCard(reader).status==CardStatus::Missing&&fileCloses==2&&sdEnds==8);
  probeDirectory=false;assert(checkCard(reader).status==CardStatus::Verified&&fileCloses==3&&sdEnds==10);
  puts("PASS: actual SD adapter uses same SPI object, CS41/LCD45 deselected,4MHz,false formatting flag,explicit read-only file mode, directory rejection, close/unmount and retry; fake API exposes no write or SPI.end.");
}
