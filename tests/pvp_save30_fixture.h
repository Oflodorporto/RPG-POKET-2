// Production Save30 journal with injectable host blob store, fake radio and UI.
// This is not a physical ESP flash/FreeRTOS test.
// Real ClubRadio callbacks, Session and Duel, with a small UI harness and fake
// ESP-NOW/FreeRTOS/persistence adapters. Compile with -Itests/club_fake and link
// ClubProtocol.cpp and ClubDuel.cpp; ClubRadio.h is included directly.
#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Persistence.h"
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
  Read presence(const char* key){before();auto result=blobs.count(key)?Read::Ok:Read::Missing;after();return result;}
  bool deleted(unsigned slot){return tomb[slot];}
  bool mark(unsigned slot,bool value){tomb[slot]=value;return true;}
  bool erase(const char* key){blobs.erase(key);return true;}
  Read bytes(const char* key,uint8_t* out,unsigned length){
    auto it=blobs.find(key);if(it==blobs.end())return Read::Missing;
    if(it->second.size()!=length)return Read::Corrupt;
    memcpy(out,it->second.data(),length);return Read::Ok;
  }
  Read read(const char* key,uint8_t* out,unsigned size){
    before();Read result;
    if(failedReadback==key){failedReadback.clear();result=Read::Error;}
    else result=bytes(key,out,size);
    after();return result;
  }
  Read readFatigue(const char* key,uint8_t* out){before();auto result=bytes(key,out,PERSISTENT_SAVE_SIZE);after();return result;}
  bool write(const char* key,const uint8_t* in,unsigned size){
    before();++primaryWrites;auto fault=failure;failure=Failure::None;
    if(fault==Failure::PrimaryWriteBefore){after();return false;}
    blobs[key]={in,in+size};
    if(fault==Failure::PrimaryReadback)failedReadback=key;
    after();return fault!=Failure::PrimaryWriteAfter;
  }
  bool writeFatigue(const char* key,const uint8_t* in){before();++auxiliaryWrites;blobs[key]={in,in+PERSISTENT_SAVE_SIZE};after();return true;}
} nvs;
using Backend=PersistentSlotBackend<RadioStore>;
Backend backend(nvs);
PersistentJournal<Backend> journal(backend);
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
std::vector<uint8_t> packed(const Game& g){std::vector<uint8_t> out(PERSISTENT_SAVE_SIZE);encodePersistent(g,1,0,out.data());return out;}
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
  game.guildMember=true;game.p.gold=100;game.fatigue=64;assert(persistentValid(game));assert(journal.save(game));
  view.page=Page::Club;afterSave=Page::Home;now=100;assert(openClub());clubRadioSent.clear();
}
Game rebootSnapshot(){PersistentJournal<Backend> observer(backend);Game loaded;assert(observer.load(loaded)==Load::Ok&&!observer.fatigueReset);return loaded;}
void recoverReboot(){assert(journal.load(game)==Load::Ok&&!journal.fatigueReset);arena=ClubUi{};clubQueue=nullptr;view.page=Page::Home;recoverClubReservation();}
void prepareAdmission(bool incoming){
  if(incoming){enqueue(packet(club::Type::Invite));tickClub(now);assert(arena.session.state==club::State::Incoming);}
  else {enqueue(packet(club::Type::Hello,0));tickClub(now);assert(arena.session.peers[0].used);}
}
void linkedAcceptor(){
  reset();prepareAdmission(true);arena.session.accept(now);assert(game.clubStage==1&&game.p.gold==75);
  enqueue(packet(club::Type::Confirm));tickClub(now);
  assert(arena.begun&&arena.session.state==club::State::Linked&&arena.duel.local==1&&arena.duel.turn==0);
  enqueue(packet(club::Type::Move));enqueue(packet(club::Type::MoveAck));tickClub(now);
  assert(arena.duel.ready&&arena.duel.revision==0);
}
