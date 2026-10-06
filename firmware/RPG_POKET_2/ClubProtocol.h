#pragma once
#include <cstdint>
#include <cstddef>

namespace club {
constexpr uint32_t MAGIC = 0x50434C31;
constexpr uint8_t VERSION = 4;
constexpr int MAX_PEERS = 6;
enum class Type : uint8_t { Hello, Invite, Accept, Confirm, Ready, Reject, Cancel, Busy, Move, MoveAck };
enum class State : uint8_t { Listing, Outgoing, Incoming, Accepting, Connecting, Linked, Notice };
enum class Notice : uint8_t { None, Refused, Cancelled, Expired, Busy, Lost };
struct Standing {
  uint32_t points=100;
  uint16_t wins=0,losses=0,draws=0;
  uint8_t slot=0,reserved=0;
};
static_assert(sizeof(Standing)==12,"Standing wire layout changed");
struct Packet {
  uint32_t magic, session;
  uint8_t version, type, classId, level;
  char name[12];
  uint8_t available, reserved[3];
  Standing standing{};
  uint32_t stakeSession=0;
};
static_assert(sizeof(Packet) == 44, "Club wire layout changed");
struct Peer {
  bool used = false, available = false;
  uint8_t mac[6] = {}, classId = 0, level = 1;
  char name[12] = {};
  uint32_t lastSeen = 0;
  Standing standing{};
};
using Sender = void (*)(void *, const uint8_t *, const Packet &);
using MoveReceiver = void (*)(void *, uint16_t, uint8_t, bool, uint32_t);
using Admission = bool (*)(void *,uint32_t,const Peer &);

class Session {
public:
  State state = State::Listing;
  Notice notice = Notice::None;
  Peer peers[MAX_PEERS];
  Peer opponent;
  void begin(const uint8_t mac[6], const char *name, uint8_t classId,
             uint8_t level, Sender sender, void *context, uint32_t now);
  void tick(uint32_t now);
  void receive(const uint8_t source[6], const Packet &packet, uint32_t now);
  bool invite(int index, uint32_t session, uint32_t now);
  void accept(uint32_t now);
  void decline(uint32_t now);
  void cancel(uint32_t now);
  void dismiss();
  uint32_t sessionId() const {return session;}
  bool isInitiator() const {return initiator;}
  void setMoveReceiver(MoveReceiver fn, void *ctx) {moveReceiver=fn;moveContext=ctx;}
  void sendMove(uint16_t sequence, uint8_t action, bool ack);
  void setStanding(const Standing &value){profile.standing=value;}
  void setAdmission(Admission fn,void *ctx){admission=fn;admissionContext=ctx;}
  void setAvailable(bool value){fundsAvailable=value;}
private:
  Admission admission=nullptr;void *admissionContext=nullptr;
  bool fundsAvailable=true;
  MoveReceiver moveReceiver = nullptr;
  void *moveContext = nullptr;
  uint8_t ownMac[6] = {};
  Packet profile{};
  Sender sender = nullptr;
  void *context = nullptr;
  uint32_t session = 0, started = 0, lastHello = 0, lastSend = 0, lastReply = 0;
  bool initiator = false;
  uint32_t closedSession = 0, closedAt = 0;
  uint8_t closedPeer[6] = {};
  Type closedReply = Type::Cancel;
  void send(Type type, const uint8_t *destination, uint32_t id);
  void close(Notice reason, Type reply, uint32_t now);
  void retry(uint32_t now);
};
}
