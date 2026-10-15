#pragma once
#include "CoopAdmission.h"
namespace coopplatform {
enum class SessionState:uint8_t {Closed,Browsing,Hosting,AwaitingAccept,Joining,Starting,Playing,Ended};
enum class SessionEnd:uint8_t {None,Left,PeerLeft,JoinTimeout,LinkLost,Won,Lost,Profile,Protocol,InviteExpired};
enum class SessionNotice:uint8_t {None,Declined,Busy,PeerExpired,SendFailed,InviteExpired};
struct Peer {
 bool valid=false,available=false;Mac mac{};coop::Actor profile{};
 uint64_t sessionToken=0;uint32_t lastSeen=0;
};
// Loop-thread only. The sender must synchronously copy the payload; it must not
// re-enter Session. Radio callbacks enqueue copied source MAC + bytes elsewhere.
// This admission protocol has replay/collision checks, not cryptographic peer
// authentication. ESP-NOW source identity is supplied by the transport adapter.
class Session {
public:
 using Sender=bool (*)(void*,const Mac&,const uint8_t*,size_t);
 static constexpr size_t kMaxPeers=4;
 static constexpr uint32_t kBeaconMs=700,kPeerExpiryMs=2800,kRetryMs=250;
 static constexpr uint32_t kAcceptTimeoutMs=30000;
 static constexpr uint32_t kAdmissionTimeoutMs=12000,kLinkTimeoutMs=10000,kTerminalDrainMs=12000;
 static constexpr uint32_t kEnemyPresentationMs=700;
 uint32_t malformed=0,rejected=0,sendDrops=0;
 void setSender(Sender fn,void* context){send=fn;sendContext=context;}
 SessionState state()const{return phase;}
 SessionEnd endReason()const{return ending;}
 SessionNotice notice()const{return note;}
 bool isHost()const{return hosting;}
 const Peer* peer(size_t index)const{return index<kMaxPeers&&peers[index].valid?&peers[index]:nullptr;}
 const Peer* pendingPeer()const{return remote.valid?&remote:nullptr;}
 const coop::Actor& localProfile()const{return local;}
 Training& training(){return combat;}
 const Training& training()const{return combat;}
 uint64_t sessionToken()const{return session;}
 uint64_t joinToken()const{return joinId;}
 uint32_t observedTime()const{return observedAt;}
 bool invitationConfirmed()const{return phase==SessionState::Joining&&decisionConfirmed;}
 bool awaitingDecision()const{return phase==SessionState::AwaitingAccept||invitationConfirmed();}
 // Pure, saturating elapsed-time queries; uint32 subtraction supports wrap.
 // Guest time is a bounded estimate from the host, never a shared millis clock.
 uint32_t decisionRemainingMs(uint32_t now)const{
  if(phase==SessionState::AwaitingAccept)return remaining(kAcceptTimeoutMs,actionAt,now);
  if(invitationConfirmed())return remaining(decisionBudget,decisionAt,now);
  return 0;
 }
 bool open(const rpg::Game& game,const Mac& own,uint64_t entropy,uint32_t now){
  if(phase!=SessionState::Closed&&phase!=SessionState::Ended)return false;
  if(!usableMac(own)||!entropy||!trainingEligible(game))return false;
  coop::Actor profile{};if(!coop::playerProfile(game,0,profile)||!admittedProfile(profile,0))return false;
  // A monotonic, bijectively mixed stream cannot repeat in this Session object's
  // lifetime, even when test entropy/time repeat. Fresh boot entropy is required.
  if(!keySet){key=entropy;for(uint8_t b:own.bytes)key=mix(key^b);keySet=true;}
  if(serial>=UINT64_MAX-16)return false;
  combat.stop();mac=own;local=profile;remote={};for(auto& p:peers)p={};
  phase=SessionState::Browsing;ending=SessionEnd::None;note=SessionNotice::None;hosting=false;
  session=joinId=0;seed=expectedInitialHash=0;openedEntropy=entropy;
  actionAt=lastSeen=lastRetry=lastDiscovery=observedAt=now;decisionConfirmed=false;decisionAt=decisionBudget=0;terminalAt=0;stallActive=false;stallAt=stallRevision=0;presentationSeen=presentationConfirmed=false;presentationAt=presentationRevision=0;sendProbe();return true;
 }
 bool host(uint32_t now){
  if(phase!=SessionState::Browsing)return false;
  session=mint();if(!session)return false;
  hosting=true;phase=SessionState::Hosting;note=SessionNotice::None;
  actionAt=lastDiscovery=observedAt=now;sendBeacon();return true;
 }
 bool join(size_t index,uint32_t now){
  if(phase!=SessionState::Browsing||index>=kMaxPeers)return false;
  const Peer& p=peers[index];
  if(!p.valid||!p.available||uint32_t(now-p.lastSeen)>=kPeerExpiryMs){note=SessionNotice::PeerExpired;return false;}
  uint64_t fresh=mint();if(!fresh)return false;
  hosting=false;remote=p;session=p.sessionToken;joinId=fresh;local.id=2;local.owner=1;
  phase=SessionState::Joining;note=SessionNotice::None;decisionConfirmed=false;decisionAt=decisionBudget=0;actionAt=lastRetry=lastSeen=observedAt=now;sendJoin();return true;
 }
 bool accept(uint32_t now){
  expire(now);if(phase!=SessionState::AwaitingAccept||!remote.valid)return false;
  seed=uint32_t(mix(openedEntropy^session^joinId^uint64_t(now)));if(!seed)seed=1;
  coop::State initial{};if(!admittedState(initial,local,remote.profile,seed)){end(SessionEnd::Profile,now);return false;}
  expectedInitialHash=coop::stateHash(initial);phase=SessionState::Starting;
  actionAt=lastRetry=lastSeen=now;note=SessionNotice::None;sendStart();return true;
 }
 bool decline(uint32_t now){
  expire(now);if(phase!=SessionState::AwaitingAccept)return false;
  sendControl(admission::Type::Decline);return resetLobby(now);
 }
 void leave(uint32_t now){
  observedAt=now;
  if(remote.valid&&session&&joinId)sendControl(admission::Type::Bye);
  end(SessionEnd::Left,now); // Explicit next open is required; packets cannot reopen.
 }
 bool receive(const Mac& source,const uint8_t* bytes,size_t length,uint32_t now){
  if(phase==SessionState::Closed||!usableMac(source)||sameMac(source,mac)||!bytes){++rejected;return false;}
  expire(now);
  if(length>=4){coop::Reader magic{bytes,length};if(magic.u32()==coop::kMagic)return receiveEngine(source,bytes,length,now);}
  admission::Header h{};
  if(!admission::decode(bytes,length,h)){++malformed;return false;}
  if(phase==SessionState::Ended){++rejected;return false;}
  coop::Reader r{bytes+admission::kHeaderBytes,h.payload};
  using T=admission::Type;
  if(h.type==T::Probe){
   if(phase==SessionState::Hosting||phase==SessionState::AwaitingAccept)sendBeacon();
   return true;
  }
  if(h.type==T::Beacon){
   uint8_t available=r.u8();coop::Actor profile=coop::readActor(r);
   if(available>1||!r.ok||!admittedProfile(profile,0)){++malformed;return false;}
   if(phase!=SessionState::Browsing){++rejected;return false;}
   size_t slot=kMaxPeers;
   for(size_t i=0;i<kMaxPeers;++i)if(peers[i].valid&&sameMac(peers[i].mac,source)){slot=i;break;}
   if(slot==kMaxPeers)for(size_t i=0;i<kMaxPeers;++i)if(!peers[i].valid||uint32_t(now-peers[i].lastSeen)>=kPeerExpiryMs){slot=i;break;}
   if(slot==kMaxPeers){++rejected;return false;}
   peers[slot]={true,available!=0,source,profile,h.session,now};return true;
  }
  if(h.type==T::Join){
   coop::Actor profile=coop::readActor(r);
   if(!r.ok||!admittedProfile(profile,1)){++malformed;return false;}
   if(!hosting||h.session!=session){++rejected;return false;}
   if(phase==SessionState::Hosting){
    remote={true,false,source,profile,h.session,now};joinId=h.join;
    phase=SessionState::AwaitingAccept;note=SessionNotice::None;actionAt=lastSeen=now;sendWaiting(now);sendBeacon();return true;
   }
   if(!remote.valid||!sameMac(source,remote.mac)||h.join!=joinId){
    sendDirect(source,T::Busy,h.session,h.join);++rejected;return false;
   }
   if(!admission::sameProfile(profile,remote.profile)){++rejected;return false;}
   // Idempotent JOIN retry never changes the candidate or extends the decision
   // deadline. Once accepted, retry START to repair a dropped admission packet.
   if(phase==SessionState::AwaitingAccept)sendWaiting(now);
   else if(phase==SessionState::Starting||phase==SessionState::Playing)sendStart();
   return true;
  }
  if(!remote.valid||!sameMac(source,remote.mac)||h.session!=session||h.join!=joinId){++rejected;return false;}
  if(h.type==T::Bye){end(SessionEnd::PeerLeft,now);return true;}
  if(h.type==T::Waiting){
   uint32_t budget=r.u32();
   if(hosting||phase!=SessionState::Joining||!r.ok||!budget||budget>kAcceptTimeoutMs){++rejected;return false;}
   // One initial estimate, then only reductions. Duplicated/reordered replies
   // cannot extend the decision window. No training starts before START.
   if(!decisionConfirmed){decisionConfirmed=true;decisionAt=now;decisionBudget=budget;}
   else if(budget<decisionRemainingMs(now)){decisionAt=now;decisionBudget=budget;}
   return true;
  }
  if(h.type==T::Expired){
   if(hosting||phase!=SessionState::Joining){++rejected;return false;}
   note=SessionNotice::InviteExpired;end(SessionEnd::InviteExpired,now);return true;
  }
  if(h.type==T::Decline||h.type==T::Busy){
   if(hosting||phase!=SessionState::Joining){++rejected;return false;}
   note=h.type==T::Decline?SessionNotice::Declined:SessionNotice::Busy;
   end(SessionEnd::JoinTimeout,now);return true;
  }
  if(h.type==T::Start){
   coop::Actor profile=coop::readActor(r);uint32_t startSeed=r.u32();
   if(hosting||!r.ok||!admittedProfile(profile,0)||!startSeed||!admission::sameProfile(profile,remote.profile)){
    ++malformed;return false;
   }
   if(phase==SessionState::Joining){
    coop::State initial{};if(!admittedState(initial,profile,local,startSeed)){++malformed;return false;}
    seed=startSeed;expectedInitialHash=coop::stateHash(initial);
    Binding binding{mac,remote.mac,session,1};
    if(combat.startAdmitted(binding,local,remote.profile,seed,now)!=StartError::None){end(SessionEnd::Profile,now);return false;}
    phase=SessionState::Starting;actionAt=lastSeen=lastRetry=now;sendReady();flush();return true;
   }
   if((phase!=SessionState::Starting&&phase!=SessionState::Playing)||startSeed!=seed){++rejected;return false;}
   sendReady(); // Same START repairs a lost READY; no new RNG/state/session.
   return true;
  }
  if(h.type==T::Ready){
   uint32_t hash=r.u32();
   if(!hosting||!r.ok||hash!=expectedInitialHash||(phase!=SessionState::Starting&&phase!=SessionState::Playing)){
    ++rejected;return false;
   }
   if(!combat.active()){
    Binding binding{mac,remote.mac,session,0};
    if(combat.startAdmitted(binding,local,remote.profile,seed,now)!=StartError::None){end(SessionEnd::Profile,now);return false;}
    lastSeen=now;flush();
   }
   return true;
  }
  ++rejected;return false;
 }
 void tick(uint32_t now){
  expire(now);
  if(phase==SessionState::Closed)return;
  if(phase==SessionState::Ended){
   // Keep final state visible and replay final ACK/snapshot long enough for the
   // other board to confirm. Never enter Playing again after a terminal result.
   if(combat.active()&&uint32_t(now-terminalAt)<kTerminalDrainMs){combat.tick(now);flush();}
   return;
  }
  if(phase==SessionState::Browsing){
   for(auto& p:peers)if(p.valid&&uint32_t(now-p.lastSeen)>=kPeerExpiryMs)p.valid=false;
   if(uint32_t(now-lastDiscovery)>=kBeaconMs){lastDiscovery=now;sendProbe();}
  }
  if(phase==SessionState::Hosting||phase==SessionState::AwaitingAccept){
   if(uint32_t(now-lastDiscovery)>=kBeaconMs){lastDiscovery=now;sendBeacon();}
  }
  if(uint32_t(now-lastRetry)>=kRetryMs){
   lastRetry=now;
   if(phase==SessionState::Joining)sendJoin();
   else if(phase==SessionState::Starting){if(hosting)sendStart();else sendReady();}
  }
  if(combat.active()){
   // UI can submit a local host command outside Session. Observe before ticking
   // so that its new revision cannot borrow an expired previous-frame delay.
   watchPresentation(now);
   const bool allowEnemy=!hosting||(presentationSeen&&presentationConfirmed&&uint32_t(now-presentationAt)>=kEnemyPresentationMs);
   combat.tick(now,allowEnemy);watchPresentation(now);watchStall(now);flush();refreshState(now);
  }
 }
private:
 SessionState phase=SessionState::Closed;SessionEnd ending=SessionEnd::None;SessionNotice note=SessionNotice::None;
 Sender send=nullptr;void* sendContext=nullptr;Training combat{};Peer peers[kMaxPeers]{},remote{};
 Mac mac{};coop::Actor local{};bool hosting=false,keySet=false;
 uint64_t key=0,serial=0,openedEntropy=0,session=0,joinId=0;
 uint32_t seed=0,expectedInitialHash=0,actionAt=0,lastSeen=0,lastRetry=0,lastDiscovery=0,terminalAt=0;
 bool decisionConfirmed=false;uint32_t decisionAt=0,decisionBudget=0,observedAt=0;
 static uint32_t remaining(uint32_t budget,uint32_t since,uint32_t now){const uint32_t elapsed=uint32_t(now-since);return elapsed>=budget?0:budget-elapsed;}
 bool stallActive=false;uint32_t stallAt=0,stallRevision=0;
 bool presentationSeen=false,presentationConfirmed=false;uint32_t presentationAt=0,presentationRevision=0;
 static uint64_t mix(uint64_t x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);}
 uint64_t mint(){if(serial>=UINT64_MAX-1)return 0;uint64_t n=mix(key+(++serial));if(!n&&serial<UINT64_MAX-1)n=mix(key+(++serial));return n;}
 bool transmit(const Mac& destination,const uint8_t* bytes,size_t size){
  bool ok=send&&send(sendContext,destination,bytes,size);
  if(!ok){++sendDrops;note=SessionNotice::SendFailed;}return ok;
 }
 static Mac broadcast(){Mac m{};for(uint8_t& b:m.bytes)b=255;return m;}
 void emit(const Mac& destination,admission::Packet& p,coop::Writer& w){if(admission::finish(p,w))transmit(destination,p.bytes,p.size);}
 void sendDirect(const Mac& destination,admission::Type type,uint64_t hostToken,uint64_t guestToken){
  admission::Packet p{};auto w=admission::begin(p,type,hostToken,guestToken);emit(destination,p,w);
 }
 void sendProbe(){sendDirect(broadcast(),admission::Type::Probe,0,0);}
 void sendBeacon(){
  admission::Packet p{};auto w=admission::begin(p,admission::Type::Beacon,session,0);
  w.u8(phase==SessionState::Hosting?1:0);coop::writeActor(w,local);emit(broadcast(),p,w);
 }
 void sendJoin(){
  admission::Packet p{};auto w=admission::begin(p,admission::Type::Join,session,joinId);
  coop::writeActor(w,local);emit(remote.mac,p,w);
 }
 void sendWaiting(uint32_t now){
  const uint32_t budget=decisionRemainingMs(now);if(!budget)return;
  admission::Packet p{};auto w=admission::begin(p,admission::Type::Waiting,session,joinId);
  w.u32(budget);emit(remote.mac,p,w);
 }
 void sendStart(){
  admission::Packet p{};auto w=admission::begin(p,admission::Type::Start,session,joinId);
  coop::writeActor(w,local);w.u32(seed);emit(remote.mac,p,w);
 }
 void sendReady(){
  admission::Packet p{};auto w=admission::begin(p,admission::Type::Ready,session,joinId);
  w.u32(expectedInitialHash);emit(remote.mac,p,w);
 }
 void sendControl(admission::Type type){sendDirect(remote.mac,type,session,joinId);}
 void flush(){coop::Packet p{};while(combat.endpoint.outgoing.pop(p))transmit(remote.mac,p.bytes,p.size);}
 bool resetLobby(uint32_t now){
  remote={};joinId=0;session=mint();decisionConfirmed=false;decisionAt=decisionBudget=0;
  if(!session){end(SessionEnd::Protocol,now);return false;}
  phase=SessionState::Hosting;actionAt=lastDiscovery=now;sendBeacon();return true;
 }
 void end(SessionEnd reason,uint32_t now){
  phase=SessionState::Ended;ending=reason;terminalAt=now;
  if(reason!=SessionEnd::Won&&reason!=SessionEnd::Lost)combat.stop();
 }
 void expire(uint32_t now){
  observedAt=now;
  if(phase==SessionState::AwaitingAccept&&!decisionRemainingMs(now)){
   sendControl(admission::Type::Expired);note=SessionNotice::InviteExpired;resetLobby(now);
  }else if(invitationConfirmed()&&!decisionRemainingMs(now)){
   note=SessionNotice::InviteExpired;end(SessionEnd::InviteExpired,now);
  }else if(((phase==SessionState::Joining&&!decisionConfirmed)||phase==SessionState::Starting)&&uint32_t(now-actionAt)>=kAdmissionTimeoutMs)end(SessionEnd::JoinTimeout,now);
  else if(phase==SessionState::Playing&&(uint32_t(now-lastSeen)>=kLinkTimeoutMs||
          (stallActive&&uint32_t(now-stallAt)>=kLinkTimeoutMs)))end(SessionEnd::LinkLost,now);
 }
 // Bound a broken reliable leg even if unrelated heartbeats still arrive.
 // Healthy idle play has no outstanding command/ACK and no progress deadline.
 void watchStall(uint32_t now){
  const auto& e=combat.endpoint;bool waiting=e.awaitingAck||e.pending;
  if(!waiting){stallActive=false;return;}
  if(!stallActive||e.state.revision!=stallRevision){stallActive=true;stallAt=now;stallRevision=e.state.revision;}
 }
 // Presentation is wall-clock metadata only, excluded from combat state/hash
 // and RNG. ACK/retry/heartbeat continue even while the next enemy is held.
 // Each authoritative revision gets a full window after peer confirmation.
 void watchPresentation(uint32_t now){
  const auto& e=combat.endpoint;if(!e.hasState)return;
  if(!presentationSeen||e.state.revision!=presentationRevision){
   presentationSeen=true;presentationRevision=e.state.revision;presentationAt=now;presentationConfirmed=!e.awaitingAck;
  }else if(!presentationConfirmed&&!e.awaitingAck){presentationConfirmed=true;presentationAt=now;}
 }
 void refreshState(uint32_t now){
  if(phase==SessionState::Ended)return;
  const auto& e=combat.endpoint;
  if(e.hasState&&e.state.phase!=coop::Phase::Playing){
   if(!hosting||!e.awaitingAck)end(e.state.phase==coop::Phase::Won?SessionEnd::Won:SessionEnd::Lost,now);
  }else if(phase==SessionState::Starting&&e.hasState&&!e.paused&&!e.awaitingAck){phase=SessionState::Playing;lastSeen=now;}
 }
 bool receiveEngine(const Mac& source,const uint8_t* bytes,size_t size,uint32_t now){
  bool terminal=phase==SessionState::Ended&&(ending==SessionEnd::Won||ending==SessionEnd::Lost);
  if(!combat.active()||!sameMac(source,remote.mac)||
     (phase!=SessionState::Starting&&phase!=SessionState::Playing&&!terminal)||
     (terminal&&uint32_t(now-terminalAt)>=kTerminalDrainMs)||size>coop::kMaxPacket){++rejected;return false;}
  coop::Packet p{};p.size=uint16_t(size);std::memcpy(p.bytes,bytes,size);coop::Header h{};
  if(!coop::decode(p,h)||h.nonce!=session||h.sender!=(hosting?1:0)){++malformed;return false;}
  // The first authoritative snapshot must match the explicitly admitted pair.
  if(!hosting&&!combat.endpoint.hasState&&(h.type==coop::Type::Snapshot||h.type==coop::Type::Event)){
   if(h.type!=coop::Type::Snapshot||h.payload!=coop::kStateBytes){++rejected;return false;}
   auto reader=coop::payloadReader(p,h);coop::State initial=coop::readState(reader);
   if(!reader.ok||coop::stateHash(initial)!=expectedInitialHash){++rejected;return false;}
  }
  uint32_t bad=combat.endpoint.malformed,denied=combat.endpoint.rejected;
  if(!combat.receive(source,p,now)){++rejected;return false;}
  flush();
  if(combat.endpoint.malformed!=bad||combat.endpoint.rejected!=denied){++rejected;return false;}
  // Structurally malformed/incorrectly bound/rejected packets never renew the
  // 10-second admitted-link deadline. Endpoint separately pauses at 1.8 seconds.
  lastSeen=now;remote.lastSeen=now;watchPresentation(now);watchStall(now);refreshState(now);return true;
 }
};
} // namespace coopplatform
