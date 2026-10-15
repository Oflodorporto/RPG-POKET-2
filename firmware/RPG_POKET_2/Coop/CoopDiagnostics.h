#pragma once
#include <cstdint>
#include <algorithm>
namespace coopdiag {
inline void increment(uint32_t& value){if(value!=UINT32_MAX)++value;}
// A callback may be stamped just after the loop captured now. Only tolerate
// a bounded 16ms future skew; old/ambiguous half-range ages remain stale.
inline uint32_t elapsed(uint32_t now,uint32_t at){const uint32_t value=now-at;
 if(value<=0x7fffffffu)return value;
 return uint32_t(at-now)<=16u?0:UINT32_MAX;
}
struct Signal {
 bool valid=false;int8_t last=0,minimum=0,maximum=0;uint8_t channel=0,rate=0;
 uint32_t at=0,samples=0;
 void observe(int8_t rssi,uint8_t ch,uint8_t phy,uint32_t now){
  if(!valid){minimum=maximum=rssi;valid=true;}else{minimum=std::min(minimum,rssi);maximum=std::max(maximum,rssi);}
  last=rssi;channel=ch;rate=phy;at=now;increment(samples);
 }
 uint32_t age(uint32_t now)const{return elapsed(now,at);}
 bool fresh(uint32_t now)const{return valid&&age(now)<2800;}
 uint8_t bars(uint32_t now)const{return !fresh(now)?0:last>=-55?4:last>=-67?3:last>=-78?2:1;}
};
struct CallbackStats {uint32_t received=0,invalid=0,unicastOk=0,unicastFail=0,broadcastOk=0,broadcastFail=0;};
struct Snapshot {
 Signal signal{};CallbackStats callbacks{};
 uint32_t now=0,accepted=0,rejected=0,apiOk=0,apiFail=0,queueDrops=0,stale=0,maxQueueAge=0,maxTickGap=0;
 uint32_t txAdmission[10]{},rxAdmission[10]{};
 uint32_t ackAccepted=0,retries=0,snapshots=0,outgoingDrops=0,rxAge=0,stallAge=0,channelDrifts=0;
 int32_t sendError=0,peerError=0,disconnectResult=-1,initError=0,callbackError=0,teardownError=0;
 int32_t channelError=-1,psError=-1,powerError=-1,protocolError=-1,modeError=-1;
 uint8_t channel=0,secondary=0,mode=0,ps=0,protocol=0,state=0,lastEnd=0;
 int8_t powerQuarterDbm=0;bool connected=false,awaitingAck=false,pending=false,paused=false,frozen=false,selectedPeer=false;
};
static_assert(sizeof(Snapshot)<=320,"Diagnostic snapshot must remain bounded");
}
