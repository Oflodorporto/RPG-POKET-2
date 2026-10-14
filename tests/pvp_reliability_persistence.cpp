// Regression coverage contributed by the independent candidate reviewer.
// Real callback lengths, terminal save failures and actual power-cut boundaries.
#define main supplied_radio_main
#include "fatigue76_club_radio.cpp"
#undef main

static void beginFight(){ linkedAcceptor();enqueue(packet(club::Type::Move,200,1));tickClub(now);assert(game.clubStage==2&&arena.duel.revision==1); }
static club::Packet surrender(){auto p=packet(club::Type::Move,200,2);p.reserved[2]=3;return p;}
static void receiveBytes(club::Packet p,int size){esp_now_recv_info_t info{remoteMac};clubReceive(&info,reinterpret_cast<const uint8_t*>(&p),size);}
int main(){
  // Exercise the actual byte callback, not merely Session::receive with a v4 flag.
  reset();auto hello=packet(club::Type::Hello,0);hello.version=4;hello.stakeSession=0;
  receiveBytes(hello,44);tickClub(now);assert(arena.session.peers[0].used&&arena.session.peers[0].version==4);
  auto writes=nvs.primaryWrites;assert(!arena.session.invite(0,201,now));assert(nvs.primaryWrites==writes&&game.clubStage==0);
  for(int length:{0,1,8,12,43,45,46,47,48,49,50,51,100}){auto count=clubQueue->items.size();receiveBytes(hello,length);assert(clubQueue->items.size()==count);}
  receiveBytes(hello,sizeof(club::Packet));assert(clubQueue->items.empty());
  hello.version=club::VERSION;receiveBytes(hello,44);assert(clubQueue->items.empty());
  receiveBytes(hello,sizeof(club::Packet));tickClub(now);assert(arena.session.peers[0].version==club::VERSION);
  auto oldInvite=packet(club::Type::Invite);oldInvite.version=4;receiveBytes(oldInvite,44);tickClub(now);assert(arena.session.state==club::State::Listing&&game.clubStage==0);
  std::puts("PASS: real callback length/version matrix, zero-extended legacy Hello, no old-version admission or debit");

  // Surrender is a real terminal command, with no artificial HP fixture.
  for(auto failure:{Failure::PrimaryWriteBefore,Failure::PrimaryWriteAfter,Failure::PrimaryReadback}){
    beginFight();auto campaign=game;nvs.failure=failure;enqueue(surrender());tickClub(now);
    assert(view.page==Page::SaveError&&arena.duel.terminalConfirmed&&game.p.gold==120&&game.clubStage==3);
    assert(game.p.hp==campaign.p.hp&&game.p.mp==campaign.p.mp&&game.p.xp==campaign.p.xp&&game.fatigue==campaign.fatigue);
    auto committed=rebootSnapshot();assert(committed.p.gold==(failure==Failure::PrimaryWriteBefore?75:120));
    auto pending=game;writes=nvs.primaryWrites;enqueue(surrender());enqueue(packet(club::Type::Cancel));tickClub(now+40000);
    assert(nvs.primaryWrites==writes&&same(game,pending)&&clubQueue->items.size()==2);
    savedTransition(afterSave);now+=40000;tickClub(now);
    assert(view.page==Page::ClubResult&&arena.result==1&&game.p.gold==120&&game.clubStage==3);
    writes=nvs.primaryWrites;enqueue(surrender());tickClub(now);assert(nvs.primaryWrites==writes&&game.p.gold==120);
    recoverReboot();assert(game.p.gold==120&&game.clubStage==3);recoverReboot();assert(game.p.gold==120);
  }
  std::puts("PASS: terminal payout before-write/after-write/readback failures, queue freeze, retry and reboot award once");

  // Enumerate actual terminal settlement persistence boundaries (separate from
  // supplied 48 admission-compensation cuts). Reboot intentionally cannot
  // reconstruct a terminal win that never became durable in Save30.
  beginFight();nvs.operations=0;enqueue(surrender());tickClub(now);unsigned boundaries=nvs.operations,cuts=0,lost=0,won=0;
  for(unsigned point=1;point<=boundaries;++point)for(bool after:{false,true}){
    beginFight();nvs.operations=0;nvs.cut=point;nvs.cutAfter=after;
    try{enqueue(surrender());tickClub(now);}catch(const PowerCut&){}
    nvs.cut=0;auto durable=rebootSnapshot();assert(durable.p.gold==75||durable.p.gold==120);
    assert((durable.clubStage==2&&durable.p.gold==75)||(durable.clubStage==3&&durable.p.gold==120));
    recoverReboot();assert(game.clubStage==3&&(game.p.gold==75||game.p.gold==120));
    if(game.p.gold==75)++lost;else ++won;
    auto settled=game;writes=nvs.primaryWrites;recoverClubReservation();recoverReboot();assert(game.p.gold==settled.p.gold&&nvs.primaryWrites==writes);++cuts;
  }
  std::printf("PASS: %u terminal-settlement power-cut boundaries; %u recover conservative loss, %u recover win, no repeated payout\n",cuts,lost,won);
  return 0;
}
