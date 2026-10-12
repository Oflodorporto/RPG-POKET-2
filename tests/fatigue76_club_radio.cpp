// Real ClubRadio callbacks, Session and Duel, with a small UI harness and fake
// ESP-NOW/FreeRTOS/persistence adapters. Compile with -Itests/club_fake and link
// ClubProtocol.cpp and ClubDuel.cpp; ClubRadio.h is included directly.
#include "TestHero.h"
#include "../firmware/RPG_POKET_2/FatigueJournal.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
using namespace rpg;
enum class Failure {None,PrimaryWriteBefore,PrimaryWriteAfter,PrimaryReadback};
struct PowerCut {};
struct RadioStore {
  bool ready=true,tomb[3]{};
  std::map<std::string,std::vector<uint8_t>> blobs;
  Failure failure=Failure::None;
  std::string failedReadback;
  unsigned primaryWrites=0,auxiliaryWrites=0,operations=0,cut=0;
  bool cutAfter=false;
  void before(){++operations;if(cut==operations&&!cutAfter)throw PowerCut{};}
  void after(){if(cut==operations&&cutAfter)throw PowerCut{};}
  bool deleted(unsigned slot){return tomb[slot];}
  bool mark(unsigned slot,bool value){tomb[slot]=value;return true;}
  bool erase(const char* key){blobs.erase(key);return true;}
  Read bytes(const char* key,uint8_t* out,unsigned length){
    auto it=blobs.find(key);if(it==blobs.end())return Read::Missing;
    if(it->second.size()!=length)return Read::Corrupt;
    memcpy(out,it->second.data(),length);return Read::Ok;
  }
  Read read(const char* key,uint8_t* out){
    before();Read result;
    if(failedReadback==key){failedReadback.clear();result=Read::Error;}
    else result=bytes(key,out,SAVE_SIZE);
    after();return result;
  }
  Read readFatigue(const char* key,uint8_t* out){before();auto result=bytes(key,out,FATIGUE_RECORD_SIZE);after();return result;}
  bool write(const char* key,const uint8_t* in){
    before();++primaryWrites;auto fault=failure;failure=Failure::None;
    if(fault==Failure::PrimaryWriteBefore){after();return false;}
    blobs[key]={in,in+SAVE_SIZE};
    if(fault==Failure::PrimaryReadback)failedReadback=key;
    after();return fault!=Failure::PrimaryWriteAfter;
  }
  bool writeFatigue(const char* key,const uint8_t* in){before();++auxiliaryWrites;blobs[key]={in,in+FATIGUE_RECORD_SIZE};after();return true;}
} nvs;
using Backend=FatigueSlotBackend<RadioStore>;
Backend backend(nvs);
FatigueJournal<Backend> journal(backend);
Game game;
enum class Page {Home,Guild,Club,ClubBattle,ClubResult,SaveError};
struct {Page page=Page::Home;} view;
struct {bool scanning=false,connecting=false,connected=false;} menu;
Page afterSave=Page::Home;
bool dirty=false;
uint32_t now=100;
uint32_t millis(){return now;}
uint32_t esp_random(){return 200;}
void resumeConnection(){}
void say(const char*){}
Page currentPage(){return Page::Home;}
void savedTransition(Page next){afterSave=next;view.page=journal.save(game)?next:Page::SaveError;dirty=true;}
bool hit(int x,int y,int left,int top,int width,int height=40){return x>=left&&x<left+width&&y>=top&&y<top+height;}
#include "../firmware/RPG_POKET_2/ClubRadio.h"

constexpr uint8_t remoteMac[6]={2,0,0,0,0,2};
std::vector<uint8_t> packed(const Game& g){std::vector<uint8_t> out(SAVE_SIZE);encode(g,1,out.data());return out;}
bool same(const Game& a,const Game& b){return packed(a)==packed(b)&&a.fatigue==b.fatigue;}
club::Packet packet(club::Type type,uint32_t session=200,unsigned sequence=0){
  club::Packet p{};p.magic=club::MAGIC;p.version=club::VERSION;p.type=uint8_t(type);
  p.session=session;p.stakeSession=session;p.classId=1;p.level=5;p.available=1;memcpy(p.name,"HERO",5);
  p.reserved[0]=sequence&255;p.reserved[1]=sequence>>8;return p;
}
void enqueue(const club::Packet& p){esp_now_recv_info_t info{remoteMac};assert(clubRadioReceiver);clubRadioReceiver(&info,reinterpret_cast<const uint8_t*>(&p),sizeof(p));}
void reset(){
  nvs=RadioStore{};backend.slot=0;Game unused;assert(journal.load(unused)==Load::Empty);
  game=testHero(1,41);game.p.level=5;game.p.mp=game.p.maxmp=totalMana(game);
  game.guildMember=true;game.p.gold=100;game.fatigue=64;assert(valid(game));assert(journal.save(game));
  view.page=Page::Club;afterSave=Page::Home;now=100;assert(openClub());clubRadioSent.clear();
}
Game rebootSnapshot(){FatigueJournal<Backend> observer(backend);Game loaded;assert(observer.load(loaded)==Load::Ok&&!observer.fatigueReset);return loaded;}
void recoverReboot(){assert(journal.load(game)==Load::Ok&&!journal.fatigueReset);arena=ClubUi{};clubQueue=nullptr;view.page=Page::Home;recoverClubReservation();}
void prepareAdmission(bool incoming){
  if(incoming){enqueue(packet(club::Type::Invite));tickClub(now);assert(arena.session.state==club::State::Incoming);}
  else {enqueue(packet(club::Type::Hello,0));tickClub(now);assert(arena.session.peers[0].used);}
}
void attemptAdmission(bool incoming){if(incoming)arena.session.accept(now);else assert(!arena.session.invite(0,200,now));}
void admissionCompensation(Failure failure,bool incoming){
  reset();prepareAdmission(incoming);auto before=game;auto oldSeq=journal.seq;
  nvs.failure=failure;attemptAdmission(incoming);
  assert(view.page==Page::SaveError&&same(game,before));
  assert(arena.session.state==(incoming?club::State::Incoming:club::State::Listing));
  auto frozenWrites=nvs.primaryWrites;auto frozenBlobs=nvs.blobs;
  attemptAdmission(incoming);assert(view.page==Page::SaveError&&same(game,before)&&nvs.primaryWrites==frozenWrites&&nvs.blobs==frozenBlobs);
  auto committed=rebootSnapshot();assert(committed.clubStage==1&&committed.clubSession==200&&committed.p.gold==75&&committed.fatigue==64);
  auto writes=nvs.primaryWrites;tickClub(now+40000);assert(nvs.primaryWrites==writes&&same(game,before));
  // A second failure during deliberate compensation must not overwrite the
  // auxiliary of the reservation that actually committed despite its lost ACK.
  nvs.failure=Failure::PrimaryWriteBefore;savedTransition(afterSave);
  assert(view.page==Page::SaveError&&same(game,before));
  assert(same(rebootSnapshot(),committed));
  savedTransition(afterSave);assert(view.page==Page::Club&&same(game,before));
  assert(journal.seq==oldSeq+2&&same(rebootSnapshot(),before));
  recoverReboot();assert(same(game,before));
}
void admissionReboot(Failure failure,bool incoming){
  reset();prepareAdmission(incoming);nvs.failure=failure;attemptAdmission(incoming);
  assert(view.page==Page::SaveError&&game.p.gold==100);
  recoverReboot();assert(game.clubStage==3&&game.p.gold==100&&game.fatigue==64);
  auto settled=game;auto writes=nvs.primaryWrites;recoverClubReservation();assert(nvs.primaryWrites==writes&&same(game,settled));
  recoverReboot();assert(same(game,settled));
}
unsigned compensationBoundaries=0;
void compensationCuts(Failure failure,bool incoming){
  reset();prepareAdmission(incoming);nvs.failure=failure;attemptAdmission(incoming);
  nvs.operations=0;savedTransition(afterSave);assert(view.page==Page::Club);
  unsigned count=nvs.operations;
  for(unsigned boundary=1;boundary<=count;++boundary)for(bool after:{false,true}){
    reset();prepareAdmission(incoming);auto before=game;nvs.failure=failure;attemptAdmission(incoming);
    auto reserved=rebootSnapshot();nvs.operations=0;nvs.cut=boundary;nvs.cutAfter=after;
    try{savedTransition(afterSave);}catch(const PowerCut&){}
    nvs.cut=0;auto restored=rebootSnapshot();
    assert(same(restored,reserved)||same(restored,before));
    recoverReboot();assert(game.p.gold==100&&game.fatigue==64&&(game.clubStage==0||game.clubStage==3));
    auto settled=game;auto writes=nvs.primaryWrites;recoverClubReservation();assert(same(game,settled)&&nvs.primaryWrites==writes);
    recoverReboot();assert(same(game,settled));++compensationBoundaries;
  }
}
void linkedAcceptor(){
  reset();prepareAdmission(true);arena.session.accept(now);assert(game.clubStage==1&&game.p.gold==75);
  enqueue(packet(club::Type::Confirm));tickClub(now);
  assert(arena.begun&&arena.session.state==club::State::Linked&&arena.duel.local==1&&arena.duel.turn==0);
  enqueue(packet(club::Type::Move));enqueue(packet(club::Type::MoveAck));tickClub(now);
  assert(arena.duel.ready&&arena.duel.revision==0);
}
void freezeQueuedPacket(Failure failure){
  linkedAcceptor();nvs.failure=failure;auto writes=nvs.primaryWrites;
  enqueue(packet(club::Type::Move,200,1));enqueue(packet(club::Type::Cancel));tickClub(now);
  assert(view.page==Page::SaveError&&game.clubStage==2&&game.p.gold==75&&game.fatigue==64);
  assert(arena.session.state==club::State::Linked&&arena.duel.revision==0&&clubQueue->items.size()==1);
  assert(nvs.primaryWrites==writes+1);auto pending=game;
  tickClub(now+40000);assert(view.page==Page::SaveError&&same(game,pending)&&clubQueue->items.size()==1&&nvs.primaryWrites==writes+1);
  savedTransition(afterSave);assert(view.page==Page::ClubBattle&&same(game,pending));
  tickClub(now);assert(view.page==Page::ClubResult&&game.clubStage==3&&game.p.gold==75&&game.fatigue==64);
  auto settled=game;writes=nvs.primaryWrites;tickClub(now);assert(same(game,settled)&&nvs.primaryWrites==writes);
  recoverReboot();assert(same(game,settled));
}
void rejectionRefund(Failure failure,bool reboot){
  reset();prepareAdmission(false);assert(arena.session.invite(0,200,now));
  assert(game.clubStage==1&&game.p.gold==75);nvs.failure=failure;
  enqueue(packet(club::Type::Reject));tickClub(now);
  assert(arena.session.state==club::State::Notice&&view.page==Page::SaveError&&game.clubStage==3&&game.p.gold==100&&game.fatigue==64);
  auto refunded=game;auto writes=nvs.primaryWrites;tickClub(now+40000);assert(same(game,refunded)&&nvs.primaryWrites==writes);
  if(reboot)recoverReboot();else {savedTransition(afterSave);assert(view.page==Page::Club);tickClub(now);}
  assert(same(game,refunded));writes=nvs.primaryWrites;recoverClubReservation();assert(same(game,refunded)&&nvs.primaryWrites==writes);
  recoverReboot();assert(same(game,refunded));
}
int main(int argc,char** argv){
  const std::string selection=argc>1?argv[1]:"all";
  for(auto failure:{Failure::PrimaryWriteAfter,Failure::PrimaryReadback}){
    if(selection=="all"||selection=="admission")for(bool incoming:{false,true}){admissionCompensation(failure,incoming);admissionReboot(failure,incoming);}
    if(selection=="all"||selection=="cuts")for(bool incoming:{false,true})compensationCuts(failure,incoming);
    if(selection=="all"||selection=="queue")freezeQueuedPacket(failure);
    if(selection=="all"||selection=="refund")for(bool reboot:{false,true})rejectionRefund(failure,reboot);
  }
  printf("PASS: fatigue76 real ClubRadio admission compensation, %u compensation power-cut boundaries, queued save-error freeze, and single-refund retry/reboot\n",compensationBoundaries);
}
