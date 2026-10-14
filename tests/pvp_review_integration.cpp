#define main included_radio_main
#include "fatigue76_club_radio.cpp"
#undef main
#include <array>
#include <cstddef>
#include <limits>
using club::Type;
void deliver(club::Packet p,unsigned delay=0,const uint8_t* mac=remoteMac){now+=delay;esp_now_recv_info_t info{mac};clubReceive(&info,reinterpret_cast<const uint8_t*>(&p),sizeof(p));tickClub(now);}
void immutable(const club::Packet& p,const uint8_t* mac=remoteMac){auto before=arena;auto save=packed(game);auto writes=nvs.primaryWrites;arena.session.receive(mac,p,now);assert(!memcmp(&before,&arena,sizeof(arena)));assert(save==packed(game)&&writes==nvs.primaryWrites);}
int main(){
 static_assert(sizeof(club::Packet)==52&&offsetof(club::Packet,version)==8&&offsetof(club::Packet,resultDigest)==44&&offsetof(club::Packet,race)==48);
 linkedAcceptor();unsigned transport=0;
 for(unsigned len=0;len<=300;++len)for(unsigned version: {0u,4u,5u,6u,255u}){
  std::vector<uint8_t> exact(len,0);if(len>8)exact[8]=version;clubQueue->items.clear();esp_now_recv_info_t info{remoteMac};clubReceive(&info,exact.data(),len);
  bool accepted=(len==44&&version==4)||(len==52&&version==5);assert(clubQueue->items.size()==unsigned(accepted));++transport;
 }
 clubQueue->items.clear();printf("PASS: %u exact-size transport/version boundaries (0..300 bytes); no short-buffer read\n",transport);
 unsigned invalid=0;
 for(unsigned t=0;t<=unsigned(Type::Result);++t)for(unsigned field=0;field<4;++field)for(unsigned value=field==0?4:field==3?1:8;value<256;++value){
  auto p=packet(Type(t),t?200:0,t>=unsigned(Type::Move)?1:0);if(t==unsigned(Type::Result)){p.reserved[2]=1;p.resultDigest=0x12345;}
  reinterpret_cast<uint8_t*>(&p)[offsetof(club::Packet,race)+field]=value;immutable(p);++invalid;
 }
 printf("PASS: %u invalid appearance values rejected before all 11 message types; no Duel/Session/Game/save mutation\n",invalid);
 // Incoming move: only authenticated session identity (not crypto) and expected turn/sequence may capture.
 auto p=packet(Type::Move,200,1);uint8_t stranger[6]={2,4,6,8,10,12};immutable(p,stranger);
 for(unsigned field=0;field<5;++field){auto bad=p;if(field==0)++bad.race;if(field==1)++bad.shirt;if(field==2)++bad.trousers;if(field==3)++bad.classId;if(field==4)++bad.level;immutable(bad);}
 auto bad=p;bad.session=bad.stakeSession=201;immutable(bad);bad=p;bad.reserved[0]=2;immutable(bad);
 deliver(p);assert(arena.visual.active()&&arena.visual.actor==0&&arena.visual.revision==1&&arena.visual.action==club::Action::Attack);
 const auto first=arena.visual;auto save=packed(game);auto writes=nvs.primaryWrites;deliver(p,60);assert(arena.visual.startedAt==first.startedAt&&arena.visual.elapsedMs==60&&save==packed(game)&&writes==nvs.primaryWrites);
 // Sender cannot animate its prediction. Wrong ACK identity/action/session/sequence cannot capture it.
 clubTap(30,230);assert(arena.duel.waiting&&arena.duel.revision==2&&arena.visual.revision==1);auto old=arena.visual;
 auto ack=packet(Type::MoveAck,200,2);immutable(ack,stranger);
 for(unsigned field=0;field<5;++field){auto bad=ack;if(field==0)++bad.race;if(field==1)++bad.shirt;if(field==2)++bad.trousers;if(field==3)++bad.classId;if(field==4)++bad.level;immutable(bad);}
 bad=ack;bad.reserved[2]=1;immutable(bad);bad=ack;bad.reserved[0]=3;immutable(bad);bad=ack;bad.session=bad.stakeSession=201;immutable(bad);
 deliver(ack,60);assert(arena.visual.revision==2&&arena.visual.actor==1&&arena.visual.startedAt==now);const auto second=arena.visual;deliver(ack,60);assert(arena.visual.startedAt==second.startedAt&&arena.visual.revision==2);
 // A valid next opponent command proves prior local command; only newest cosmetic event survives.
 p=packet(Type::Move,200,3);deliver(p,60);assert(arena.visual.revision==3&&arena.visual.actor==0);clubTap(30,230);assert(arena.duel.waiting&&arena.visual.revision==3);p=packet(Type::Move,200,5);p.reserved[2]=2;deliver(p,60);assert(arena.visual.revision==5&&arena.visual.actor==0&&arena.visual.action==club::Action::Defensive&&arena.duel.confirmedRevision==5&&!arena.duel.waiting);
 puts("PASS: real queue + Session + ClubRadio guards peer/session/appearance/sequence/turn; captures correct actor only after proof; duplicate ACK/Move never restarts; newest valid event replaces cosmetics");
 // Cadence is controlled by tickClub, including wrap, not by redraw count.
 for(uint32_t start:{uint32_t(500),uint32_t(UINT32_MAX-100)}){
  reset();arena.redraw=start;dirty=false;tickClub(start+199);assert(!dirty);tickClub(start+200);assert(dirty);
  arena.visual.revision=1;arena.visual.startedAt=start;arena.visual.elapsedMs=0;arena.visual.action=club::Action::Attack;arena.redraw=start;dirty=false;
  tickClub(start+59);assert(!dirty&&arena.visual.elapsedMs==59);tickClub(start+60);assert(dirty&&arena.visual.elapsedMs==60);
  arena.visual.startedAt=start;arena.visual.elapsedMs=1099;arena.redraw=start+1099;dirty=false;tickClub(start+1100);assert(dirty&&!arena.visual.active());
 }
 puts("PASS: tickClub 60 ms active/200 ms idle and expiry redraw across uint32 wrap");
 // Terminal proof may immediately settle: effect does not delay durable result or grace period.
 linkedAcceptor();deliver(packet(Type::Move,200,1));arena.duel.fighters[0].hp=1;clubTap(30,230);assert(arena.duel.waiting&&!arena.duel.terminalConfirmed);
 auto result=packet(Type::Result,200,2);result.reserved[2]=2;result.resultDigest=arena.duel.stateDigest();auto wrong=result;++wrong.race;immutable(wrong);deliver(result,20);assert(view.page==Page::ClubResult&&arena.result==1&&arena.visual.revision==2&&game.p.gold==120);save=packed(game);writes=nvs.primaryWrites;
 deliver(result,20);assert(save==packed(game)&&writes==nvs.primaryWrites);const auto finished=arena.finishedAt;
 now=finished+10999;clubTap(30,285);assert(arena.opened&&view.page==Page::ClubResult);now=finished+11000;clubTap(30,285);assert(!arena.opened&&view.page==Page::Guild);
 puts("PASS: Result identity/proof capture cannot delay settlement; duplicated result does not pay again; exit exactly at 11000 ms");
 // Legacy discovery is explanatory only. Malformed old payloads cannot invite or start.
 reset();auto legacy=packet(Type::Hello,0);legacy.version=4;esp_now_recv_info_t info{remoteMac};clubReceive(&info,reinterpret_cast<uint8_t*>(&legacy),44);tickClub(now);assert(arena.session.peers[0].used&&arena.session.peers[0].version==4&&!arena.session.invite(0,200,now));
 legacy=packet(Type::Invite);legacy.version=4;clubReceive(&info,reinterpret_cast<uint8_t*>(&legacy),44);tickClub(now);assert(arena.session.state==club::State::Listing&&game.clubStage==0);
 puts("PASS: 44-byte v4 Hello displayed but invitation blocked; no old-version admission");
 // Decoder fuzz, memory-safety only; fresh bounded message inputs, reproducible seed.
 reset();uint32_t seed=0x91a5ba;for(unsigned i=0;i<100000;++i){club::Packet fuzz;auto* b=reinterpret_cast<uint8_t*>(&fuzz);for(unsigned j=0;j<sizeof(fuzz);++j){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;b[j]=seed;}if(i%2==0){fuzz.magic=club::MAGIC;fuzz.version=club::VERSION;}arena.session.receive(remoteMac,fuzz,now);}
 puts("PASS: 100000 deterministic arbitrary Packet decoder inputs, memory safety only");
}
