#pragma once
#include "CoopViewModel.h"
#include "CoopSession.h"
namespace coopplatform {
// Same-loop capture for FrameBuffer checkpoints. No Session action is dispatched
// from a renderer, radio callback or I2C read. Invalid/overflowed contact cancels.
class Input {
 struct Sample {bool valid=false,down=false;int16_t x=0,y=0;uint32_t now=0;uint64_t stamp=0;};
 coop::Queue<Sample,16> queue{};bool overflow=false,armed=false,down=false,releasing=false;
 uint32_t releaseAt=0;uint64_t pressedStamp=0;coopview::Control pressed=coopview::Control::None;
 static uint64_t mix(uint64_t h,uint64_t n){for(unsigned i=0;i<8;++i){h^=uint8_t(n>>(8*i));h*=1099511628211ULL;}return h;}
 static uint64_t identity(const Peer* p,uint64_t h){
  h=mix(h,p&&p->valid);if(!p)return h;for(auto b:p->mac.bytes)h=mix(h,b);
  h=mix(h,p->available);h=mix(h,p->sessionToken);
  uint8_t bytes[27]{};coop::Writer w{bytes,sizeof(bytes)};coop::writeActor(w,p->profile);for(auto b:bytes)h=mix(h,b);return h;
 }
 void cancel(){armed=down=releasing=false;pressed=coopview::Control::None;pressedStamp=0;}
public:
 uint32_t dropped=0,stale=0;
 static uint64_t fingerprint(const Session& session){
  uint64_t h=mix(1469598103934665603ULL,uint8_t(session.state()));h=mix(h,session.sessionToken());h=mix(h,session.joinToken());
  h=identity(session.pendingPeer(),h);for(size_t i=0;i<Session::kMaxPeers;++i)h=identity(session.peer(i),h);return h;
 }
 void reset(){queue.clear();overflow=false;cancel();}
 void capture(Session& session,bool valid,bool touching,int32_t px,int32_t py,uint32_t now){
  if(session.state()==SessionState::Playing)session.training().captureSample(valid,touching,px,py,now);
  if(overflow){++dropped;return;}
  const auto p=toLandscape(px,py);
  Sample sample{valid&&(!touching||p.x>=0),touching,p.x,p.y,now,fingerprint(session)};
  // Rendering can capture many identical levels before the loop dispatches.
  // Keep the first AND latest time of each unchanged run: release debounce
  // needs its start, while every error, edge, drag and state stamp must survive.
  auto same=[](const Sample& a,const Sample& b){return a.valid==b.valid&&a.down==b.down&&a.x==b.x&&a.y==b.y&&a.stamp==b.stamp;};
  if(queue.count>=2){
   auto& previous=queue.data[(queue.head+queue.count-2)%16];auto& latest=queue.data[(queue.head+queue.count-1)%16];
   if(same(previous,sample)&&same(latest,sample)){latest.now=now;return;}
  }
  if(!queue.push(sample)){++dropped;queue.clear();overflow=true;}
 }
 coopview::Control dispatch(Session& session,uint32_t now){
  if(overflow){queue.clear();overflow=false;cancel();if(session.state()==SessionState::Playing)session.training().dispatchSamples(now);return coopview::Control::None;}
  Sample s{};while(queue.pop(s)){
   if(!s.valid||s.stamp!=fingerprint(session)){if(s.stamp!=fingerprint(session))++stale;cancel();continue;}
   auto hit=coopview::hitTest(coopview::model(session,true),s.x,s.y);
   if(s.down){
    releasing=false;if(!down){if(!armed)continue;down=true;armed=false;pressed=hit;pressedStamp=s.stamp;}
    else if(hit!=pressed)pressed=coopview::Control::None;
    continue;
   }
   if(!releasing){releaseAt=s.now;releasing=true;}
   if(uint32_t(s.now-releaseAt)<ContactSampler::kReleaseMs)continue;
   if(down){down=false;auto result=pressed;pressed=coopview::Control::None;armed=true;
    if(result!=coopview::Control::None&&pressedStamp==s.stamp){queue.clear();cancel();return result;}
   }else armed=true;
  }
  if(session.state()==SessionState::Playing)session.training().dispatchSamples(now);
  return coopview::Control::None;
 }
};
}
