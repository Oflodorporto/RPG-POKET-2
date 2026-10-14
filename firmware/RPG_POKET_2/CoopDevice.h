#pragma once
#include "CoopUi.h"
#if RPG_ENABLE_COOP_TRAINING
#include "Coop/CoopEligibilityBridge.h"
// Included by the sketch after globals/ClubRadio. Keep this bridge small: all
// Session/Input/radio code is separate; rendering and eligibility stay here.
// These functions never save, charge, reward or advance the campaign RNG.
inline bool coopIsOpen(){return coopruntime::active();}
inline void captureCoopTouch(bool valid,bool down,int x,int y,uint32_t now){coopruntime::capture(valid,down,x,y,now);}
inline void finishCoopExit(uint32_t now){
 const bool restoreOk=coopruntime::close();
 pendingTouch=false;gate=TouchGate{};view.page=Page::Guild;view.message="";lastActivity=now;dirty=true;
 // The framebuffer/LCD were never rotated; returning to normal rendering and
 // resetting the press gate restores portrait mapping without a stale release.
 menu.connected=WiFi.status()==WL_CONNECTED;menu.connecting=menu.scanning=false;
 menu.signalBars=0;if(!restoreOk)say("Treino fechado; reconecte Wi-Fi");
}
inline bool openCoopTraining(){
 const bool blocked=view.page!=Page::Guild||arena.opened||updateInfo.busy||menu.scanning||menu.connecting||!buffered||!touchReady||!lcdReady||combatFx.active()||journal.blocked||journal.active<0;
 uint64_t entropy=(uint64_t(esp_random())<<32)|esp_random();if(!entropy)entropy=1;
 if(!coopruntime::open(game,blocked,entropy,millis()))return false;
 pendingTouch=false;gate=TouchGate{};
 menu.connected=menu.connecting=menu.scanning=false;menu.signalBars=0;
 view.page=Page::CoopTraining;view.message="";lastActivity=millis();dirty=true;return true;
}
inline void tickCoopTraining(uint32_t now){
 const auto result=coopruntime::tick(now);
 if(result.closed){finishCoopExit(now);return;}
 if(result.activity){lastActivity=now;dirty=true;}
}
#endif
