// Real adapter closure ordering and boundaries. HP/revision changes below are
// explicitly minimal terminal fixtures, not claims of naturally played games.
#define main original_radio_main
#include "fatigue76_club_radio.cpp"
#undef main
static club::Packet pendingResult(){
 auto candidate=arena.duel;candidate.setResultSender(nullptr);
 if(candidate.surrenderRequested)candidate.receive(candidate.revision+1,uint8_t(club::Action::Surrender),true,now);
 auto p=packet(club::Type::Result,200,candidate.revision);p.reserved[2]=uint8_t(candidate.winner+1);p.resultDigest=candidate.stateDigest();return p;
}
static void lethal(){
 linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);
 arena.duel.fighters[0].hp=1;clubTap(30,230);assert(arena.duel.finished&&arena.duel.waiting&&!arena.duel.terminalConfirmed);
}
int main(){
 for(bool receipt:{false,true}){
  linkedAcceptor();clubTap(150,285);assert(arena.duel.surrenderRequested&&!arena.duel.finished);
  auto proof=receipt?pendingResult():packet(club::Type::MoveAck,200,1);if(!receipt)proof.reserved[2]=3;
  enqueue(packet(club::Type::Cancel));enqueue(proof);tickClub(now);
  assert(arena.result==-1&&!arena.resultUnconfirmed&&arena.duel.terminalConfirmed&&game.p.gold==75&&game.clubStage==3);
 }
 linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);arena.duel.revision=199;
 clubTap(30,280);assert(arena.duel.finished&&arena.duel.winner==-1&&arena.duel.waiting);
 auto proof=pendingResult();enqueue(packet(club::Type::Cancel));enqueue(proof);tickClub(now);
 assert(arena.result==0&&!arena.resultUnconfirmed&&game.p.gold==100&&arena.duel.terminalConfirmed);
 // Link expiry can overtake a proof too; the original command deadline remains.
 lethal();proof=pendingResult();now+=6000;tickClub(now);
 assert(arena.session.notice==club::Notice::Lost&&view.page==Page::ClubBattle&&game.clubStage==2);
 now+=400;enqueue(proof);tickClub(now);assert(arena.result==1&&game.p.gold==120);
 // Invalid proofs, new commands and invitations cannot reopen a closing duel.
 lethal();proof=pendingResult();enqueue(packet(club::Type::Cancel));tickClub(now);auto writes=nvs.primaryWrites;
 const auto revision=arena.duel.revision;auto bad=proof;bad.resultDigest^=1;enqueue(bad);
 bad=proof;bad.classId=2;enqueue(bad);bad=proof;bad.session=bad.stakeSession=201;enqueue(bad);
 enqueue(packet(club::Type::Move,200,revision+1));enqueue(packet(club::Type::Invite,201));tickClub(now);
 assert(!arena.duel.terminalConfirmed&&arena.duel.revision==revision&&game.clubStage==2&&nvs.primaryWrites==writes);
 enqueue(proof);tickClub(now);assert(arena.result==1&&game.p.gold==120);
 // Receive batch is processed before timers, matching existing adapter semantics.
 lethal();now+=10000;enqueue(packet(club::Type::Cancel));enqueue(packet(club::Type::MoveAck,200,2));tickClub(now);
 assert(arena.result==1&&arena.duel.terminalConfirmed);
 lethal();now+=10000;tickClub(now);assert(arena.resultUnconfirmed&&arena.duel.failed);
 enqueue(packet(club::Type::MoveAck,200,2));tickClub(now);assert(game.p.gold==75&&!arena.duel.terminalConfirmed);
 for(bool proofArrives:{false,true}){
  linkedAcceptor();now=UINT32_MAX-200;enqueue(packet(club::Type::Confirm));enqueue(packet(club::Type::Move,200,1));tickClub(now);
  arena.duel.fighters[0].hp=1;clubTap(30,230);const auto pending=now;proof=pendingResult();
  now+=500;enqueue(packet(club::Type::Cancel));tickClub(now);assert(view.page==Page::ClubBattle);
  if(proofArrives){now=pending+9999;enqueue(proof);tickClub(now);assert(arena.result==1&&game.p.gold==120);}
  else {now=pending+10000;tickClub(now);assert(arena.resultUnconfirmed&&game.p.gold==75&&arena.duel.failed);}
 }
 std::puts("PASS: closing surrender/draw/loss, proof-only admission, original deadline and uint32 wrap; no reopening or post-settlement reward");
}
