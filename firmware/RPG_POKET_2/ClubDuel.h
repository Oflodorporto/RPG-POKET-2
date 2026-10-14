#pragma once
#include <cstdint>

namespace club {
enum class Action : uint8_t { Attack, Offensive, Defensive, Surrender };
struct Fighter {
  uint16_t hp=0,maxHP=0,mp=0,maxMP=0,attack=0,defense=0;
  uint8_t classId=0,guard=0;
};
using MoveSender = void (*)(void *,uint16_t,uint8_t,bool);
using ResultSender = void (*)(void *,uint16_t,int8_t,uint32_t);
// Deterministic alternating turns; packets carry commands, never trusted damage.
class Duel {
public:
  Fighter fighters[2];
  uint16_t revision=0,confirmedRevision=0,lastDamage=0,lastHealing=0;
  uint8_t local=0,turn=0,lastActor=0;
  Action lastAction=Action::Attack;
  bool active=false,ready=false,waiting=false,finished=false,failed=false;
  bool surrenderRequested=false,terminalConfirmed=false;
  int8_t winner=-1;
  void begin(uint32_t id,bool initiator,uint8_t ownClass,uint8_t ownLevel,
             uint8_t otherClass,uint8_t otherLevel,MoveSender sender,void *context,uint32_t now);
  void tick(uint32_t now);
  void receive(uint16_t sequence,uint8_t action,bool ack,uint32_t now);
  void setResultSender(ResultSender fn) {resultSender=fn;}
  void receiveResult(uint16_t sequence,int8_t result,uint32_t digest,uint32_t now);
  uint32_t stateDigest() const;
  bool play(Action action,uint32_t now);
  bool canPlay(Action action) const{
    if(!active || !ready || failed || finished || surrenderRequested)return false;
    // A voluntary surrender is an independently retried request. It never predicts
    // a result and is available while waiting for the opponent or a previous ACK.
    if(action==Action::Surrender)return true;
    return !waiting && turn==local && legal(action,local);
  }
  bool acceptsMove(uint16_t sequence,uint8_t action) const;
  uint8_t cost(Action action) const{return price(fighters[local].classId,action);}
private:
  // Pure queries are header-defined so every real-renderer host harness can link
  // without the network/state-machine translation unit. Bodies are unchanged.
  static uint8_t price(uint8_t cls,Action a){
    if(a==Action::Defensive)return cls==2?3:4;
    if(a==Action::Offensive)return cls==1||cls==3?4:3;
    return 0;
  }
  MoveSender sender=nullptr;
  ResultSender resultSender=nullptr;
  void *context=nullptr;
  uint32_t started=0,lastSend=0,pendingSince=0,surrenderSince=0;
  uint32_t terminalSince=0,lastResultSend=0;
  uint16_t lastRemote=0;
  uint8_t remoteAction=0;
  bool gotReady=false,gotReadyAck=false;
  void send(uint16_t seq,uint8_t action,bool ack);
  bool legal(Action action,uint8_t actor) const{
    return !finished && (uint8_t)action<=3 && fighters[actor].mp>=price(fighters[actor].classId,action);
  }
  void apply(Action action,uint8_t actor);
  void confirmTerminal(uint32_t now);
  void sendResult(uint32_t now);
};
}
