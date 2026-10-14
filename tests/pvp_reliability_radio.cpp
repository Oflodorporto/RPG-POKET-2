// Includes the real ClubRadio adapter and repository fake ESP-NOW/NVS backend.
#define main original_radio_main
#include "fatigue76_club_radio.cpp"
#undef main

static void keepLinked(unsigned elapsed) {
  for(unsigned i=0;i<elapsed;i+=750) {now+=750;enqueue(packet(club::Type::Confirm));tickClub(now);}
}
int main(){
  linkedAcceptor();auto before=game;
  keepLinked(300000);
  assert(arena.duel.ready&&!arena.duel.failed&&view.page==Page::ClubBattle);
  assert(arena.duel.turn!=arena.duel.local&&game.clubStage==1);
  clubTap(150,285); // Rendered Desistir control.
  assert(view.page==Page::ClubBattle&&game.clubStage==2&&arena.duel.surrenderRequested);
  auto ack=packet(club::Type::MoveAck,200,1);ack.reserved[2]=3;enqueue(ack);tickClub(now);
  assert(view.page==Page::ClubResult&&arena.result==-1&&!arena.resultUnconfirmed&&game.p.gold==75);
  assert(game.p.hp==before.p.hp&&game.p.mp==before.p.mp&&game.p.xp==before.p.xp&&game.fatigue==before.fatigue);
  std::puts("PASS: real ClubRadio voluntary surrender leaves idle remote turn; acknowledged loss preserves campaign resources");

  // Pre-start and post-start connection loss have different accounting.
  linkedAcceptor();auto hp=game.p.hp;auto mp=game.p.mp;auto xp=game.p.xp;auto fatigue=game.fatigue;
  now+=6000;tickClub(now);
  assert(view.page==Page::ClubResult&&arena.result==2&&game.p.gold==100&&game.clubStage==3);
  assert(game.p.hp==hp&&game.p.mp==mp&&game.p.xp==xp&&game.fatigue==fatigue);
  std::puts("PASS: pre-action link loss refunds 25g, shows cancellation, keeps campaign HP/MP/XP/fatigue");

  linkedAcceptor();hp=game.p.hp;mp=game.p.mp;xp=game.p.xp;fatigue=game.fatigue;
  enqueue(packet(club::Type::Move,200,1));tickClub(now);
  assert(game.clubStage==2&&arena.duel.revision==1);
  now+=6000;tickClub(now);
  assert(view.page==Page::ClubResult&&arena.result==-1&&game.p.gold==75&&game.clubStage==3);
  assert(game.p.hp==hp&&game.p.mp==mp&&game.p.xp==xp&&game.fatigue==fatigue);
  auto settled=game;auto writes=nvs.primaryWrites;tickClub(now);assert(same(game,settled)&&nvs.primaryWrites==writes);
  std::puts("PASS: post-action link loss consumes 25g, settles once, leaves campaign resources intact");

  // A final command ACK cannot be withheld forever without changing result.
  linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);
  assert(arena.duel.turn==arena.duel.local);
  arena.duel.fighters[0].hp=1; // Minimal deterministic terminal-state fixture.
  clubTap(30,230);assert(arena.duel.finished&&arena.duel.waiting&&arena.duel.winner==arena.duel.local);
  keepLinked(10500); // Confirm/Ready heartbeats succeed; no MoveAck is delivered.
  assert(view.page==Page::ClubResult&&arena.result==-1&&arena.resultUnconfirmed&&game.p.gold==75);
  std::puts("PASS: no terminal ACK/receipt gives explicitly unconfirmed result, with no fabricated reward");

  // A valid terminal ACK and later Cancel in the same drained batch also lose.
  linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);
  arena.duel.fighters[0].hp=1;clubTap(30,230);
  assert(arena.duel.finished&&arena.duel.waiting&&arena.duel.winner==arena.duel.local);
  now+=11000;
  enqueue(packet(club::Type::MoveAck,200,2));enqueue(packet(club::Type::Cancel));tickClub(now);
  assert(!arena.duel.waiting&&arena.duel.finished&&!arena.duel.failed);
  assert(view.page==Page::ClubResult&&arena.result==1&&!arena.resultUnconfirmed&&game.p.gold==120);
  auto terminal=game;writes=nvs.primaryWrites;enqueue(packet(club::Type::MoveAck,200,2));tickClub(now);
  assert(same(game,terminal)&&nvs.primaryWrites==writes);
  std::puts("PASS: terminal ACK followed by Cancel in one batch pays the confirmed win exactly once");

  // Invalid turn/sequence and duplicate command do not start or debit twice.
  linkedAcceptor();writes=nvs.primaryWrites;
  enqueue(packet(club::Type::Move,200,2));tickClub(now);
  assert(game.clubStage==1&&arena.duel.revision==0&&nvs.primaryWrites==writes);
  auto first=packet(club::Type::Move,200,1);enqueue(first);tickClub(now);writes=nvs.primaryWrites;
  const auto health=arena.duel.fighters[1].hp;
  enqueue(first);tickClub(now);assert(nvs.primaryWrites==writes&&arena.duel.revision==1&&arena.duel.fighters[1].hp==health&&game.p.gold==75);
  std::puts("PASS: future-sequence packet does not commit stake; repeated command does not repeat damage or persist again");

  // Result back button intentionally keeps radio open for final retransmission.
  enqueue(packet(club::Type::Cancel));tickClub(now);assert(view.page==Page::ClubResult);
  clubTap(30,285);assert(view.page==Page::ClubResult&&arena.opened);
  now+=11000;clubTap(30,285);assert(view.page==Page::Guild&&!arena.opened);
  assert(openClub());view.page=Page::Club;assert(!arena.begun&&arena.session.state==club::State::Listing);
  std::puts("PASS: result return blocked for 11s; leaving and reopening resets duel and discovery for rematch");

  // A receipt followed by Cancel has the same immutable terminal ordering.
  linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);
  arena.duel.fighters[0].hp=1;clubTap(30,230);assert(!arena.duel.terminalConfirmed);
  auto receipt=packet(club::Type::Result,200,2);receipt.reserved[2]=2;receipt.resultDigest=arena.duel.stateDigest();
  enqueue(receipt);enqueue(packet(club::Type::Cancel));tickClub(now);
  assert(view.page==Page::ClubResult&&arena.result==1&&game.p.gold==120&&arena.duel.terminalConfirmed);
  terminal=game;writes=nvs.primaryWrites;tickClub(now);assert(same(game,terminal)&&nvs.primaryWrites==writes);
  // A closing packet overtaking proof must not change an acknowledged outcome.
  linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);
  arena.duel.fighters[0].hp=1;clubTap(30,230);
  enqueue(packet(club::Type::Cancel));enqueue(packet(club::Type::MoveAck,200,2));tickClub(now);
  assert(!arena.resultUnconfirmed&&arena.result==1&&game.p.gold==120&&arena.duel.terminalConfirmed);
  // A proof in a later batch is accepted within the ORIGINAL command deadline.
  linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);
  arena.duel.fighters[0].hp=1;clubTap(30,230);const auto commandAt=now;
  enqueue(packet(club::Type::Cancel));tickClub(now);
  assert(view.page==Page::ClubBattle&&game.clubStage==2&&!arena.duel.terminalConfirmed);
  now+=400;receipt=packet(club::Type::Result,200,2);receipt.reserved[2]=2;receipt.resultDigest=arena.duel.stateDigest();
  enqueue(receipt);tickClub(now);assert(arena.result==1&&game.p.gold==120&&arena.duel.terminalConfirmed);
  // A complete response partition still expires, even when Cancel arrives late.
  linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);
  arena.duel.fighters[0].hp=1;clubTap(30,230);const auto pendingAt=now;
  now+=9000;enqueue(packet(club::Type::Cancel));tickClub(now);
  assert(view.page==Page::ClubBattle&&!arena.duel.terminalConfirmed);
  now=pendingAt+10000;tickClub(now);
  assert(view.page==Page::ClubResult&&arena.resultUnconfirmed&&game.p.gold==75&&arena.duel.failed);
  terminal=game;writes=nvs.primaryWrites;enqueue(packet(club::Type::MoveAck,200,2));tickClub(now);
  assert(same(game,terminal)&&nvs.primaryWrites==writes&&!arena.duel.terminalConfirmed);
  (void)commandAt;
  // A valid remote surrender is accepted even while it is our turn.
  linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);
  auto surrender=packet(club::Type::Move,200,2);surrender.reserved[2]=3;
  enqueue(surrender);tickClub(now);assert(arena.duel.winner==arena.duel.local&&arena.result==1&&game.p.gold==120);
  // A first-action surrender still requires a durable start before any ACK.
  linkedAcceptor();nvs.failure=Failure::PrimaryWriteAfter;
  surrender=packet(club::Type::Move,200,1);surrender.reserved[2]=3;enqueue(surrender);tickClub(now);
  assert(view.page==Page::SaveError&&arena.duel.revision==0&&!arena.duel.terminalConfirmed);
  writes=nvs.primaryWrites;tickClub(now+1000);assert(nvs.primaryWrites==writes);
  savedTransition(afterSave);enqueue(surrender);tickClub(now);
  assert(arena.result==1&&game.p.gold==120&&arena.duel.revision==1&&arena.duel.terminalConfirmed);
  terminal=game;writes=nvs.primaryWrites;enqueue(surrender);tickClub(now);
  assert(same(game,terminal)&&nvs.primaryWrites==writes);
  std::puts("PASS: receipt/Cancel ordering, no predicted reward and remote out-of-turn surrender");
}
