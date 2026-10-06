#pragma once
#include <cstdint>

namespace club {
enum class Action : uint8_t { Attack, Offensive, Defensive, Surrender };
struct Fighter {
  uint16_t hp=0,maxHP=0,mp=0,maxMP=0,attack=0,defense=0;
  uint8_t classId=0,guard=0;
};
using MoveSender = void (*)(void *,uint16_t,uint8_t,bool);
// Deterministic alternating turns; packets carry commands, never trusted damage.
class Duel {
public:
  Fighter fighters[2];
  uint16_t revision=0,lastDamage=0;
  uint8_t local=0,turn=0,lastActor=0;
  Action lastAction=Action::Attack;
  bool active=false,ready=false,waiting=false,finished=false,failed=false;
  int8_t winner=-1;
  void begin(uint32_t id,bool initiator,uint8_t ownClass,uint8_t ownLevel,
             uint8_t otherClass,uint8_t otherLevel,MoveSender sender,void *context,uint32_t now);
  void tick(uint32_t now);
  void receive(uint16_t sequence,uint8_t action,bool ack,uint32_t now);
  bool play(Action action,uint32_t now);
  uint8_t cost(Action action) const;
private:
  MoveSender sender=nullptr;
  void *context=nullptr;
  uint32_t started=0,lastSend=0,pendingSince=0;
  uint16_t lastRemote=0;
  uint8_t remoteAction=0;
  bool gotReady=false,gotReadyAck=false;
  void send(uint16_t seq,uint8_t action,bool ack);
  bool legal(Action action) const;
  void apply(Action action);
};
}
