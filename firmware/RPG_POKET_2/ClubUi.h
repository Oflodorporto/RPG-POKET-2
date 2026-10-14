#pragma once
#include "ClubProtocol.h"
#include "ClubDuel.h"
// Presentation records only a committed revision. Tick owns time; repeated
// renders are pure. Latest confirmed action replaces older cosmetic playback.
struct ClubVisual {
 static constexpr uint16_t duration=1100;
 uint32_t startedAt=0;
 uint16_t revision=0,damage=0,healing=0,elapsedMs=duration;
 uint8_t actor=0;club::Action action=club::Action::Attack;
 bool active()const{return revision&&elapsedMs<duration&&action!=club::Action::Surrender;}
 bool capture(const club::Duel& duel,uint32_t now){
  if(duel.confirmedRevision<=revision||duel.confirmedRevision!=duel.revision)return false;
  revision=duel.confirmedRevision;actor=duel.lastActor;action=duel.lastAction;
  damage=duel.lastDamage;healing=duel.lastHealing;startedAt=now;
  elapsedMs=action==club::Action::Surrender?duration:0;return true;
 }
 bool tick(uint32_t now){if(!active())return false;uint32_t elapsed=uint32_t(now-startedAt);elapsedMs=elapsed>=duration?duration:uint16_t(elapsed);return true;}
};
struct ClubUi {club::Session session;club::Duel duel;ClubVisual visual;bool opened=false,begun=false,resultUnconfirmed=false;int choice=0,result=0;uint32_t finishedAt=0,redraw=0;const char* notice="";};
inline ClubUi arena;
