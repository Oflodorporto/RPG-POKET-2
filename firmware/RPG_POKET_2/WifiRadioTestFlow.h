#pragma once
#include "WifiRadioTestModel.h"
namespace wifi_radio_test {
class Flow {
 public:
  void setHooks(const Hooks& hooks){hooks_=hooks;}
  void setRadioConfig(uint8_t channel,uint8_t protocol,uint8_t powerSave){if(!dirty_&&!inRound()){channel_=channel;protocol_=protocol;powerSave_=powerSave;}}
  bool begin(const uint8_t* localMac,uint32_t nonce,PowerResult baseline,int8_t ceilingRaw,uint32_t now);
  void tick(uint32_t now);
  bool receive(const uint8_t* mac,const uint8_t* bytes,size_t size,int8_t rssi,bool rssiValid,uint32_t timestamp);
  bool invitePeer(const uint8_t* mac,uint32_t now);
  bool acceptInvite(uint32_t now);
  void rejectInvite(uint32_t now);
  bool continueAtDistance(Distance distance,uint32_t now);
  bool armManual(Distance distance,Level level,uint32_t now);
  void cancel(uint32_t now);
  void deviceFailure(Reason reason,PowerResult observed,uint32_t now);
  void noteQueueOverflow();
  void onMacSend(uint32_t run,uint8_t sequence,bool success,uint32_t now);
  void onMacSend(bool success,uint32_t now){onMacSendCounts(success?1:0,success?0:1,now);}
  void onMacSendCounts(uint32_t okay,uint32_t failed,uint32_t now);
  void acknowledgeCleanup(bool restored){if(restored&&state_==State::Fatal){dirty_=false;state_=State::Cancelled;}}
  State state()const{return state_;}
  Reason reason()const{return reason_;}
  Distance selectedDistance()const{return distance_;}
  Level selectedLevel()const{return level_;}
  bool pairedIdentity()const{return paired_;}
  const uint8_t* peerMac()const{return selected_;}
  const uint8_t* inviterMac()const{return state_==State::IncomingInvite?selected_:nullptr;}
  const uint8_t* localMac()const{return local_;}
  uint32_t session()const{return session_;}
  bool powerArmed()const{return dirty_;}
  size_t peerCount()const{return peerCount_;}
  const Peer* peerAt(size_t index)const{return index<peerCount_?&peers_[index]:nullptr;}
  const Report& current()const{return current_;}
  size_t reportCount()const{return reportCount_;}
  const Report* reportAt(size_t index)const{return index<reportCount_?&reports_[(reportHead_+index)%kMaxReports]:nullptr;}
 private:
  Hooks hooks_{}; State state_=State::Closed;Reason reason_=Reason::None;
  uint8_t local_[6]={},selected_[6]={};uint32_t nonce_=0,peerNonce_=0,session_=0;
  PowerResult baseline_{};int8_t ceiling_=0,remoteBaseline_=0;uint8_t channel_=6,protocol_=7,powerSave_=0;Peer peers_[kMaxPeers]{};size_t peerCount_=0;
  Report reports_[kMaxReports]{};size_t reportHead_=0,reportCount_=0;Report current_{};
  Distance distance_=Distance::PairingNear;Level level_=Level::Baseline;
  bool paired_=false,leader_=false,dirty_=false,manual_=false,haveCurrent_=false,recorded_=false;
  bool remoteReady_=false,remoteAck_=false,remoteSawRestore_=false,remoteCountsComplete_=false;
  bool invitedAccepted_=false,clockValid_=false,preflightRecorded_=false;
  uint32_t observed_=0,stateAt_=0,lastSend_=0,lastBeacon_=0,tokenCounter_=0,token_=0,remoteToken_=0,remoteFloor_=0;
  bool timingInvalid_=false;uint8_t nextStartSeq_=0,firstStartSeq_=0;uint8_t nextProbe_=0;uint32_t probeSentAt_[kProbeCount]={};uint32_t acceptedMask_=0,macMask_=0;
  uint32_t fresh(uint32_t supplied){uint32_t n=hooks_.nowMs?hooks_.nowMs(hooks_.context):supplied;
 if(timeAtOrAfter(supplied,n))n=supplied;
 if(clockValid_&&!timeAtOrAfter(n,observed_))n=observed_;
 observed_=n;clockValid_=true;return n;}
  bool inRound()const{return state_==State::Armed||state_==State::Running||state_==State::Preliminary||state_==State::Finalizing;}
  bool testedPower()const{return state_==State::Armed||state_==State::Running||state_==State::Preliminary;}
  bool settingsMatch(const WireFrame&f)const;
  bool frameForRun(const WireFrame&f)const{return f.run&&f.run==current_.run&&f.token==remoteToken_&&f.ackToken==token_&&f.stage==current_.stage&&f.distance==distance_&&f.level==level_;}
  WireFrame frame(Type type)const;
  int32_t send(Type type,uint32_t now,uint8_t seq=0,const uint8_t* destination=nullptr);
  void enter(State s,uint32_t now){state_=s;stateAt_=now;lastSend_=now-500;}
  void setReason(Reason r){reason_=r;
 if(haveCurrent_)current_.reason=r;}
  bool arm(Distance d,Level l,bool manual,uint32_t now);
  void start(uint32_t now);
  void preliminary(uint32_t now,Reason reason);
  void restore(uint32_t now);
  void finish(uint32_t now,State destination);
  void remember();
  void preflight(Reason reason,uint32_t now);
  bool remoteRestored()const{return current_.remoteFinal&&current_.remoteRestoreAttempted&&powerOkay(current_.remoteRestore)&&current_.remoteRestore.requestedRaw==remoteBaseline_&&current_.remoteRestore.actualRaw==remoteBaseline_;}
  void ingestSummary(const WireFrame&f,bool final);
  void observePeer(const uint8_t*mac,const WireFrame&f,int8_t rssi,bool valid,uint32_t now);
};
inline bool Flow::begin(const uint8_t* mac,uint32_t nonce,PowerResult baseline,int8_t ceiling,uint32_t now){
 if(state_==State::Fatal||dirty_||!validMac(mac)||!nonce||!powerOkay(baseline)||!validPower(baseline)||ceiling<8||ceiling>80||channel_<1||channel_>14||!protocol_||powerSave_>2)return false;
 memcpy(local_,mac,6);memset(selected_,0,6);nonce_=nonce;baseline_=baseline;ceiling_=ceiling;peerCount_=0;
 paired_=false;remoteBaseline_=0;preflightRecorded_=false;session_=0;peerNonce_=0;haveCurrent_=false;recorded_=false;remoteFloor_=0;reason_=Reason::None;
 current_=Report{};current_.localPower=baseline_;current_.local.clockDomain=nonce_;current_.channel=channel_;current_.protocol=protocol_;current_.powerSave=powerSave_;current_.localBaselineRaw=baseline_.actualRaw;memcpy(current_.localMac,local_,6);
 distance_=Distance::PairingNear;level_=Level::Baseline;clockValid_=false;now=fresh(now);lastBeacon_=now-700;enter(State::Browsing,now);return true;
}
inline WireFrame Flow::frame(Type type)const{
 WireFrame f;f.type=type;f.session=session_;f.run=haveCurrent_?current_.run:0;f.token=token_;f.ackToken=remoteToken_;f.nonce=nonce_;f.stage=haveCurrent_?current_.stage:0;f.distance=distance_;f.level=level_;f.ceilingRaw=ceiling_;f.baselineRaw=baseline_.actualRaw;f.channel=channel_;f.protocol=protocol_;f.powerSave=powerSave_;
 f.power=haveCurrent_?current_.localPower:baseline_;f.restore=current_.localRestore;f.stats=current_.local;
 if(current_.localStarted)f.flags|=Started;
 if(paired_)f.flags|=Paired;
 if(current_.localRestoreAttempted)f.flags|=RestoreAttempted;
 if(remoteRestored())f.flags|=SeenRemoteRestore;
 if(current_.queueOverflow)f.flags|=Overflow;
 if(current_.configMismatch)f.flags|=Mismatch;
 if(current_.local.timingValid)f.flags|=CountsComplete;
 if(current_.telemetryCorrelated)f.flags|=TelemetryCorrelated;
 if(manual_)f.flags|=Manual;
 if(timingInvalid_)f.flags|=TimingInvalid;
 return f;
}
inline int32_t Flow::send(Type type,uint32_t now,uint8_t seq,const uint8_t*destination){
 static const uint8_t broadcast[6]={255,255,255,255,255,255};WireFrame f=frame(type);if(type==Type::Start){seq=nextStartSeq_++;if(seq>=kProbeCount)return -1;probeSentAt_[seq]=now;}f.seq=seq;uint8_t bytes[kMaxFrame];size_t n=encode(f,bytes,sizeof(bytes));
 lastSend_=now;int32_t error=hooks_.send&&n?hooks_.send(hooks_.context,destination?destination:(type==Type::Beacon?broadcast:selected_),bytes,n):-1;
 if(inRound()&&type!=Type::Probe){Stats&s=current_.local;if(type==Type::Echo){++s.echoTxAttempts;if(!error)++s.echoApiAccepted;else ++s.echoApiErrors;}else{++s.controlTxAttempts;if(!error)++s.controlApiAccepted;else ++s.controlApiErrors;}}
 return error;
}
inline void Flow::observePeer(const uint8_t*mac,const WireFrame&f,int8_t rssi,bool valid,uint32_t now){
 if(!f.nonce||!validMac(mac)||sameMac(mac,local_))return;
 size_t i=0;for(;i<peerCount_;++i)if(sameMac(mac,peers_[i].mac))break;
 if(i==peerCount_){if(peerCount_==kMaxPeers)return;++peerCount_;}Peer&p=peers_[i];memcpy(p.mac,mac,6);p.nonce=f.nonce;p.lastSeen=now;p.rssi=rssi;p.rssiValid=valid;
}
inline bool Flow::invitePeer(const uint8_t*mac,uint32_t now){
 now=fresh(now);
 if(state_!=State::Browsing||!validMac(mac))return false;
 const Peer*p=nullptr;for(size_t i=0;i<peerCount_;++i)if(sameMac(mac,peers_[i].mac)){p=&peers_[i];break;}if(!p)return false;
 reason_=Reason::None;preflightRecorded_=false;memcpy(selected_,mac,6);peerNonce_=p->nonce;leader_=memcmp(local_,selected_,6)<0;session_=sessionId(local_,nonce_,selected_,peerNonce_);invitedAccepted_=false;enter(State::Inviting,now);send(Type::Invite,now);return true;
}
inline bool Flow::acceptInvite(uint32_t now){now=fresh(now);
 if(state_!=State::IncomingInvite)return false;
 reason_=Reason::None;preflightRecorded_=false;invitedAccepted_=true;enter(State::Pairing,now);send(Type::Accept,now);return true;}
inline void Flow::rejectInvite(uint32_t now){now=fresh(now);
 if(state_!=State::IncomingInvite)return;
 send(Type::Reject,now);memset(selected_,0,6);session_=0;enter(State::Browsing,now);}
inline bool Flow::continueAtDistance(Distance d,uint32_t now){
 now=fresh(now);
 if(state_!=State::DistancePause||d!=distance_)return false;
 return arm(d,Level::Baseline,false,now);
}
inline bool Flow::armManual(Distance d,Level l,uint32_t now){
 now=fresh(now);
 if(!paired_||!(state_==State::ManualPause||state_==State::DistancePause||state_==State::Complete)||!validSetting(d,l)||l==Level::Baseline)return false;
 return arm(d,l,true,now);
}
inline bool Flow::arm(Distance d,Level l,bool manual,uint32_t now){
 if(dirty_||state_==State::Fatal||!validSetting(d,l))return false;
 distance_=d;level_=l;manual_=manual;token_=++tokenCounter_;
 if(!token_)token_=++tokenCounter_;
 remoteToken_=0;remoteReady_=remoteAck_=remoteSawRestore_=remoteCountsComplete_=false;
 haveCurrent_=true;recorded_=false;current_=Report{};current_.session=session_;current_.stage=stageId(d,l);current_.distance=d;current_.level=l;current_.manual=manual;current_.armedAt=now;current_.channel=channel_;current_.protocol=protocol_;current_.powerSave=powerSave_;current_.localBaselineRaw=baseline_.actualRaw;current_.remoteBaselineRaw=remoteBaseline_;current_.local.clockDomain=nonce_;memcpy(current_.localMac,local_,6);memcpy(current_.peerMac,selected_,6);
 reason_=Reason::None;timingInvalid_=false;nextStartSeq_=0;firstStartSeq_=0;nextProbe_=0;acceptedMask_=macMask_=0;memset(probeSentAt_,0,sizeof(probeSentAt_));
 // Arm the independently enforced deadline before touching the radio. Even a failed
 // setter may have changed the driver, so restoration remains mandatory.
 dirty_=true;enter(State::Armed,now);int8_t requested=requestedPower(l,baseline_.actualRaw);
 if(hooks_.applyPower)current_.localPower=hooks_.applyPower(hooks_.context,requested);else current_.localPower.error=-1;
 current_.localPower.requestedRaw=requested;now=fresh(now);
 if(!powerOkay(current_.localPower)||!validPower(current_.localPower)||(l==Level::Baseline?current_.localPower.actualRaw!=baseline_.actualRaw:current_.localPower.actualRaw>ceiling_)||current_.localPower.actualRaw>80){setReason(Reason::PowerFailure);restore(now);return false;}
 if(elapsed(now,current_.armedAt,kArmedLimitMs)){setReason(Reason::HandshakeTimeout);restore(now);return false;}
 send(Type::Ready,now);return true;
}
inline bool Flow::settingsMatch(const WireFrame&f)const{
 return f.distance==distance_&&f.level==level_&&f.stage==current_.stage&&bool(f.flags&Manual)==manual_&&f.ceilingRaw==ceiling_&&f.channel==channel_&&f.protocol==protocol_&&f.powerSave==powerSave_&&powerOkay(f.power)&&f.power.requestedRaw==current_.localPower.requestedRaw&&f.power.actualRaw==current_.localPower.actualRaw;
}
inline void Flow::start(uint32_t now){
 if(state_!=State::Armed)return;
 current_.paired=true;current_.localStarted=true;current_.local.startedAt=now;enter(State::Running,now);
}
inline void Flow::preliminary(uint32_t now,Reason reason){
 if(state_==State::Preliminary||state_==State::Finalizing)return;
 if(reason!=Reason::None)setReason(reason);
 current_.local.endedAt=now;
 current_.local.timingValid=current_.localStarted&&!timingInvalid_&&nextProbe_==kProbeCount&&elapsed(now,current_.local.startedAt,kRoundMs)&&!elapsed(now,current_.local.startedAt,kRoundMs+201);
 if(current_.localStarted&&elapsed(now,current_.local.startedAt,kRoundMs+201)){timingInvalid_=true;setReason(Reason::TimingInvalid);}
 enter(State::Preliminary,now);send(Type::Preliminary,now);
}
inline void Flow::restore(uint32_t now){
 if(!dirty_)return;
 current_.localRestoreAttempted=true;
 if(hooks_.restorePower)current_.localRestore=hooks_.restorePower(hooks_.context);else current_.localRestore.error=-1;
 now=fresh(now);current_.localRestore.requestedRaw=baseline_.actualRaw;
 if(!powerOkay(current_.localRestore)||current_.localRestore.actualRaw!=baseline_.actualRaw){
  // Keep the initiating device error in the report. The independent restoration
  // error and Fatal state still block every arm until verified full cleanup.
  if(current_.reason==Reason::ConfigMismatch||current_.reason==Reason::PowerFailure)reason_=Reason::RestoreFailure;else setReason(Reason::RestoreFailure);
  current_.finishedAt=now;remember();enter(State::Fatal,now);return;
 }
 dirty_=false;enter(State::Finalizing,now);send(Type::Final,now);
}
inline void Flow::preflight(Reason reason,uint32_t now){
 if(preflightRecorded_)return;
 current_=Report{};current_.distance=Distance::PairingNear;current_.localPower=baseline_;current_.local.clockDomain=nonce_;current_.armedAt=stateAt_;current_.finishedAt=now;current_.reason=reason;current_.channel=channel_;current_.protocol=protocol_;current_.powerSave=powerSave_;memcpy(current_.localMac,local_,6);memcpy(current_.peerMac,selected_,6);haveCurrent_=true;recorded_=false;remember();preflightRecorded_=true;haveCurrent_=false;
}
inline void Flow::remember(){
 if(!haveCurrent_||recorded_)return;
 size_t i=(reportHead_+reportCount_)%kMaxReports;
 if(reportCount_==kMaxReports){i=reportHead_;reportHead_=(reportHead_+1)%kMaxReports;}else ++reportCount_;
 reports_[i]=current_;recorded_=true;
}
inline void Flow::finish(uint32_t now,State destination){
 current_.finishedAt=now;current_.complete=current_.local.timingValid&&remoteCountsComplete_;
 current_.remoteRestoreConfirmed=remoteRestored()&&remoteSawRestore_;
 if(remoteToken_&&(!remoteFloor_||int32_t(remoteToken_-remoteFloor_)>0))remoteFloor_=remoteToken_;
 remember();enter(destination,now);
}
inline void Flow::ingestSummary(const WireFrame&f,bool final){
 // A reordered preliminary snapshot must never overwrite a final snapshot.
 if(current_.remoteFinal&&!final)return;
 current_.remote=f.stats;current_.remotePower=f.power;current_.remotePresent=true;current_.remoteStarted=bool(f.flags&Started);
 current_.queueOverflow=current_.queueOverflow||bool(f.flags&Overflow);current_.configMismatch=current_.configMismatch||bool(f.flags&Mismatch);
 current_.telemetryCorrelated=current_.telemetryCorrelated&&bool(f.flags&TelemetryCorrelated);remoteCountsComplete_=bool(f.flags&CountsComplete)&&!(f.flags&TimingInvalid);
 if(final){current_.remoteFinal=true;current_.remoteRestore=f.restore;current_.remoteRestoreAttempted=bool(f.flags&RestoreAttempted);remoteSawRestore_=remoteSawRestore_||bool(f.flags&SeenRemoteRestore);}
 current_.complete=current_.local.timingValid&&remoteCountsComplete_;
 current_.remoteRestoreConfirmed=remoteRestored()&&remoteSawRestore_;
}
inline void Flow::noteQueueOverflow(){if(inRound()){current_.queueOverflow=true;setReason(Reason::QueueOverflow);}}
inline void Flow::onMacSendCounts(uint32_t okay,uint32_t failed,uint32_t now){
 now=fresh(now);(void)now;
 if(!inRound())return;
 current_.telemetryCorrelated=false;current_.local.macTelemetryValid=true;
 uint32_t a=uint32_t(current_.local.macSuccess)+(okay>65535?65535:okay),b=uint32_t(current_.local.macErrors)+(failed>65535?65535:failed);
 current_.local.macSuccess=uint16_t(a>65535?65535:a);current_.local.macErrors=uint16_t(b>65535?65535:b);
}
inline void Flow::onMacSend(uint32_t run,uint8_t sequence,bool success,uint32_t now){
 now=fresh(now);(void)now;
 if(!inRound()||run!=current_.run||sequence>=kProbeCount)return;
 uint32_t bit=1u<<sequence;
 if(!(acceptedMask_&bit)||(macMask_&bit))return;
 macMask_|=bit;current_.local.macTelemetryValid=true;
 if(success)++current_.local.macSuccess;else ++current_.local.macErrors;
}
inline void Flow::deviceFailure(Reason reason,PowerResult observed,uint32_t now){
 now=fresh(now);
 if(state_==State::Fatal)return;
 if(state_==State::Closed){reason_=reason;return;}
 if(!inRound()){
  current_=Report{};current_.distance=Distance::PairingNear;current_.local.clockDomain=nonce_;current_.armedAt=now;current_.channel=channel_;current_.protocol=protocol_;current_.powerSave=powerSave_;current_.localBaselineRaw=baseline_.actualRaw;memcpy(current_.localMac,local_,6);memcpy(current_.peerMac,selected_,6);haveCurrent_=true;recorded_=false;observed.requestedRaw=baseline_.actualRaw;
 }else{
  observed.requestedRaw=current_.localPower.requestedRaw;
  send(Type::Cancel,now);
 }
 current_.localPower=observed;current_.local.endedAt=now;current_.local.timingValid=false;current_.complete=false;
 if(reason==Reason::ConfigMismatch)current_.configMismatch=true;
 setReason(reason);dirty_=true;restore(fresh(now));
 if(state_!=State::Fatal)finish(fresh(now),State::Cancelled);
}
inline void Flow::cancel(uint32_t now){
 now=fresh(now);
 if(state_==State::Fatal)return;
 if(inRound()){setReason(Reason::Cancelled);send(Type::Cancel,now);current_.local.endedAt=now;current_.local.timingValid=false;
 if(dirty_)restore(now);
 if(state_!=State::Fatal)finish(fresh(now),State::Cancelled);}
 else enter(State::Cancelled,now);
}
inline void Flow::tick(uint32_t supplied){
 uint32_t now=fresh(supplied);
 // Restoration is first and never depends on receiving a packet, matching a peer,
 // or reaching the next transmit slot. All comparisons tolerate uint32 wrap.
 if(testedPower()&&elapsed(now,current_.armedAt,kArmedLimitMs)){if(state_!=State::Preliminary)preliminary(now,Reason::RoundTimeout);restore(fresh(now));
 return;}
 if(state_==State::Browsing){if(elapsed(now,lastBeacon_,700)){lastBeacon_=now;send(Type::Beacon,now);}if(elapsed(now,stateAt_,30000)){reason_=Reason::PairTimeout;preflight(Reason::PairTimeout,now);}
 return;}
 if(state_==State::Inviting||state_==State::Pairing||state_==State::IncomingInvite){
  if(elapsed(now,stateAt_,10000)){setReason(Reason::PairTimeout);preflight(Reason::PairTimeout,now);session_=0;memset(selected_,0,6);enter(State::Browsing,now);
 return;}
  if(state_!=State::IncomingInvite&&elapsed(now,lastSend_,500))send(state_==State::Inviting?Type::Invite:Type::Accept,now);
 return;
 }
 if(state_==State::Armed){if(elapsed(now,stateAt_,kHandshakeMs)){preliminary(now,Reason::HandshakeTimeout);restore(fresh(now));
 return;}
  if(elapsed(now,lastSend_,300))send(leader_&&remoteReady_&&remoteAck_?Type::Start:Type::Ready,now);
 return;
 }
 if(state_==State::Running){
  if(nextProbe_<kProbeCount&&elapsed(now,current_.local.startedAt,uint32_t(nextProbe_+1)*kProbeCadenceMs)){
   if(elapsed(now,current_.local.startedAt,uint32_t(nextProbe_+1)*kProbeCadenceMs+201)){timingInvalid_=true;preliminary(now,Reason::TimingInvalid);restore(fresh(now));return;}
   uint8_t seq=nextProbe_++;++current_.local.txAttempts;probeSentAt_[seq]=now;int32_t e=send(Type::Probe,now,seq);
   if(e==0){++current_.local.apiAccepted;acceptedMask_|=1u<<seq;}else ++current_.local.apiErrors;
   now=fresh(now);
  }
  if(elapsed(now,current_.local.startedAt,kRoundMs))preliminary(now,Reason::None);
 return;
 }
 if(state_==State::Preliminary){if(elapsed(now,stateAt_,kPreliminaryMs)){restore(now);
 return;}if(elapsed(now,lastSend_,200))send(Type::Preliminary,now);
 return;}
 if(state_==State::Finalizing){
  if(elapsed(now,stateAt_,kFinalMs)){
   bool okay=remoteRestored()&&remoteSawRestore_&&comparable(current_)&&lossPermille(current_,true)>=0&&lossPermille(current_,false)>=0;
   if(!okay){if(reason_==Reason::None)setReason(Reason::MissingRemote);finish(now,State::ManualPause);
 return;}
   finish(now,manual_?State::ManualPause:State::DistancePause);
   if(manual_)return;
   if(level_!=Level::N4){Level next=Level(uint8_t(level_)+1);arm(distance_,next,false,now);}
   else if(distance_!=Distance::TwoMeters){distance_=Distance(uint8_t(distance_)+1);level_=Level::Baseline;}
   else enter(State::Complete,now);
   return;
  }
  if(elapsed(now,lastSend_,200))send(Type::Final,now);
 return;
 }
}
inline bool Flow::receive(const uint8_t*mac,const uint8_t*bytes,size_t size,int8_t rssi,bool rssiValid,uint32_t timestamp){
 WireFrame f;
 if(!validMac(mac)||sameMac(mac,local_)||!decode(bytes,size,f,false))return false;
 if(!decode(bytes,size,f,true)){
  if(inRound()&&sameMac(mac,selected_)&&f.session==session_&&f.nonce==peerNonce_&&frameForRun(f)){current_.configMismatch=true;setReason(Reason::ConfigMismatch);}
  return false;
 }
 uint32_t now=fresh(timestamp);
 // Deadlines also precede packet handling. A packet at the deadline cannot extend
 // the arm or use expired power. tick is safe for a stale outer-loop timestamp.
 if(testedPower()&&elapsed(now,current_.armedAt,kArmedLimitMs)){preliminary(now,Reason::RoundTimeout);restore(fresh(now));return false;}
 if(state_==State::Armed&&elapsed(now,stateAt_,kHandshakeMs)){preliminary(now,Reason::HandshakeTimeout);restore(fresh(now));return false;}
 if(f.type==Type::Beacon){if(state_==State::Browsing)observePeer(mac,f,rssi,rssiValid,now);return state_==State::Browsing;}
 if(f.type==Type::Invite&&state_==State::Browsing){
  if(!f.nonce||!powerOkay(f.power)||f.baselineRaw<8||f.baselineRaw>80||f.power.actualRaw!=f.baselineRaw||f.session!=sessionId(local_,nonce_,mac,f.nonce))return false;
  observePeer(mac,f,rssi,rssiValid,now);memcpy(selected_,mac,6);peerNonce_=f.nonce;remoteBaseline_=f.baselineRaw;leader_=memcmp(local_,selected_,6)<0;session_=f.session;enter(State::IncomingInvite,now);return true;
 }
 if(!sameMac(mac,selected_)||f.session!=session_||f.nonce!=peerNonce_||f.baselineRaw<8||f.baselineRaw>80||(remoteBaseline_&&f.baselineRaw!=remoteBaseline_))return false;
 if(f.type==Type::Reject&&state_==State::Inviting){setReason(Reason::PeerRejected);enter(State::Browsing,now);return true;}
 if(f.type==Type::Accept&&(state_==State::Inviting||state_==State::DistancePause)){
  if(!paired_){remoteBaseline_=f.baselineRaw;paired_=true;distance_=Distance::TwoPalms;level_=Level::Baseline;enter(State::DistancePause,now);}send(Type::Paired,now);return true;
 }
 if(f.type==Type::Paired&&state_==State::Pairing&&invitedAccepted_){paired_=true;distance_=Distance::TwoPalms;level_=Level::Baseline;enter(State::DistancePause,now);return true;}
 if(!paired_||!inRound())return false;
 if(f.type==Type::Ready&&state_==State::Armed){
  if(!f.token||(remoteFloor_&&int32_t(f.token-remoteFloor_)<=0))return false;
  if(!settingsMatch(f)){current_.configMismatch=true;setReason(Reason::ConfigMismatch);return false;}
  if(remoteToken_&&remoteToken_!=f.token)return false;
  remoteToken_=f.token;remoteReady_=true;remoteAck_=f.ackToken==token_;current_.remotePower=f.power;
  current_.run=leader_?runId(session_,token_,remoteToken_,current_.stage):runId(session_,remoteToken_,token_,current_.stage);
  // Avoid an unbounded Ready ping-pong: only send on adoption/ack advancement;
  // periodic tick retries any dropped handshake packet.
  return true;
 }
 if(!frameForRun(f))return false;
 if(!settingsMatch(f)){current_.configMismatch=true;setReason(Reason::ConfigMismatch);return false;}
 if(f.type==Type::Start&&!leader_&&(state_==State::Armed||state_==State::Running)){
  if(!remoteReady_||f.ackToken!=token_)return false;
 if(state_==State::Armed){firstStartSeq_=f.seq;start(now);}else if(elapsed(now,current_.local.startedAt,201)){timingInvalid_=true;setReason(Reason::TimingInvalid);}
  send(Type::StartAck,now,firstStartSeq_);return true;
 }
 if(f.type==Type::StartAck&&leader_&&(state_==State::Armed||state_==State::Running)){
  if(state_==State::Running)return true;
  if(f.seq>=nextStartSeq_||elapsed(now,probeSentAt_[f.seq],201)||!timeAtOrAfter(now,probeSentAt_[f.seq])){timingInvalid_=true;setReason(Reason::TimingInvalid);send(Type::Cancel,now);preliminary(now,Reason::TimingInvalid);restore(fresh(now));return false;}
  start(now);return true;
 }
 if(f.type==Type::Cancel){preliminary(now,Reason::Cancelled);restore(fresh(now));return true;}
 if((f.type==Type::Preliminary||f.type==Type::Final)&&f.stats.clockDomain!=peerNonce_){current_.configMismatch=true;setReason(Reason::ConfigMismatch);return false;}
 if(f.type==Type::Preliminary||f.type==Type::Final){ingestSummary(f,f.type==Type::Final);return true;}
 if((f.type==Type::Probe||f.type==Type::Echo)&&state_!=State::Running)return false;
 if((f.type==Type::Probe||f.type==Type::Echo)&&(!timeAtOrAfter(timestamp,current_.local.startedAt)||elapsed(now,timestamp,201))){timingInvalid_=true;setReason(Reason::TimingInvalid);return false;}
 if(f.type==Type::Probe){
  uint32_t bit=1u<<f.seq;
 if(!(current_.local.rxMask&bit)){
   current_.local.rxMask|=bit;++current_.local.uniqueRx;
   if(rssiValid){Stats&s=current_.local;
 if(!s.rssiValid){s.rssiMin=s.rssiMax=rssi;}else{if(rssi<s.rssiMin)s.rssiMin=rssi;
 if(rssi>s.rssiMax)s.rssiMax=rssi;}s.rssiValid=true;s.rssiLast=rssi;s.rssiAt=timestamp;}
  }
  send(Type::Echo,now,f.seq);return true;
 }
 if(f.type==Type::Echo){
  uint32_t bit=1u<<f.seq;
 if(!(acceptedMask_&bit)||(current_.local.echoMask&bit)||!timeAtOrAfter(timestamp,probeSentAt_[f.seq]))return false;
  uint32_t rtt=timestamp-probeSentAt_[f.seq];
 if(rtt>kRoundMs)return false;
 Stats&s=current_.local;s.echoMask|=bit;
 if(!s.echoes||rtt<s.rttMinMs)s.rttMinMs=rtt;
 if(rtt>s.rttMaxMs)s.rttMaxMs=rtt;
 ++s.echoes;s.rttSumMs+=rtt;return true;
 }
 return false;
}
} // namespace wifi_radio_test
