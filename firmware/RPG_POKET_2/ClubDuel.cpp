#include "ClubDuel.h"
#include <algorithm>

namespace club {
namespace {
Fighter fighter(uint8_t cls,uint8_t level){
  cls=cls<4?cls:0;level=std::max<uint8_t>(1,std::min<uint8_t>(99,level));
  const uint16_t hp[]={18,22,20,24},atk[]={6,7,8,9},def[]={4,6,5,3};
  const int growth=(int)level-3;
  Fighter f;f.classId=cls;f.hp=f.maxHP=hp[cls]+2*growth;
  f.mp=f.maxMP=(cls==0?12:8)+level-1;f.attack=atk[cls]+growth;f.defense=def[cls];
  return f;
}
}
void Duel::begin(uint32_t id,bool initiator,uint8_t ownClass,uint8_t ownLevel,
                 uint8_t otherClass,uint8_t otherLevel,MoveSender fn,void *ctx,uint32_t now){
  *this=Duel{};active=true;local=initiator?0:1;turn=id&1;
  fighters[local]=fighter(ownClass,ownLevel);fighters[1-local]=fighter(otherClass,otherLevel);
  sender=fn;context=ctx;started=now;lastSend=now-400;
}
void Duel::send(uint16_t seq,uint8_t action,bool ack){if(sender)sender(context,seq,action,ack);}
void Duel::apply(Action action,uint8_t actor){
  auto &a=fighters[actor];auto &b=fighters[1-actor];
  lastActor=actor;lastAction=action;lastDamage=0;lastHealing=0;++revision;
  a.mp-=price(a.classId,action);
  if(action==Action::Surrender){turn=actor;finished=true;winner=1-actor;return;}
  if(action==Action::Defensive){
    a.guard=a.classId<2?75:50;
    if(a.classId==3){const auto before=a.hp;a.hp=std::min<uint16_t>(a.maxHP,a.hp+std::max<int>(1,a.maxHP/4));lastHealing=a.hp-before;}
  }else{
    int damage=std::max(1,(int)a.attack-(action==Action::Offensive && a.classId==0?0:b.defense/3));
    if(action==Action::Offensive)damage=damage*(a.classId==0?180:a.classId==3?200:150)/100;
    damage=std::max(1,(damage*(100-b.guard)+99)/100);b.guard=0;
    lastDamage=std::min<int>(damage,b.hp);b.hp-=lastDamage;
    if(!b.hp){finished=true;winner=actor;}
  }
  if(!finished && revision>=200){finished=true;winner=-1;}
  turn=1-actor;
}
bool Duel::play(Action action,uint32_t now){
  if(!canPlay(action))return false;
  if(action==Action::Surrender){
    surrenderRequested=true;surrenderSince=now;
    if(!waiting){lastSend=now;send(revision+1,(uint8_t)action,false);}
    return true;
  }
  apply(action,local);waiting=true;pendingSince=lastSend=now;
  send(revision,(uint8_t)action,false);return true;
}
bool Duel::acceptsMove(uint16_t seq,uint8_t action) const{
  if(!active || !ready || failed || finished || action>3 || seq!=revision+1)return false;
  if(action==(uint8_t)Action::Surrender){
    // Simultaneous requests use the already agreed turn as a tie-breaker.
    // Only the current turn owner's request wins this ordering race.
    return !(surrenderRequested && turn==local);
  }
  return turn!=local && legal((Action)action,1-local);
}
uint32_t Duel::stateDigest() const{
  // Canonical byte order, not struct padding. This detects state disagreement;
  // it is not authentication and does not make a modified peer trustworthy.
  uint32_t hash=2166136261u;
  auto add=[&hash](uint16_t value){for(int i=0;i<2;++i){hash^=(value>>(i*8))&255;hash*=16777619u;}};
  add(revision);add(turn);add(lastActor);add((uint8_t)lastAction);add(uint8_t(winner+1));
  for(const auto &f:fighters){add(f.hp);add(f.maxHP);add(f.mp);add(f.maxMP);add(f.attack);add(f.defense);add(f.classId);add(f.guard);}
  return hash;
}
void Duel::sendResult(uint32_t now){
  if(!finished || !terminalConfirmed || !resultSender)return;
  lastResultSend=now;resultSender(context,revision,winner,stateDigest());
}
void Duel::confirmTerminal(uint32_t now){
  if(!finished)return;
  if(!terminalConfirmed){confirmedRevision=revision;terminalConfirmed=true;terminalSince=now;sendResult(now);}
}
void Duel::receiveResult(uint16_t seq,int8_t result,uint32_t digest,uint32_t now){
  if(!active || !ready || failed || result < -1 || result > 1)return;
  // A receipt can also acknowledge a surrender request whose MoveAck was lost.
  // Check a disposable copy before changing live state or accepting its outcome.
  if(!finished && surrenderRequested && !waiting && seq==revision+1){
    Duel candidate=*this;candidate.apply(Action::Surrender,local);
    if(candidate.winner!=result || candidate.stateDigest()!=digest)return;
    apply(Action::Surrender,local);surrenderRequested=false;
  }
  if(!finished || seq!=revision || result!=winner || digest!=stateDigest())return;
  waiting=false;surrenderRequested=false;confirmTerminal(now);
}
void Duel::tick(uint32_t now){
  if(!active || failed)return;
  if((!ready && uint32_t(now-started)>=15000) ||
     (waiting && uint32_t(now-pendingSince)>=10000) ||
     (surrenderRequested && uint32_t(now-surrenderSince)>=10000)){
    failed=true;return;
  }
  // Receipts remain available on the result page for the existing 11 s radio
  // grace period. Retries do not reapply combat or write persistent rewards.
  if(terminalConfirmed && uint32_t(now-terminalSince)<10000 && uint32_t(now-lastResultSend)>=400)
    sendResult(now);
  if(uint32_t(now-lastSend)<400)return;
  lastSend=now;
  if(!ready)send(0,0,false);
  else if(waiting)send(revision,(uint8_t)lastAction,false);
  else if(surrenderRequested)send(revision+1,(uint8_t)Action::Surrender,false);
}
void Duel::receive(uint16_t seq,uint8_t action,bool ack,uint32_t now){
  if(!active || failed || action>3)return;
  if(seq==0){
    if(action!=0)return;
    if(ack)gotReadyAck=true;
    else {gotReady=true;send(0,0,true);}
    ready=gotReady&&gotReadyAck;return;
  }
  if(!ready)return;
  if(ack){
    if(waiting && seq==revision && action==(uint8_t)lastAction){waiting=false;confirmedRevision=revision;confirmTerminal(now);}
    else if(surrenderRequested && !waiting && seq==revision+1 && action==(uint8_t)Action::Surrender){
      apply(Action::Surrender,local);surrenderRequested=false;confirmTerminal(now);
    }
    return;
  }
  if(seq==lastRemote && action==remoteAction){send(seq,action,true);return;}
  if(!acceptsMove(seq,action))return;
  // A valid next command also acknowledges our previous move if its ACK was lost.
  waiting=false;apply((Action)action,1-local);confirmedRevision=revision;lastRemote=seq;remoteAction=action;
  if(finished)surrenderRequested=false;
  send(seq,action,true);lastSend=now;confirmTerminal(now);
}
}
