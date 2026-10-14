#include "pvp_save30_fixture.h"
static void beginFight(){linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);assert(game.clubStage==2);}
static club::Packet surrender(){auto p=packet(club::Type::Move,200,2);p.reserved[2]=3;return p;}
static void verifyFormat(){for(auto& entry:nvs.blobs)if(entry.second.size()==PERSISTENT_SAVE_SIZE){assert(get16(entry.second.data(),4)==30&&get32(entry.second.data(),144)==1);}}
int main(){
  static_assert(CURRENT_SAVE_FORMAT==30&&SAVE_RESET_EPOCH==1,"review must not migrate saves");
  for(auto failure:{Failure::PrimaryWriteBefore,Failure::PrimaryWriteAfter,Failure::PrimaryReadback})for(bool reboot:{false,true}){
    beginFight();auto before=game;nvs.failure=failure;enqueue(surrender());tickClub(now);
    assert(arena.duel.terminalConfirmed&&game.clubStage==3&&game.p.gold==120);
    // PersistentSlotBackend verifies the written bytes and tolerates a false
    // write acknowledgement when its immediate readback proves the commit.
    if(failure==Failure::PrimaryWriteAfter)assert(view.page==Page::ClubResult);
    else assert(view.page==Page::SaveError);
    assert(game.p.hp==before.p.hp&&game.p.mp==before.p.mp&&game.p.xp==before.p.xp&&game.fatigue==before.fatigue);
    if(reboot){recoverReboot();assert(game.p.gold==(failure==Failure::PrimaryWriteBefore?75:120));}
    else {if(view.page==Page::SaveError){auto writes=nvs.primaryWrites;enqueue(surrender());tickClub(now+40000);assert(nvs.primaryWrites==writes);savedTransition(afterSave);}tickClub(now);assert(game.p.gold==120);recoverReboot();assert(game.p.gold==120);}
    auto settled=game;auto writes=nvs.primaryWrites;recoverClubReservation();recoverReboot();assert(same(game,settled)&&nvs.primaryWrites==writes);verifyFormat();
  }
  std::puts("PASS: real ClubRadio + production PersistentJournal Save30/epoch1: failed write, false write ACK, failed readback, retry/reboot; no repeated reward or campaign mutation");
  beginFight();nvs.operations=0;enqueue(surrender());tickClub(now);auto boundaries=nvs.operations;unsigned count=0,lost=0,won=0;
  for(unsigned point=1;point<=boundaries;++point)for(bool after:{false,true}){
    beginFight();nvs.operations=0;nvs.cut=point;nvs.cutAfter=after;
    try{enqueue(surrender());tickClub(now);}catch(const PowerCut&){}
    nvs.cut=0;auto durable=rebootSnapshot();assert((durable.p.gold==75&&durable.clubStage==2)||(durable.p.gold==120&&durable.clubStage==3));
    recoverReboot();assert(game.clubStage==3&&(game.p.gold==75||game.p.gold==120));if(game.p.gold==75)++lost;else ++won;
    auto settled=game;auto writes=nvs.primaryWrites;recoverClubReservation();recoverReboot();assert(same(game,settled)&&nvs.primaryWrites==writes);verifyFormat();++count;
  }
  std::printf("PASS: %u production-journal terminal-settlement simulated cut boundaries: %u conservative losses, %u persisted wins; at-most-once credit, no reconciliation claim\n",count,lost,won);
  return 0;
}
