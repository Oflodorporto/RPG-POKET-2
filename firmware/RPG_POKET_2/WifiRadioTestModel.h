#pragma once
// Portable diagnostic protocol. No game state, Arduino dependency or heap storage.
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace wifi_radio_test {
static const uint32_t kMagic = 0x54574652u; // RFWT, deliberately separate from co-op
static const uint8_t kVersion = 1;
static const size_t kMaxPeers = 4, kMaxReports = 24, kMaxFrame = 200;
static const uint8_t kProbeCount = 30;
static const uint32_t kProbeCadenceMs = 700, kArmedLimitMs = 30000;
static const uint32_t kHandshakeMs = 5000, kRoundMs = 22700;
static const uint32_t kPreliminaryMs = 1500, kFinalMs = 3000;
static const int32_t kUnknown = -1;
inline bool elapsed(uint32_t now,uint32_t then,uint32_t ms){return int32_t(now-then)>=0 && uint32_t(now-then)>=ms;}
inline bool timeAtOrAfter(uint32_t a,uint32_t b){return int32_t(a-b)>=0;}
inline bool sameMac(const uint8_t*a,const uint8_t*b){return memcmp(a,b,6)==0;}
inline bool validMac(const uint8_t*m){static const uint8_t zero[6]={};return m && !(m[0]&1) && !sameMac(m,zero);}

enum class Distance : uint8_t { PairingNear=0, TwoPalms=1, OneMeter=2, TwoMeters=3 };
enum class Level : uint8_t { Baseline=0, N1=1, N2=2, N3=3, N4=4 };
enum class State : uint8_t { Closed, Browsing, Inviting, IncomingInvite, Pairing, DistancePause, Armed, Running, Preliminary, Finalizing, ManualPause, Complete, Cancelled, Fatal };
enum class Reason : uint8_t { None, Cancelled, PairTimeout, HandshakeTimeout, RoundTimeout, MissingRemote, ConfigMismatch, QueueOverflow, PowerFailure, RestoreFailure, PeerRejected, IdentityMismatch, TimingInvalid };
enum class Type : uint8_t { Beacon=1, Invite, Accept, Paired, Reject, Ready, Start, StartAck, Probe, Echo, Preliminary, Final, Cancel };
inline const char* distanceName(Distance d){switch(d){case Distance::TwoPalms:return "2_palmos";case Distance::OneMeter:return "1_m";case Distance::TwoMeters:return "2_m";default:return "pareamento_perto";}}
inline const char* levelName(Level l){switch(l){case Level::N1:return "N1";case Level::N2:return "N2";case Level::N3:return "N3";case Level::N4:return "N4";default:return "Atual";}}
inline int8_t requestedPower(Level l,int8_t baseline){const int8_t values[5]={baseline,20,44,60,80};return values[uint8_t(l)<=4?uint8_t(l):0];}
inline bool validSetting(Distance d,Level l){return uint8_t(d)>=1&&uint8_t(d)<=3&&uint8_t(l)<=4;}
inline uint8_t stageId(Distance d,Level l){return uint8_t((uint8_t(d)-1)*5+uint8_t(l)+1);}
struct PowerResult { int8_t requestedRaw=0, actualRaw=0; bool actualValid=false; int32_t error=0; };
inline bool powerOkay(const PowerResult&p){return p.actualValid&&p.error==0&&p.actualRaw>=8&&p.actualRaw<=80;}
struct Stats {
 uint16_t txAttempts=0,apiAccepted=0,apiErrors=0,macSuccess=0,macErrors=0,uniqueRx=0,echoes=0;
 uint16_t echoTxAttempts=0,echoApiAccepted=0,echoApiErrors=0,controlTxAttempts=0,controlApiAccepted=0,controlApiErrors=0;
 uint32_t rxMask=0,echoMask=0,rttSumMs=0,rttMinMs=0,rttMaxMs=0;
 bool rssiValid=false,timingValid=false,macTelemetryValid=false;
 int8_t rssiLast=0,rssiMin=0,rssiMax=0;
 uint32_t rssiAt=0,startedAt=0,endedAt=0,clockDomain=0;
};
struct Report {
 uint32_t session=0,run=0; uint8_t stage=0; Distance distance=Distance::PairingNear; Level level=Level::Baseline;
 uint8_t localMac[6]={},peerMac[6]={}; PowerResult localPower,remotePower,localRestore,remoteRestore;
 Stats local,remote;
 bool paired=false,localStarted=false,remoteStarted=false,remotePresent=false,remoteFinal=false;
 bool localRestoreAttempted=false,remoteRestoreAttempted=false,remoteRestoreConfirmed=false;
 bool queueOverflow=false,configMismatch=false,telemetryCorrelated=true,complete=false,manual=false;
 Reason reason=Reason::None; uint32_t armedAt=0,finishedAt=0;
 uint8_t channel=6,protocol=7,powerSave=0;int8_t localBaselineRaw=0,remoteBaselineRaw=0;
};
inline bool comparable(const Report&r){return r.paired&&r.localStarted&&r.remoteStarted&&r.remotePresent&&r.complete&&!r.queueOverflow&&!r.configMismatch&&r.local.timingValid&&r.remote.timingValid&&powerOkay(r.localPower)&&powerOkay(r.remotePower)&&r.localPower.actualRaw==r.remotePower.actualRaw;}
inline int32_t lossPermille(const Report&r,bool localToRemote){
 if(!comparable(r))return kUnknown;
 uint16_t sent=localToRemote?r.local.apiAccepted:r.remote.apiAccepted;
 uint16_t received=localToRemote?r.remote.uniqueRx:r.local.uniqueRx;
 return !sent||received>sent?kUnknown:int32_t((uint32_t(sent-received)*1000)/sent);
}
struct Peer { uint8_t mac[6]={}; uint32_t nonce=0,lastSeen=0; int8_t rssi=0; bool rssiValid=false; };
struct Hooks {
 void* context=nullptr;
 uint32_t (*nowMs)(void*)=nullptr;
 int32_t (*send)(void*,const uint8_t*,const uint8_t*,size_t)=nullptr;
 PowerResult (*applyPower)(void*,int8_t)=nullptr;
 PowerResult (*restorePower)(void*)=nullptr;
};
// Explicit fixed-width serialization avoids struct padding and native-endian wire ABI.
struct Writer { uint8_t* p;size_t n,at=0;bool okay=true; void u8(uint8_t v){if(at<n)p[at++]=v;else okay=false;} void u16(uint16_t v){u8(uint8_t(v));u8(uint8_t(v>>8));} void u32(uint32_t v){u16(uint16_t(v));u16(uint16_t(v>>16));} };
struct Reader {const uint8_t*p;size_t n,at=0;bool okay=true;uint8_t u8(){if(at<n)return p[at++];okay=false;return 0;}uint16_t u16(){uint16_t a=u8();return uint16_t(a|(uint16_t(u8())<<8));}uint32_t u32(){uint32_t a=u16();return a|(uint32_t(u16())<<16);} };
inline void writePower(Writer&w,const PowerResult&p){w.u8(uint8_t(p.requestedRaw));w.u8(uint8_t(p.actualRaw));w.u8(p.actualValid);w.u32(uint32_t(p.error));}
inline PowerResult readPower(Reader&r){PowerResult p;p.requestedRaw=int8_t(r.u8());p.actualRaw=int8_t(r.u8());p.actualValid=r.u8()!=0;p.error=int32_t(r.u32());return p;}
inline void writeStats(Writer&w,const Stats&s){w.u16(s.txAttempts);w.u16(s.apiAccepted);w.u16(s.apiErrors);w.u16(s.macSuccess);w.u16(s.macErrors);w.u16(s.uniqueRx);w.u16(s.echoes);w.u16(s.echoTxAttempts);w.u16(s.echoApiAccepted);w.u16(s.echoApiErrors);w.u16(s.controlTxAttempts);w.u16(s.controlApiAccepted);w.u16(s.controlApiErrors);w.u32(s.rxMask);w.u32(s.echoMask);w.u32(s.rttSumMs);w.u32(s.rttMinMs);w.u32(s.rttMaxMs);w.u8(s.rssiValid);w.u8(s.timingValid);w.u8(s.macTelemetryValid);w.u8(uint8_t(s.rssiLast));w.u8(uint8_t(s.rssiMin));w.u8(uint8_t(s.rssiMax));w.u32(s.rssiAt);w.u32(s.startedAt);w.u32(s.endedAt);w.u32(s.clockDomain);}
inline Stats readStats(Reader&r){Stats s;s.txAttempts=r.u16();s.apiAccepted=r.u16();s.apiErrors=r.u16();s.macSuccess=r.u16();s.macErrors=r.u16();s.uniqueRx=r.u16();s.echoes=r.u16();s.echoTxAttempts=r.u16();s.echoApiAccepted=r.u16();s.echoApiErrors=r.u16();s.controlTxAttempts=r.u16();s.controlApiAccepted=r.u16();s.controlApiErrors=r.u16();s.rxMask=r.u32();s.echoMask=r.u32();s.rttSumMs=r.u32();s.rttMinMs=r.u32();s.rttMaxMs=r.u32();s.rssiValid=r.u8()!=0;s.timingValid=r.u8()!=0;s.macTelemetryValid=r.u8()!=0;s.rssiLast=int8_t(r.u8());s.rssiMin=int8_t(r.u8());s.rssiMax=int8_t(r.u8());s.rssiAt=r.u32();s.startedAt=r.u32();s.endedAt=r.u32();s.clockDomain=r.u32();return s;}
struct WireFrame {
 Type type=Type::Beacon;uint32_t session=0,run=0,token=0,ackToken=0,nonce=0;
 uint8_t stage=0,seq=0;Distance distance=Distance::PairingNear;Level level=Level::Baseline;
 int8_t ceilingRaw=0,baselineRaw=0;uint8_t channel=6,protocol=7,powerSave=0;uint16_t flags=0;PowerResult power,restore;Stats stats;
};
enum WireFlag : uint16_t { Started=1, Paired=2, RestoreAttempted=4, SeenRemoteRestore=8, Overflow=16, Mismatch=32, CountsComplete=64, TelemetryCorrelated=128, Manual=256, TimingInvalid=512 };
inline bool validPower(const PowerResult&p){return !p.actualValid||(p.actualRaw>=8&&p.actualRaw<=80);}
inline unsigned popcount(uint32_t v){unsigned n=0;while(v){n+=v&1u;v>>=1;}return n;}
inline bool validStats(const Stats&s){return s.txAttempts<=kProbeCount&&uint32_t(s.echoApiAccepted)+s.echoApiErrors==s.echoTxAttempts&&uint32_t(s.controlApiAccepted)+s.controlApiErrors==s.controlTxAttempts&&s.apiAccepted<=s.txAttempts&&s.apiErrors<=s.txAttempts&&s.apiAccepted+s.apiErrors==s.txAttempts&&s.uniqueRx<=kProbeCount&&s.echoes<=s.apiAccepted&&!(s.rxMask>>kProbeCount)&&!(s.echoMask>>kProbeCount)&&popcount(s.rxMask)==s.uniqueRx&&popcount(s.echoMask)==s.echoes&&(!s.rssiValid||(s.rssiMin<=s.rssiLast&&s.rssiLast<=s.rssiMax))&&(!s.timingValid||(s.txAttempts==kProbeCount&&timeAtOrAfter(s.endedAt,s.startedAt)&&uint32_t(s.endedAt-s.startedAt)>=kRoundMs&&uint32_t(s.endedAt-s.startedAt)<=kRoundMs+200))&&(!s.echoes||(s.rttMinMs<=s.rttMaxMs&&s.rttMaxMs<=kRoundMs&&s.rttSumMs>=s.rttMinMs*s.echoes&&s.rttSumMs<=s.rttMaxMs*s.echoes));}
inline bool knownType(Type t){return uint8_t(t)>=1&&uint8_t(t)<=uint8_t(Type::Cancel);}
inline size_t encode(const WireFrame&f,uint8_t*out,size_t capacity){
 Writer w{out,capacity};w.u32(kMagic);w.u8(kVersion);w.u8(uint8_t(f.type));w.u32(f.session);w.u32(f.run);w.u32(f.token);w.u32(f.ackToken);w.u32(f.nonce);w.u8(f.stage);w.u8(f.seq);w.u8(uint8_t(f.distance));w.u8(uint8_t(f.level));w.u8(uint8_t(f.ceilingRaw));w.u8(f.channel);w.u8(f.protocol);w.u8(f.powerSave);w.u8(uint8_t(f.baselineRaw));w.u16(f.flags);writePower(w,f.power);writePower(w,f.restore);writeStats(w,f.stats);return w.okay?w.at:0;
}
inline bool decode(const uint8_t*data,size_t n,WireFrame&f,bool validate=true){
 if(!data||n>kMaxFrame)return false;
 Reader r{data,n};
 if(r.u32()!=kMagic||r.u8()!=kVersion)return false;
 f.type=Type(r.u8());f.session=r.u32();f.run=r.u32();f.token=r.u32();f.ackToken=r.u32();f.nonce=r.u32();f.stage=r.u8();f.seq=r.u8();f.distance=Distance(r.u8());f.level=Level(r.u8());f.ceilingRaw=int8_t(r.u8());f.channel=r.u8();f.protocol=r.u8();f.powerSave=r.u8();f.baselineRaw=int8_t(r.u8());f.flags=r.u16();f.power=readPower(r);f.restore=readPower(r);f.stats=readStats(r);
 return r.okay&&r.at==n&&knownType(f.type)&&uint8_t(f.distance)<=3&&uint8_t(f.level)<=4&&f.seq<kProbeCount&&(!validate||(f.channel>=1&&f.channel<=14&&f.protocol&&f.powerSave<=2&&validPower(f.power)&&validPower(f.restore)&&((f.type!=Type::Preliminary&&f.type!=Type::Final)||validStats(f.stats))));
}
inline uint32_t hashBytes(uint32_t h,const uint8_t*p,size_t n){for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
inline uint32_t hashNumber(uint32_t h,uint32_t v){uint8_t b[4]={uint8_t(v),uint8_t(v>>8),uint8_t(v>>16),uint8_t(v>>24)};return hashBytes(h,b,4);}
inline uint32_t sessionId(const uint8_t*a,uint32_t an,const uint8_t*b,uint32_t bn){if(memcmp(a,b,6)>0){const uint8_t*t=a;a=b;b=t;uint32_t t2=an;an=bn;bn=t2;}uint32_t h=hashBytes(2166136261u,a,6);h=hashNumber(h,an);h=hashBytes(h,b,6);h=hashNumber(h,bn);return h?h:1;}
inline uint32_t runId(uint32_t session,uint32_t leaderToken,uint32_t followerToken,uint8_t stage){uint32_t h=hashNumber(session,leaderToken);h=hashNumber(h,followerToken);h=hashNumber(h,stage);return h?h:1;}
} // namespace wifi_radio_test
