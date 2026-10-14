// Independent local-only UI review regressions. Does not modify frozen engine.
#include "CoopTouchUi.h"
#include <cstdio>
#include <cstdlib>
using namespace coop;
using namespace coopui;
static unsigned checks=0,failures=0,cases=0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); } } while(0)
static State fixture(uint8_t turn=0) {
 auto a=rpg::create(0,111),b=rpg::create(2,222); State s{};
 if(!initialize(s,a,b,123))std::abort();
 s.turn=turn; return s;
}
static void clean(Endpoint&e) { Packet p{};while(e.outgoing.pop(p)){};UiEvent u{};while(e.ui.pop(u)){}; }
static void relay(Endpoint&a,Endpoint&b,uint32_t now) { Packet p{};while(a.outgoing.pop(p))b.receive(a.local,p,now); }
static void exchange(Endpoint&h,Endpoint&c,uint32_t now=0) { for(int i=0;i<4;++i){relay(h,c,now);relay(c,h,now);}CHECK(!h.awaitingAck&&!h.paused&&!c.paused&&stateHash(h.state)==stateHash(c.state)); }
static void setup(Endpoint&h,Endpoint&c,State s=fixture()) { h.hostStart(s,0x12345678,0);c.clientStart(h.nonce,0);exchange(h,c);clean(h);clean(c); }
static void tap(TouchUi&u,Endpoint&e,int x,int y,uint32_t now=1) {u.onTouch(e,true,x,y,now);u.onTouch(e,false,x,y,now+1);}
static void button(TouchUi&u,Endpoint&e,unsigned b,uint32_t now=1) {const auto&r=kButtons[b];tap(u,e,r.x+r.w/2,r.y+r.h/2,now);}
static void target(TouchUi&u,Endpoint&e,unsigned slot,uint32_t now=1) {auto r=u.targetRect(e,uint8_t(slot));tap(u,e,r.x+r.w/2,r.y+r.h/2,now);}
static Packet command(const Endpoint&e,const Command&c) {Packet p{};auto w=begin(p,Type::Command,1,e.nonce,c.sequence);writeCommand(w,c);if(!finish(p,w))std::abort();return p;}
static void report(const char*name,unsigned start) {++cases;std::printf("%s %s\n",failures==start?"PASS":"FAIL",name);}
static void dragOutAndBackAction() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;
 auto hash=stateHash(h.state);u.onTouch(h,true,40,214,1);u.onTouch(h,true,100,80,2);u.onTouch(h,true,40,214,3);u.onTouch(h,false,40,214,4);
 CHECK(u.mode==Mode::Action&&u.submitted==0&&stateHash(h.state)==hash);
 report("drag_out_and_back_action_does_not_activate",start);
}
static void dragOutAndBackTarget() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;button(u,h,0);auto hash=stateHash(h.state);
 u.onTouch(h,true,215,70,3);u.onTouch(h,true,283,96,4);u.onTouch(h,true,215,70,5);u.onTouch(h,false,215,70,6);
 CHECK(u.mode==Mode::Target&&u.submitted==0&&stateHash(h.state)==hash);
 report("drag_between_targets_and_back_does_not_submit",start);
}
static void dragOutsideAndBack() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;
 u.onTouch(h,true,40,214,1);u.onTouch(h,true,INT32_MIN,INT32_MAX,2);u.onTouch(h,false,40,214,3);
 CHECK(u.mode==Mode::Action&&u.submitted==0);
 report("contact_leaves_screen_and_returns_does_not_activate",start);
}
static void jitterInsideSameZone() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;
 u.onTouch(h,true,40,214,1);u.onTouch(h,true,42,215,2);u.onTouch(h,true,39,212,3);u.onTouch(h,false,40,214,4);
 CHECK(u.mode==Mode::Target&&u.submitted==0);
 report("minor_motion_inside_same_hit_zone_allows_one_tap",start);
}
static void successNotOverriddenByUnrelatedReject() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;
 button(u,h,0);target(u,h,4,5);CHECK(h.state.accepted[0]==1&&u.mode==Mode::Pending&&h.awaitingAck);
 const auto hash=stateHash(h.state);
 h.receive(1,command(h,{1,h.state.revision,1,3,Action::Attack}),8);
 CHECK(h.lastError==Error::Busy&&stateHash(h.state)==hash);
 exchange(h,c,10);u.sync(h);
 CHECK(h.state.accepted[0]==1&&u.mode==Mode::Action&&u.notice==Notice::Confirmed&&u.error==Error::None);
 report("own_accepted_action_is_confirmed_despite_unrelated_peer_reject",start);
}
static void heldAcrossStateChanges() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;button(u,h,0);
 u.onTouch(h,true,215,70,1);h.state.actors[3].hp=0;++h.state.revision;u.sync(h);
 u.onTouch(h,true,40,214,2);u.onTouch(h,false,40,214,3);CHECK(u.mode==Mode::Action&&u.submitted==0&&u.staleCleared==1);
 button(u,h,0,4);CHECK(u.mode==Mode::Target&&!u.eligible(h,3));
 report("held_touch_does_not_rearm_after_state_change",start);
}
static void unchangedRevisionChangedHash() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;button(u,h,0);u.onTouch(h,true,215,70,1);
 --h.state.actors[3].hp;CHECK(validState(h.state));u.onTouch(h,false,215,70,2);
 CHECK(u.mode==Mode::Action&&u.submitted==0&&u.staleCleared==1);
 report("same_revision_different_state_clears_gesture",start);
}
static void pausedResumeHeld() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;button(u,h,0);u.onTouch(h,true,215,70,1);
 h.paused=true;u.sync(h);h.paused=false;u.onTouch(h,true,40,214,2);u.onTouch(h,false,40,214,3);
 CHECK(u.mode==Mode::Action&&u.submitted==0);button(u,h,0,4);CHECK(u.mode==Mode::Target);
 report("pause_resume_requires_fresh_complete_tap",start);
}
static void pendingPressDoesNotReplayOnAck() {
 const auto start=failures;Endpoint h,c;setup(h,c,fixture(1));TouchUi u;button(u,c,0);target(u,c,4,3);CHECK(c.pending);
 u.onTouch(c,true,40,214,5);exchange(h,c,6);u.sync(c);u.onTouch(c,false,40,214,7);
 CHECK(u.mode==Mode::Action&&u.submitted==1&&c.state.accepted[1]==1&&!u.contactDown());
 report("pending_press_cannot_become_post_ack_action",start);
}
static void unchangedSnapshotKeepsOriginalIntent() {
 const auto start=failures;Endpoint h,c;setup(h,c,fixture(1));TouchUi u;button(u,c,0);target(u,c,4,3);
 Packet original{};CHECK(c.outgoing.pop(original));auto hash=stateHash(c.state);c.paused=true;c.tick(600);relay(c,h,601);relay(h,c,602);u.sync(c);
 CHECK(c.pending&&u.mode==Mode::Pending&&stateHash(c.state)==hash);button(u,c,3,603);target(u,c,4,605);CHECK(c.pending&&u.submitted==1);
 c.tick(900);exchange(h,c,901);u.sync(c);CHECK(c.state.accepted[1]==1&&u.notice==Notice::Confirmed&&u.submitted==1);
 report("same_revision_recovery_preserves_original_pending_intent",start);
}
static void invalidStateNeverHits() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;button(u,h,0);
 h.state.turn=255;u.sync(h);CHECK(!u.playable(h)&&u.mode==Mode::Action);
 for(int x=-2;x<323;++x)for(int y=-2;y<243;y+=17){CHECK(u.hitTest(h,x,y).kind==HitKind::None);u.onTouch(h,true,x,y,1);u.onTouch(h,false,x,y,2);}
 CHECK(u.submitted==0);h.state=fixture();h.local=255;u.sync(h);CHECK(!u.playable(h));
 report("invalid_cursor_or_local_peer_does_not_access_invalid_actor",start);
}
static void boundaryTouchesRequireSameHit() {
 const auto start=failures;Endpoint h,c;setup(h,c);TouchUi u;
 u.onTouch(h,true,4,199,1);u.onTouch(h,false,79,199,2);CHECK(u.mode==Mode::Action);
 u.onTouch(h,true,78,228,3);u.onTouch(h,false,4,199,4);CHECK(u.mode==Mode::Target);
 u.onTouch(h,true,185,45,5);u.onTouch(h,false,246,45,6);CHECK(u.submitted==0);
 report("half_open_hit_edges_are_consistent",start);
}
static void sequenceAndResourcesGateSelection() {
 const auto start=failures;Endpoint h,c;auto s=fixture();s.accepted[0]=UINT32_MAX;s.revision=UINT32_MAX;setup(h,c,s);TouchUi u;
 button(u,h,0);CHECK(u.mode==Mode::Action&&u.error==Error::Limit&&u.submitted==0);
 s=fixture();s.actors[0].mp=1;setup(h,c,s);u=TouchUi{};button(u,h,1);CHECK(u.mode==Mode::Action&&u.error==Error::Resource);
 s=fixture(1);setup(h,c,s);u=TouchUi{};for(unsigned b=0;b<4;++b)button(u,h,b);CHECK(u.mode==Mode::Action&&u.submitted==0);
 report("limits_resources_and_other_turn_block_actions",start);
}
static void unacceptedNewRevisionIsRejected() {
 const auto start=failures;Endpoint h,c;setup(h,c,fixture(1));TouchUi u;button(u,c,0);target(u,c,4,3);CHECK(c.pending);
 ++h.state.revision;relay(c,h,5);relay(h,c,6);u.sync(c);
 CHECK(!c.pending&&u.mode==Mode::Action&&u.notice==Notice::Rejected&&u.error==Error::Revision&&c.state.accepted[1]==0);
 exchange(h,c,7);u.sync(c);CHECK(u.notice!=Notice::Confirmed&&u.submitted==1&&c.state.accepted[1]==0);
 report("newer_revision_without_own_sequence_acceptance_is_not_confirmation",start);
}
static void changedSessionDiscardsPendingPresentation() {
 const auto start=failures;Endpoint h,c;setup(h,c,fixture(1));TouchUi u;button(u,c,0);target(u,c,4,3);CHECK(c.pending);
 c.clientStart(h.nonce+1,5);u.sync(c);
 CHECK(u.mode==Mode::Action&&u.notice==Notice::Updated&&!u.playable(c)&&u.staleCleared==1);
 report("new_nonce_does_not_confirm_old_pending_action",start);
}
static void sampledGestureMatrix() {
 const auto start=failures;
 struct Point {int x,y;};
 constexpr Point points[]={{40,214},{120,214},{200,214},{278,214},{215,70},{283,96},{215,120},{0,0},{-1,10},{320,240}};
 for(unsigned initialMode=0;initialMode<2;++initialMode)for(const auto&a:points)for(const auto&b:points)for(const auto&d:points){
  Endpoint h,c;setup(h,c);TouchUi u;if(initialMode)button(u,h,0);
  const auto ha=u.hitTest(h,a.x,a.y),hb=u.hitTest(h,b.x,b.y),hd=u.hitTest(h,d.x,d.y);
  const bool activate=ha.kind!=HitKind::None&&ha==hb&&ha==hd;
  const auto originalHash=stateHash(h.state);const auto originalMode=u.mode;
  u.onTouch(h,true,a.x,a.y,10);u.onTouch(h,true,b.x,b.y,11);u.onTouch(h,false,d.x,d.y,12);
  if(!activate){CHECK(u.mode==originalMode&&stateHash(h.state)==originalHash&&u.submitted==0&&u.cancelled==0);}
  else if(initialMode&&ha.kind==HitKind::Enemy){CHECK(u.mode==Mode::Pending&&h.state.accepted[0]==1&&u.submitted==1);}
  else if(initialMode){CHECK(u.mode==Mode::Action&&u.cancelled==1&&stateHash(h.state)==originalHash);}
  else if(ha.index==0){CHECK(u.mode==Mode::Target&&u.submitted==0);}
  else if(ha.index==1){CHECK(u.mode==Mode::Ability&&u.submitted==0);}
  else if(ha.index==2){CHECK(u.mode==Mode::Pending&&h.state.accepted[0]==1&&u.submitted==1);}
  else{CHECK(u.mode==Mode::Action&&u.submitted==0);}
  CHECK(!u.contactDown());
 }
 report("2000_sampled_start_move_release_gesture_sequences",start);
}
int main() {
 dragOutAndBackAction();dragOutAndBackTarget();dragOutsideAndBack();jitterInsideSameZone();successNotOverriddenByUnrelatedReject();heldAcrossStateChanges();unchangedRevisionChangedHash();pausedResumeHeld();pendingPressDoesNotReplayOnAck();unchangedSnapshotKeepsOriginalIntent();invalidStateNeverHits();boundaryTouchesRequireSameHit();sequenceAndResourcesGateSelection();unacceptedNewRevisionIsRejected();changedSessionDiscardsPendingPresentation();sampledGestureMatrix();
 std::printf("REVIEW cases=%u checks=%u failures=%u sizeof_TouchUi=%zu\n",cases,checks,failures,sizeof(TouchUi));return failures?1:0;
}
