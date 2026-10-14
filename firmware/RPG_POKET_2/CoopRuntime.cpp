#include "CoopUi.h"
#if RPG_ENABLE_COOP_TRAINING
#include "Coop/CoopRadioDevice.h"
#include "Coop/CoopInput.h"
// This translation unit deliberately does not include View/Menu/Update headers.
// It also excludes art/font/Save/Persistence; rendering and eligibility remain
// in the original sketch TU. Mutable menu effects stay in the sketch bridge.
static_assert(sizeof(coopplatform::Session)<=6144,"Coop session exceeded bounded internal RAM budget");
static_assert(sizeof(coopplatform::Input)<=1024,"Coop input exceeded bounded internal RAM budget");
static_assert(sizeof(coopradio::Device)<=256,"Coop device state grew unexpectedly");
static_assert(sizeof(coopradio::queueBytes)+sizeof(coopradio::queueState)<=3072,"Coop inbox grew unexpectedly");
// Preserve one named instance for linking and target object-budget evidence.
coopplatform::Session coopSession;
coopplatform::Input coopInput;
coopradio::Device coopDevice;
bool coopClosing=false;
namespace {
uint32_t closeAt=0,byeAt=0;
}
namespace coopruntime {
bool active(){return coopDevice.active();}
bool open(const rpg::Game& game,bool blocked,uint64_t entropy,uint32_t now){
 if(coopDevice.open(coopSession,game,blocked,entropy,now)!=coopradio::OpenError::None)return false;
 coopInput.reset();coopClosing=false;return true;
}
void capture(bool valid,bool down,int x,int y,uint32_t now){coopInput.capture(coopSession,valid,down,x,y,now);}
bool close(){
 coopDevice.close();coopInput.reset();coopClosing=false;return coopDevice.restoreOk;
}
TickResult tick(uint32_t now){
 TickResult result{};coopDevice.tick(now);
 if(coopClosing){
  if(uint32_t(now-closeAt)>=350){result.closed=true;result.restoreOk=close();return result;}
  if(uint32_t(now-byeAt)>=100){byeAt=now;coopSession.leave(now);}
  return result;
 }
 using coopview::Control;
 const auto action=coopInput.dispatch(coopSession,now);
 if(action!=Control::None){
  result.activity=true;
  if(action==Control::Exit){coopSession.leave(now);coopClosing=true;closeAt=byeAt=now;coopInput.reset();}
  else if(action==Control::Host)coopSession.host(now);
  else if(action>=Control::Join0&&action<=Control::Join3)coopSession.join(unsigned(action)-unsigned(Control::Join0),now);
  else if(action==Control::Accept)coopSession.accept(now);
  else if(action==Control::Decline)coopSession.decline(now);
 }
 return result;
}
}
#endif
