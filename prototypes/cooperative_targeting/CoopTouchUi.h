#pragma once
// Local-only single-contact controller. Logical coordinates are landscape 320 x 240.
// It adapts the frozen authoritative Endpoint. No firmware/touch-driver integration.
#include "../cooperative/CoopEndpoint.h"
namespace coopui {
using namespace coop;
struct Rect { int16_t x,y,w,h; bool contains(int32_t px,int32_t py) const {return px>=x&&py>=y&&px<int32_t(x)+w&&py<int32_t(y)+h;} };
constexpr Rect kButtons[4]={{4,199,75,30},{83,199,75,30},{162,199,75,30},{241,199,75,30}};
constexpr Rect kEnemyHits[3]={{185,45,61,50},{252,70,63,53},{185,98,61,50}};
constexpr Rect kBossHit={202,50,104,98};
constexpr int16_t kWidth=320,kHeight=240;
enum class Mode:uint8_t { Action, Ability, Items, Target, Pending };
enum class Notice:uint8_t { Choose, ChooseTarget, ExperimentalFocus, Items, Cancelled, Pending, Confirmed, Updated, Unavailable, Rejected };
enum class HitKind:uint8_t { None, Button, Enemy };
struct Hit {HitKind kind=HitKind::None;uint8_t index=0;};
inline bool operator==(const Hit&a,const Hit&b){return a.kind==b.kind&&a.index==b.index;}
inline bool intersects(const Rect&a,const Rect&b){return a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;}
inline const char* modeName(Mode m){switch(m){case Mode::Action:return "action";case Mode::Ability:return "ability";case Mode::Items:return "items";case Mode::Target:return "target";case Mode::Pending:return "pending";}return "action";}
class TouchUi {
public:
 Mode mode=Mode::Action;Action selected=Action::Attack;Notice notice=Notice::Choose;Error error=Error::None;
 uint32_t submitted=0,cancelled=0,staleCleared=0;
 // Call after transport updates and before drawing. Every onTouch also calls it.
 void sync(const Endpoint&e){
  const uint32_t hash=e.hasState?stateHash(e.state):0;
  const bool sessionChanged=observed&&e.nonce!=seenNonce;
  const bool stateChanged=observed&&(e.hasState!=seenHasState||e.state.revision!=seenRevision||hash!=seenHash);
  if(sessionChanged){mode=Mode::Action;notice=Notice::Updated;error=Error::None;armed={};++staleCleared;}
  else if(mode==Mode::Pending){
   if(!e.pending&&!e.awaitingAck){
    const bool accepted=e.hasState&&submittedPeer<kPlayers&&e.state.accepted[submittedPeer]>=submittedSequence;
    mode=Mode::Action;error=accepted?Error::None:e.lastError;notice=accepted?Notice::Confirmed:Notice::Rejected;armed={};
   }
  }else if(stateChanged&&(mode!=Mode::Action||armed.kind!=HitKind::None)){
   mode=Mode::Action;notice=Notice::Updated;error=Error::None;armed={};++staleCleared;
  }
  if(mode!=Mode::Pending&&!playable(e)){
   if(mode!=Mode::Action){mode=Mode::Action;notice=Notice::Unavailable;++staleCleared;}
   armed={};
  }
  seenNonce=e.nonce;seenRevision=e.state.revision;seenHash=hash;seenHasState=e.hasState;observed=true;
 }
 bool playable(const Endpoint&e)const{return e.local<kPlayers&&e.hasState&&validState(e.state)&&e.canPlay();}
 Error actionError(const Endpoint&e,Action a,uint8_t target)const{
  if(!playable(e))return e.paused?Error::Paused:Error::Busy;
  if(e.state.accepted[e.local]==UINT32_MAX)return Error::Limit;
  Command c{e.state.accepted[e.local]+1,e.state.revision,e.state.turn,target,a};
  return legal(e.state,e.local,c); // Same validator used by the host, no copied rules/formulas.
 }
 bool eligible(const Endpoint&e,uint8_t slot)const{
  return mode==Mode::Target&&slot>=kPlayerCapacity&&slot<kActors&&
   actionError(e,selected,slot)==Error::None;
 }
 Rect targetRect(const Endpoint&e,uint8_t slot)const{if(slot<kPlayerCapacity||slot>=kActors)return {0,0,0,0};return e.state.wave==2?(slot==3?kBossHit:Rect{0,0,0,0}):kEnemyHits[slot-kPlayerCapacity];}
 Hit hitTest(const Endpoint&e,int32_t x,int32_t y)const{
  if(x<0||x>=kWidth||y<0||y>=kHeight||mode==Mode::Pending||!playable(e))return {};
  for(uint8_t b=0;b<4;++b)if(kButtons[b].contains(x,y)){
   if(mode==Mode::Action)return {HitKind::Button,b};
   if(b==3)return {HitKind::Button,b}; // Cancel only exists before a command is queued.
   if(mode==Mode::Ability&&b==0&&firstLegal(e,Action::Focus)!=kNoActor)return {HitKind::Button,b};
   if(mode==Mode::Items&&b<2&&actionError(e,b==0?Action::Life:Action::Mana,e.local)==Error::None)return {HitKind::Button,b};
   return {};
  }
  if(mode==Mode::Target)for(uint8_t s=kPlayerCapacity;s<kActors;++s)if(eligible(e,s)&&targetRect(e,s).contains(x,y))return {HitKind::Enemy,s};
  return {};
 }
 // 'pressed' is the current single-contact level, not an edge. A complete down/up
 // in the same zone is required. Repeated samples, dragging and early release do not fire.
 void onTouch(Endpoint&e,bool pressed,int32_t x,int32_t y,uint32_t now){
  sync(e);
  if(pressed){if(!down){down=true;armed=hitTest(e,x,y);}else if(!(armed==hitTest(e,x,y)))armed={};return;}
  if(!down)return;
  down=false;const Hit start=armed;armed={};const Hit end=hitTest(e,x,y);
  if(start.kind!=HitKind::None&&start==end)activate(e,end,now);
 }
 bool contactDown()const{return down;}
private:
 bool observed=false,seenHasState=false,down=false;
 uint64_t seenNonce=0;uint32_t seenRevision=0,seenHash=0,submittedSequence=0;uint8_t submittedPeer=kNoPeer;Hit armed{};
 uint8_t firstLegal(const Endpoint&e,Action a)const{for(uint8_t s=kPlayerCapacity;s<kActors;++s)if(actionError(e,a,s)==Error::None)return s;return kNoActor;}
 void unavailable(Error why){error=why;notice=Notice::Unavailable;}
 void selectTarget(const Endpoint&e,Action a){
  if(firstLegal(e,a)==kNoActor){unavailable(actionError(e,a,firstEnemy(e.state)));return;}
  selected=a;mode=Mode::Target;error=Error::None;notice=Notice::ChooseTarget;
 }
 void submit(Endpoint&e,Action a,uint8_t target,uint32_t now){
  Error legalNow=actionError(e,a,target);if(legalNow!=Error::None){unavailable(legalNow);return;}
  submittedSequence=e.state.accepted[e.local]+1;submittedPeer=e.local;Error result=e.submit(a,target,now);error=result;
  if(result==Error::None){selected=a;mode=Mode::Pending;notice=Notice::Pending;++submitted;}
  else{mode=Mode::Action;notice=Notice::Rejected;}
 }
 void activate(Endpoint&e,const Hit&h,uint32_t now){
  if(h.kind==HitKind::Enemy){if(eligible(e,h.index))submit(e,selected,h.index,now);return;}
  if(h.kind!=HitKind::Button)return;
  const uint8_t b=h.index;
  if(mode!=Mode::Action&&b==3){mode=Mode::Action;notice=Notice::Cancelled;error=Error::None;selected=Action::Attack;++cancelled;return;}
  if(mode==Mode::Action){
   if(b==0)selectTarget(e,Action::Attack);
   else if(b==1){if(firstLegal(e,Action::Focus)==kNoActor)unavailable(actionError(e,Action::Focus,firstEnemy(e.state)));else{mode=Mode::Ability;notice=Notice::ExperimentalFocus;error=Error::None;}}
   else if(b==2)submit(e,Action::Guard,e.local,now);
   else if(b==3){if(actionError(e,Action::Life,e.local)!=Error::None&&actionError(e,Action::Mana,e.local)!=Error::None)unavailable(Error::Resource);else{mode=Mode::Items;notice=Notice::Items;error=Error::None;}}
  }else if(mode==Mode::Ability&&b==0)selectTarget(e,Action::Focus);
  else if(mode==Mode::Items&&b<2)submit(e,b==0?Action::Life:Action::Mana,e.local,now);
 }
};
static_assert(sizeof(TouchUi)<=80,"Keep UI state bounded; excludes existing Endpoint and renderer");
}
