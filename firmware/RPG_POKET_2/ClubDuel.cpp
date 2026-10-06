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
uint8_t price(uint8_t cls,Action a){
  if(a==Action::Defensive)return cls==2?3:4;
  if(a==Action::Offensive)return cls==1||cls==3?4:3;
  return 0;
}
}
void Duel::begin(uint32_t id,bool initiator,uint8_t ownClass,uint8_t ownLevel,
                 uint8_t otherClass,uint8_t otherLevel,MoveSender fn,void *ctx,uint32_t now){
  *this=Duel{};active=true;local=initiator?0:1;turn=id&1;
  fighters[local]=fighter(ownClass,ownLevel);fighters[1-local]=fighter(otherClass,otherLevel);
  sender=fn;context=ctx;started=now;lastSend=now-400;
}
void Duel::send(uint16_t seq,uint8_t action,bool ack){if(sender)sender(context,seq,action,ack);}
uint8_t Duel::cost(Action action) const{return price(fighters[local].classId,action);}
bool Duel::legal(Action action) const{
  return !finished && (uint8_t)action<=3 && fighters[turn].mp>=price(fighters[turn].classId,action);
}
void Duel::apply(Action action){
  auto &a=fighters[turn];auto &b=fighters[1-turn];
  lastActor=turn;lastAction=action;lastDamage=0;++revision;
  a.mp-=price(a.classId,action);
  if(action==Action::Surrender){finished=true;winner=1-turn;return;}
  if(action==Action::Defensive){
    a.guard=a.classId<2?75:50;
    if(a.classId==3)a.hp=std::min<uint16_t>(a.maxHP,a.hp+std::max<int>(1,a.maxHP/4));
  }else{
    int damage=std::max(1,(int)a.attack-(action==Action::Offensive && a.classId==0?0:b.defense/3));
    if(action==Action::Offensive)damage=damage*(a.classId==0?180:a.classId==3?200:150)/100;
    damage=std::max(1,(damage*(100-b.guard)+99)/100);b.guard=0;
    lastDamage=std::min<int>(damage,b.hp);b.hp-=lastDamage;
    if(!b.hp){finished=true;winner=turn;}
  }
  if(!finished && revision>=200){finished=true;winner=-1;}
  turn=1-turn;
}
bool Duel::play(Action action,uint32_t now){
  if(!active || !ready || failed || waiting || turn!=local || !legal(action))return false;
  apply(action);waiting=true;pendingSince=lastSend=now;
  send(revision,(uint8_t)action,false);return true;
}
void Duel::tick(uint32_t now){
  if(!active || failed)return;
  if((!ready && uint32_t(now-started)>=15000) || (waiting && uint32_t(now-pendingSince)>=10000)){
    failed=true;return;
  }
  if(uint32_t(now-lastSend)<400)return;
  lastSend=now;
  if(!ready)send(0,0,false);
  else if(waiting)send(revision,(uint8_t)lastAction,false);
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
    if(waiting && seq==revision && action==(uint8_t)lastAction)waiting=false;
    return;
  }
  if(seq==lastRemote && action==remoteAction){send(seq,action,true);return;}
  if(seq!=revision+1 || turn==local || !legal((Action)action))return;
  // A valid next turn also acknowledges our previous turn if its ACK was lost.
  waiting=false;apply((Action)action);lastRemote=seq;remoteAction=action;
  send(seq,action,true);lastSend=now;
}
}
