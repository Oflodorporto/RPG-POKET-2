#pragma once
#include "Coop/CoopRadioDevice.h"
#include "Coop/CoopInput.h"
// Included by the sketch after globals/ClubRadio. These functions never call
// journal, savedTransition, reserveFight, settleFight or campaign RNG helpers.
static_assert(sizeof(coopplatform::Session)<=6144,"Coop session exceeded bounded internal RAM budget");
static_assert(sizeof(coopplatform::Input)<=1024,"Coop input exceeded bounded internal RAM budget");
static_assert(sizeof(coopradio::Device)<=256,"Coop device state grew unexpectedly");
static_assert(sizeof(coopradio::queueBytes)+sizeof(coopradio::queueState)<=3072,"Coop inbox grew unexpectedly");
inline coopradio::Device coopDevice;
inline coopplatform::Input coopInput;
inline bool coopClosing=false;inline uint32_t coopCloseAt=0,coopByeAt=0;
inline bool coopIsOpen(){return coopDevice.active();}
inline void captureCoopTouch(bool valid,bool down,int x,int y,uint32_t now){coopInput.capture(coopSession,valid,down,x,y,now);}
inline void finishCoopExit(uint32_t now){
 coopDevice.close();coopInput.reset();coopClosing=false;
 pendingTouch=false;gate=TouchGate{};view.page=Page::Guild;view.message="";lastActivity=now;dirty=true;
 // The framebuffer/LCD were never rotated; returning to normal rendering and
 // resetting the press gate restores portrait mapping without a stale release.
 menu.connected=WiFi.status()==WL_CONNECTED;menu.connecting=menu.scanning=false;
 menu.signalBars=0;if(!coopDevice.restoreOk)say("Treino fechado; reconecte Wi-Fi");
}
inline bool openCoopTraining(){
 const bool blocked=view.page!=Page::Guild||arena.opened||updateInfo.busy||menu.scanning||menu.connecting||!buffered||!touchReady||!lcdReady||combatFx.active()||journal.blocked||journal.active<0;
 uint64_t entropy=(uint64_t(esp_random())<<32)|esp_random();if(!entropy)entropy=1;
 if(coopDevice.open(coopSession,game,blocked,entropy,millis())!=coopradio::OpenError::None)return false;
 coopInput.reset();coopClosing=false;pendingTouch=false;gate=TouchGate{};
 menu.connected=menu.connecting=menu.scanning=false;menu.signalBars=0;
 view.page=Page::CoopTraining;view.message="";lastActivity=millis();dirty=true;return true;
}
inline void tickCoopTraining(uint32_t now){
 coopDevice.tick(now);
 if(coopClosing){
  if(uint32_t(now-coopCloseAt)>=350){finishCoopExit(now);return;}
  if(uint32_t(now-coopByeAt)>=100){coopByeAt=now;coopSession.leave(now);}
  return;
 }
 using coopview::Control;
 const auto action=coopInput.dispatch(coopSession,now);
 if(action!=Control::None){
  lastActivity=now;dirty=true;
  if(action==Control::Exit){coopSession.leave(now);coopClosing=true;coopCloseAt=coopByeAt=now;coopInput.reset();}
  else if(action==Control::Host)coopSession.host(now);
  else if(action>=Control::Join0&&action<=Control::Join3)coopSession.join(unsigned(action)-unsigned(Control::Join0),now);
  else if(action==Control::Accept)coopSession.accept(now);
  else if(action==Control::Decline)coopSession.decline(now);
 }
}
