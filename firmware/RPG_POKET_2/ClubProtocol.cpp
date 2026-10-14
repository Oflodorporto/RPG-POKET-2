#include "ClubProtocol.h"
#include <cstring>

namespace club {
namespace {
const uint8_t broadcast[6] = {255,255,255,255,255,255};
bool same(const uint8_t *a, const uint8_t *b) {return std::memcmp(a,b,6) == 0;}
bool valid(const Packet &p) {
  if (p.magic != MAGIC || p.version != VERSION || p.type > (uint8_t)Type::Result ||
      p.classId > 3 || p.race>3 || p.shirt>7 || p.trousers>7 || p.appearanceReserved || p.level < 1 || p.level > 99 || p.available > 1 ||
      !std::memchr(p.name,0,12) || p.standing.points>99999 || p.standing.slot>2 || p.standing.reserved) return false;
  if(p.type < (uint8_t)Type::Move && (p.reserved[0] || p.reserved[1] || p.reserved[2]))return false;
  if((p.type == (uint8_t)Type::Move || p.type == (uint8_t)Type::MoveAck) && p.reserved[2]>3)return false;
  if(p.type == (uint8_t)Type::Result && (p.reserved[2]>2 || !(p.reserved[0]|p.reserved[1])))return false;
  if(p.type != (uint8_t)Type::Result && p.resultDigest)return false;
  const auto type=Type(p.type);
  if((type==Type::Invite || type==Type::Accept || type==Type::Confirm || type==Type::Ready || type==Type::Move || type==Type::MoveAck || type==Type::Result) && p.stakeSession!=p.session)return false;
  for (int i=0; p.name[i]; ++i)
    if (!(p.name[i] >= 'A' && p.name[i] <= 'Z') && p.name[i] != ' ') return false;
  return p.type == (uint8_t)Type::Hello ? p.session == 0 : p.session != 0;
}
Peer makePeer(const uint8_t *mac, const Packet &p, uint32_t now) {
  Peer peer;
  peer.used = true; peer.available = p.available;peer.version=p.version;
  std::memcpy(peer.mac,mac,6); std::memcpy(peer.name,p.name,12);
  peer.classId=p.classId;peer.level=p.level;peer.lastSeen=now;
  peer.standing=p.standing;peer.race=p.race;peer.shirt=p.shirt;peer.trousers=p.trousers;
  return peer;
}
}

void Session::begin(const uint8_t mac[6], const char *name, uint8_t classId,
                    uint8_t level, Sender sendFn, void *ctx, uint32_t now) {
  *this = Session{};
  std::memcpy(ownMac,mac,6);
  profile.magic=MAGIC;profile.version=VERSION;
  profile.classId=classId<4?classId:0;profile.level=level>=1&&level<=99?level:1;
  for(int i=0;i<11 && name[i];++i)
    profile.name[i]=(name[i]>='A'&&name[i]<='Z')||name[i]==' '?name[i]:' ';
  sender=sendFn;context=ctx;lastHello=now-1500;
}

void Session::send(Type type, const uint8_t *destination, uint32_t id) {
  Packet p=profile;p.type=(uint8_t)type;p.session=id;
  p.available=state==State::Listing && fundsAvailable;
  if(sender) sender(context,destination,p);
}

void Session::sendMove(uint16_t sequence,uint8_t action,bool ack){
  if(state!=State::Linked || action>3)return;
  Packet p=profile;p.type=(uint8_t)(ack?Type::MoveAck:Type::Move);p.session=session;
  p.reserved[0]=sequence&255;p.reserved[1]=sequence>>8;p.reserved[2]=action;
  if(sender)sender(context,opponent.mac,p);
}

void Session::sendResult(uint16_t sequence,int8_t winner,uint32_t digest){
  if(state!=State::Linked || !sequence || winner < -1 || winner > 1)return;
  Packet p=profile;p.type=(uint8_t)Type::Result;p.session=session;
  p.reserved[0]=sequence&255;p.reserved[1]=sequence>>8;p.reserved[2]=uint8_t(winner+1);
  p.resultDigest=digest;
  if(sender)sender(context,opponent.mac,p);
}

void Session::close(Notice reason, Type reply, uint32_t now) {
  closedLinked=state==State::Linked;closedSession=session;closedAt=now;closedReply=reply;
  std::memcpy(closedPeer,opponent.mac,6);
  state=State::Notice;notice=reason;
}

void Session::retry(uint32_t now) {
  lastSend=now;
  if(state==State::Outgoing)send(Type::Invite,opponent.mac,session);
  else if(state==State::Accepting)send(Type::Accept,opponent.mac,session);
  else if(state==State::Connecting || (state==State::Linked && initiator))
    send(Type::Confirm,opponent.mac,session);
}

void Session::tick(uint32_t now) {
  for(auto &p:peers)if(p.used && uint32_t(now-p.lastSeen)>=6000)p.used=false;
  if(uint32_t(now-lastHello)>=1500){lastHello=now;send(Type::Hello,broadcast,0);}
  if(state==State::Listing || state==State::Notice)return;
  if(state==State::Linked){
    if(uint32_t(now-lastReply)>=6000){close(Notice::Lost,Type::Cancel,now);return;}
  } else if(uint32_t(now-started)>=30000){
    send(Type::Cancel,opponent.mac,session);close(Notice::Expired,Type::Cancel,now);return;
  }
  if(uint32_t(now-lastSend)>=750)retry(now);
}

bool Session::invite(int index,uint32_t id,uint32_t now) {
  if(state!=State::Listing || !id || index<0 || index>=MAX_PEERS ||
     !peers[index].used || !peers[index].available || peers[index].version!=VERSION || uint32_t(now-peers[index].lastSeen)>=6000)return false;
  opponent=peers[index];
  if(admission && !admission(admissionContext,id,opponent))return false;
  profile.stakeSession=id;session=id;initiator=true;state=State::Outgoing;
  started=lastReply=now;notice=Notice::None;retry(now);return true;
}
void Session::accept(uint32_t now) {
  if(state!=State::Incoming)return;
  if(uint32_t(now-started)>=30000){close(Notice::Expired,Type::Cancel,now);return;}
  if(admission && !admission(admissionContext,session,opponent))return;
  profile.stakeSession=session;
  state=State::Accepting;retry(now);
}
void Session::decline(uint32_t now) {
  if(state!=State::Incoming)return;
  send(Type::Reject,opponent.mac,session);close(Notice::Refused,Type::Reject,now);
  dismiss();
}
void Session::cancel(uint32_t now) {
  if(state!=State::Listing && state!=State::Notice){
    send(Type::Cancel,opponent.mac,session);close(Notice::Cancelled,Type::Cancel,now);
  }
  dismiss();
}
void Session::dismiss(){state=State::Listing;notice=Notice::None;}

void Session::receive(const uint8_t source[6],const Packet &p,uint32_t now) {
  // v4 discovery is visible only to explain why matching is unavailable. No
  // invitation, admission, move or result from an older version is accepted.
  bool legacyHello=p.version==4 && p.type==(uint8_t)Type::Hello;
  Packet check=p;if(legacyHello)check.version=VERSION;
  if(!valid(check) || (!legacyHello && p.version!=VERSION) || same(source,ownMac) || (source[0]&1))return;
  const Type type=(Type)p.type;
  if(type==Type::Hello){
    int slot=-1;
    for(int i=0;i<MAX_PEERS;++i)if(peers[i].used && same(peers[i].mac,source)){slot=i;break;}
    if(slot<0)for(int i=0;i<MAX_PEERS;++i)if(!peers[i].used){slot=i;break;}
    if(slot>=0)peers[slot]=makePeer(source,p,now);
    return;
  }
  // A closing packet may overtake an honest terminal proof. Admit only ACKs
  // and receipts for the just-linked peer/session, never new commands, within
  // a bounded grace window. The Duel's original command deadline still rules;
  // the adapter refuses outcome changes after it has durably settled.
  if(type==Type::MoveAck || type==Type::Result){
    const bool closing=state==State::Notice && closedLinked && session==closedSession &&
      (notice==Notice::Cancelled || notice==Notice::Lost) && uint32_t(now-closedAt)<10000;
    if((state==State::Linked || closing) && p.session==session && same(source,opponent.mac) && p.classId==opponent.classId && p.level==opponent.level && p.race==opponent.race && p.shirt==opponent.shirt && p.trousers==opponent.trousers){
      const auto seq=(uint16_t)(p.reserved[0]|(p.reserved[1]<<8));
      if(type==Type::MoveAck && moveReceiver)moveReceiver(moveContext,seq,p.reserved[2],true,now);
      else if(type==Type::Result && resultReceiver)resultReceiver(resultContext,seq,int8_t(p.reserved[2])-1,p.resultDigest,now);
    }
    return;
  }
  if(p.session==closedSession && same(source,closedPeer) && uint32_t(now-closedAt)<30000){
    if(type==Type::Invite || type==Type::Accept || type==Type::Confirm)
      send(closedReply,source,p.session);
    return;
  }
  if(type==Type::Invite){
    if(state==State::Listing){
      opponent=makePeer(source,p,now);session=p.session;started=lastReply=now;
      initiator=false;state=State::Incoming;notice=Notice::None;return;
    }
    if(state==State::Outgoing){send(Type::Busy,source,p.session);return;}
    if(p.session==session && same(source,opponent.mac)){
      if(state==State::Accepting)retry(now);
      return;
    }
    send(Type::Busy,source,p.session);return;
  }
  if(state==State::Listing || state==State::Notice || p.session!=session || !same(source,opponent.mac))return;
  if(type==Type::Move){
    if(state==State::Linked && p.classId==opponent.classId && p.level==opponent.level && p.race==opponent.race && p.shirt==opponent.shirt && p.trousers==opponent.trousers && moveReceiver)
      moveReceiver(moveContext,(uint16_t)(p.reserved[0]|(p.reserved[1]<<8)),p.reserved[2],false,now);
    return;
  }
  if(type==Type::Cancel){close(Notice::Cancelled,Type::Cancel,now);return;}
  if(type==Type::Reject && state==State::Outgoing){close(Notice::Refused,Type::Cancel,now);return;}
  if(type==Type::Busy && state==State::Outgoing){close(Notice::Busy,Type::Cancel,now);return;}
  if(type==Type::Accept && initiator && (state==State::Outgoing || state==State::Connecting)){
    state=State::Connecting;lastReply=now;retry(now);return;
  }
  if(type==Type::Confirm && !initiator && (state==State::Accepting || state==State::Linked)){
    state=State::Linked;lastReply=now;send(Type::Ready,source,session);return;
  }
  if(type==Type::Ready && initiator && (state==State::Connecting || state==State::Linked)){
    state=State::Linked;lastReply=now;
  }
}
}
