#pragma once
#include "CoopWire.h"
namespace coop {
constexpr uint32_t kRetryMs=200,kHeartbeatMs=500,kDisconnectMs=1800;
constexpr uint8_t kRetryBurst=8;
struct UiEvent {Effect effect{};uint32_t revision=0,hash=0;uint8_t wave=0,turn=0;uint16_t round=0;Phase phase=Phase::Playing;};
// All queues, state and receipts are RAM only; no persistence/transport callbacks.
class Endpoint {
public:
 State state{};Queue<Packet,8> outgoing{};Queue<UiEvent,8> ui{};
 bool host=false,hasState=false,paused=true,awaitingAck=false,pending=false;
 uint8_t local=0;uint64_t nonce=0;Receipt receipt{};Error lastError=Error::None;
 uint32_t malformed=0,rejected=0,retries=0,snapshots=0,eventsApplied=0,duplicateEvents=0;
 void hostStart(const State&s,uint64_t id,uint32_t now){State initial=s;*this=Endpoint{};host=true;local=0;nonce=id;state=initial;hasState=validState(initial)&&id;lastSeen=lastTx=lastHeartbeat=now;paused=true;if(hasState)sendSnapshot(now);}
 void clientStart(uint64_t id,uint32_t now){*this=Endpoint{};local=1;nonce=id;lastSeen=lastTx=lastHeartbeat=now;requestSnapshot(now);}
 bool canPlay()const{return hasState&&!paused&&!awaitingAck&&!pending&&state.phase==Phase::Playing&&state.actors[state.turn].owner==local;}
 Error submit(Action action,uint8_t target,uint32_t now){
  if(!canPlay())return lastError=paused?Error::Paused:Error::Busy;
  if(state.accepted[local]==UINT32_MAX)return lastError=Error::Limit;
  Command c{};c.sequence=state.accepted[local]+1;c.revision=state.revision;c.actor=state.turn;c.target=target;c.action=action;
  Error e=legal(state,local,c);if(e!=Error::None)return lastError=e;
  if(host)return processCommand(local,c,now);
  pendingCommand=c;pending=true;attempts=0;sendCommand(now);return lastError=Error::None;
 }
 void tick(uint32_t now,bool allowEnemyStep=true){
  if(!nonce)return;
  if(uint32_t(now-lastSeen)>=kDisconnectMs)paused=true;
  if(uint32_t(now-lastHeartbeat)>=kHeartbeatMs){lastHeartbeat=now;sendHeartbeat();}
  if(host){
   if(awaitingAck&&uint32_t(now-lastTx)>=kRetryMs){if(attempts<kRetryBurst){outgoing.push(lastReliable);lastTx=now;++attempts;++retries;}else paused=true;}
   if(paused&&uint32_t(now-lastSync)>=kHeartbeatMs)sendSnapshot(now);
   if(allowEnemyStep&&hasState&&!paused&&!awaitingAck&&state.phase==Phase::Playing&&state.actors[state.turn].owner==kNoPeer)resolve(kNoPeer,enemyCommand(state),now,true);
  }else{
   if(pending&&!paused&&uint32_t(now-lastTx)>=kRetryMs){if(attempts<kRetryBurst){sendCommand(now);++retries;}else{paused=true;requestSnapshot(now);}}
   if((paused||wantSnapshot)&&uint32_t(now-lastSync)>=kHeartbeatMs)requestSnapshot(now);
  }
 }
 void receive(uint8_t source,const Packet&p,uint32_t now){
  Header h{};if(!decode(p,h)||source!=h.sender||source!=(host?1:0)||h.nonce!=nonce){++malformed;return;}
  Reader r=payloadReader(p,h);
  if(host){
   if(h.type==Type::Command){if(h.payload!=7){++malformed;return;}Command c=readCommand(r,h.sequence);if(!r.ok){++malformed;return;}lastSeen=now;Error e=processCommand(source,c,now);if(e!=Error::None)sendReject(c.sequence,e);return;}
   if(h.type==Type::Ack){if(h.payload!=4){++malformed;return;}uint32_t hash=r.u32();if(h.sequence!=state.revision||hash!=stateHash(state)){++rejected;return;}lastSeen=now;awaitingAck=false;paused=false;attempts=0;confirmReceipt();return;}
   if(h.type==Type::SnapshotRequest){if(h.payload){++malformed;return;}lastSeen=now;sendSnapshot(now);return;}
   if(h.type==Type::Heartbeat){if(h.payload!=4){++malformed;return;}uint32_t hash=r.u32();lastSeen=now;if(h.sequence!=state.revision||hash!=stateHash(state)||paused)sendSnapshot(now);return;}
   ++malformed;return;
  }
  if(h.type==Type::Event||h.type==Type::Snapshot){
   bool event=h.type==Type::Event;size_t expected=kStateBytes+(event?16:0);if(h.payload!=expected){++malformed;return;}
   Effect effect{};if(event)effect=readEffect(r);State incoming=readState(r);
   if(!r.ok||r.n!=r.cap||!validState(incoming)||h.sequence!=incoming.revision){++malformed;return;}
   if(event&&(effect.peer!=kNoPeer&&effect.peer>=kPlayers)){++malformed;return;}
   if(event&&(uint8_t(effect.action)>uint8_t(Action::Focus)||effect.fromWave>2)){++malformed;return;}
   if(receipt.valid&&(incoming.revision!=receipt.revision||stateHash(incoming)!=receipt.stateHash)){++rejected;sendAck();return;}
   if(hasState&&incoming.revision<state.revision){++rejected;sendAck();return;}
   if(event&&hasState&&incoming.revision==state.revision){
    if(stateHash(incoming)!=stateHash(state)){++rejected;paused=true;requestSnapshot(now);return;}
    ++duplicateEvents;lastSeen=now;sendAck();return;
   }
   if(event&&(!hasState||incoming.revision!=state.revision+1||effect.beforeHash!=stateHash(state))){++rejected;paused=true;requestSnapshot(now);return;}
   if(event){
    // Confirmed presentation must refer to the old, valid actor/target instances.
    uint8_t actor=actorSlot(state,effect.actor),target=actorSlot(state,effect.target);
    if(actor==kNoActor||target==kNoActor||effect.fromWave!=state.wave){++malformed;return;}
    bool enemy=effect.peer==kNoPeer;
    if((enemy&&effect.commandSequence!=0)||(!enemy&&(state.accepted[effect.peer]==UINT32_MAX||effect.commandSequence!=state.accepted[effect.peer]+1))){++malformed;return;}
    Command check{effect.commandSequence,state.revision,actor,target,effect.action};State projected=state;Effect calculated{};
    if(apply(projected,effect.peer,check,calculated,enemy)!=Error::None){++malformed;return;}
    calculated.beforeHash=stateHash(state);
    // Validate every confirmed effect and state byte by deterministic local replay.
    uint8_t expectedBytes[16+kStateBytes]{};Writer verify{expectedBytes,sizeof(expectedBytes)};writeEffect(verify,calculated);writeState(verify,projected);
    bool identical=verify.ok;for(size_t i=0;i<sizeof(expectedBytes);++i)identical=identical&&expectedBytes[i]==p.bytes[kHeaderBytes+i];
    if(!identical){++malformed;return;}
   }
   if(!event&&hasState&&incoming.revision==state.revision&&stateHash(incoming)!=stateHash(state)&&!wantSnapshot){++rejected;paused=true;requestSnapshot(now);return;}
   state=incoming;hasState=true;paused=false;wantSnapshot=false;lastSeen=now;
   if(pending&&state.accepted[local]>=pendingCommand.sequence)pending=false;
   // On rejected/stale outstanding intent, don't silently rebase it to a different turn.
   if(pending&&state.revision!=pendingCommand.revision){pending=false;lastError=Error::Revision;}
   if(pending&&!event){attempts=0;lastTx=now;} // Same-host resync revives the unchanged original intent.
   if(event){++eventsApplied;present(effect);}else ++snapshots;
   confirmReceipt();sendAck();return;
  }
  if(h.type==Type::Reject){if(h.payload!=9){++malformed;return;}Error e=Error(r.u8());uint32_t rev=r.u32(),hash=r.u32();if(uint8_t(e)>uint8_t(Error::Limit)){++malformed;return;}lastSeen=now;if(pending&&h.sequence==pendingCommand.sequence){pending=false;lastError=e;}if(!hasState||rev!=state.revision||hash!=stateHash(state)||e==Error::Paused){paused=true;requestSnapshot(now);}return;}
  if(h.type==Type::Heartbeat){if(h.payload!=4){++malformed;return;}uint32_t hash=r.u32();lastSeen=now;if(!hasState||h.sequence!=state.revision||hash!=stateHash(state)){paused=true;requestSnapshot(now);}return;}
  ++malformed;
 }
private:
 uint32_t lastSeen=0,lastTx=0,lastHeartbeat=0,lastSync=0;
 uint8_t attempts=0;bool wantSnapshot=false;
 Command pendingCommand{},lastCommands[kPlayers]{};bool cached[kPlayers]={};
 Packet lastReliable{};
 void confirmReceipt(){if(hasState&&state.phase!=Phase::Playing)receipt={nonce,state.revision,stateHash(state),state.phase,true};}
 void present(const Effect&e){UiEvent u{};u.effect=e;u.revision=state.revision;u.hash=stateHash(state);u.wave=state.wave;u.round=state.round;u.turn=state.actors[state.turn].id;u.phase=state.phase;
  // Slow animation never blocks reliability. Drop oldest visual event, count it.
  if(ui.count==8){UiEvent discarded{};ui.pop(discarded);++ui.overflow;}ui.push(u);
 }
 void sendCommand(uint32_t now){Packet p{};auto w=begin(p,Type::Command,local,nonce,pendingCommand.sequence);writeCommand(w,pendingCommand);finish(p,w);outgoing.push(p);lastTx=now;++attempts;}
 void sendAck(){Packet p{};auto w=begin(p,Type::Ack,local,nonce,state.revision);w.u32(stateHash(state));finish(p,w);outgoing.push(p);}
 void sendHeartbeat(){Packet p{};auto w=begin(p,Type::Heartbeat,local,nonce,hasState?state.revision:0);w.u32(hasState?stateHash(state):0);finish(p,w);outgoing.push(p);}
 void requestSnapshot(uint32_t now){wantSnapshot=true;Packet p{};auto w=begin(p,Type::SnapshotRequest,local,nonce,hasState?state.revision:0);finish(p,w);outgoing.push(p);lastSync=now;}
 void sendSnapshot(uint32_t now){if(!hasState)return;Packet p{};auto w=begin(p,Type::Snapshot,local,nonce,state.revision);writeState(w,state);finish(p,w);outgoing.push(p);lastReliable=p;awaitingAck=true;lastTx=lastSync=now;attempts=0;}
 void sendReject(uint32_t seq,Error e){++rejected;lastError=e;Packet p{};auto w=begin(p,Type::Reject,local,nonce,seq);w.u8(uint8_t(e));w.u32(state.revision);w.u32(stateHash(state));finish(p,w);outgoing.push(p);}
 Error processCommand(uint8_t peer,const Command&c,uint32_t now){
  if(peer>=kPlayers)return Error::Peer;
  if(c.sequence==state.accepted[peer]&&cached[peer]){
   if(!sameCommand(c,lastCommands[peer]))return Error::Conflict;
   sendSnapshot(now);return Error::None; // Idempotent repair, never reroll.
  }
  if(state.accepted[peer]==UINT32_MAX||c.sequence!=state.accepted[peer]+1)return Error::Sequence;
  if(paused)return Error::Paused;if(awaitingAck)return Error::Busy;
  Error e=legal(state,peer,c);if(e!=Error::None)return e;
  lastCommands[peer]=c;cached[peer]=true;return resolve(peer,c,now,false);
 }
 Error resolve(uint8_t peer,const Command&c,uint32_t now,bool enemy){
  uint32_t before=stateHash(state);Effect effect{};Error e=apply(state,peer,c,effect,enemy);if(e!=Error::None)return lastError=e;effect.beforeHash=before;
  Packet p{};auto w=begin(p,Type::Event,local,nonce,state.revision);writeEffect(w,effect);writeState(w,state);finish(p,w);
  outgoing.push(p);lastReliable=p;awaitingAck=true;lastTx=now;attempts=0;++eventsApplied;present(effect);return lastError=Error::None;
 }
};
}
