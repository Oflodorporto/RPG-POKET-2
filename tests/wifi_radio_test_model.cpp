#include "WifiRadioTestModel.h"
#include "WifiRadioTestCsv.h"
#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>
using namespace wifi_radio_test;
static std::vector<std::string> fields(const char*s){std::vector<std::string> v;std::string f;bool quote=false;for(;*s&&*s!='\n';++s){if(*s=='"'){if(quote&&s[1]=='"'){f+='"';++s;}else quote=!quote;}else if(*s==','&&!quote){v.push_back(f);f.clear();}else f+=*s;}v.push_back(f);return v;}
int main(){
 static_assert(kMaxFrame<=250,"ESP-NOW v1 transport bound");static_assert(kMaxPeers==4&&kMaxReports==24,"bounded RAM contract");
 assert(requestedPower(Level::N1,80)==20);assert(requestedPower(Level::N2,80)==44);assert(requestedPower(Level::N3,80)==60);assert(requestedPower(Level::N4,80)==80);assert(requestedPower(Level::Baseline,80)==80);
 assert(!elapsed(90,100,1));assert(elapsed(10,UINT32_MAX-10,21));assert(!elapsed(9,UINT32_MAX-10,21));assert(!strcmp(distanceName(Distance::PairingNear),"pareamento_perto"));
 WireFrame f;f.type=Type::Final;f.session=77;f.run=91;f.token=32;f.ackToken=42;f.nonce=0x12345678;f.stage=15;f.distance=Distance::TwoMeters;f.level=Level::N4;f.ceilingRaw=80;f.seq=29;f.flags=511;f.power={80,76,true,-77};f.restore={80,80,true,0};f.stats.txAttempts=30;f.stats.apiAccepted=28;f.stats.apiErrors=2;f.stats.macErrors=4;f.stats.uniqueRx=20;f.stats.rxMask=(1u<<20)-1;f.stats.rssiLast=-71;f.stats.rssiMin=-91;f.stats.rssiMax=-44;f.stats.rssiValid=true;f.stats.rssiAt=UINT32_MAX;f.stats.clockDomain=45;
 uint8_t bytes[kMaxFrame];size_t n=encode(f,bytes,sizeof(bytes));assert(n&&n<kMaxFrame);WireFrame g;assert(decode(bytes,n,g));assert(g.session==77&&g.run==91&&g.nonce==0x12345678&&g.power.error==-77&&g.stats.rssiMin==-91&&g.stats.rssiAt==UINT32_MAX);
 for(size_t i=0;i<n;++i){assert(!decode(bytes,i,g));}
 bytes[0]^=1;assert(!decode(bytes,n,g));bytes[0]^=1;bytes[4]++;assert(!decode(bytes,n,g));bytes[4]--;assert(!decode(bytes,n+1,g));assert(!encode(f,bytes,5));
 Report r;assert(lossPermille(r,true)==-1);r.paired=r.localStarted=r.remoteStarted=r.remotePresent=r.complete=true;r.local.timingValid=r.remote.timingValid=true;r.localPower=r.remotePower={80,80,true,0};r.local.apiAccepted=30;r.remote.apiAccepted=20;r.local.uniqueRx=18;r.remote.uniqueRx=24;assert(comparable(r));assert(lossPermille(r,true)==200);assert(lossPermille(r,false)==100);
 r.remotePower.actualRaw=76;assert(lossPermille(r,true)==-1);r.remotePower.actualRaw=80;r.queueOverflow=true;assert(lossPermille(r,true)==-1);r.queueOverflow=false;r.remotePresent=false;assert(lossPermille(r,true)==-1);r.remotePresent=true;r.local.apiAccepted=0;assert(lossPermille(r,true)==-1);
 char header[4096],row[4096];CsvContext c{"build,\"quoted\"",77,1,0,1};assert(csvHeader(header,sizeof(header)));assert(csvRow(Report{},c,row,sizeof(row)));auto h=fields(header),v=fields(row);assert(h.size()==v.size());assert(v[0]=="build,\"quoted\"");assert(v[8]=="pareamento_perto");assert(v[15]=="-1"&&v[38]=="-1"&&v[45]=="-1"&&v[47]=="not_measured");assert(!csvRow(r,c,row,5));assert(!csvHeader(header,5));
 Stats corrupt;corrupt.txAttempts=30;corrupt.apiAccepted=30;corrupt.uniqueRx=2;corrupt.rxMask=1;assert(!validStats(corrupt));corrupt.uniqueRx=1;assert(validStats(corrupt));corrupt.apiErrors=1;assert(!validStats(corrupt));corrupt.apiErrors=0;corrupt.echoes=1;assert(!validStats(corrupt));corrupt.echoMask=1;corrupt.rttMinMs=10;corrupt.rttMaxMs=5;assert(!validStats(corrupt));corrupt.rttMaxMs=20;corrupt.rttSumMs=10;assert(validStats(corrupt));corrupt.timingValid=true;corrupt.startedAt=100;corrupt.endedAt=100+kRoundMs+201;assert(!validStats(corrupt));corrupt.endedAt=100+kRoundMs;assert(validStats(corrupt));corrupt.rxMask=1u<<30;assert(!validStats(corrupt));
 const uint8_t a[6]={2,1,2,3,4,5},b[6]={2,2,2,3,4,5};assert(sessionId(a,7,b,8)==sessionId(b,8,a,7));assert(sessionId(a,7,b,8)!=sessionId(a,7,b,9));assert(runId(5,7,9,1)!=runId(5,7,9,2));
 printf("wifi_radio_test_model PASS: wire %zu bytes; CSV %zu columns; unknowns, quarter-dBm, bounded storage, wrap-safe time\n",n,h.size());
}
