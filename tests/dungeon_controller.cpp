
#include "../firmware/RPG_POKET_2/View.h"
#include "../firmware/RPG_POKET_2/Slots.h"
#include "../firmware/RPG_POKET_2/TouchGate.h"
#include <cassert>
#include <map>
#include <string>
#include <vector>
struct Store {bool ready=true;bool fail=false;std::map<std::string,std::vector<uint8_t>> blobs;bool tomb[3]={};
 bool deleted(unsigned s){return tomb[s];}bool mark(unsigned s,bool b){if(fail)return false;tomb[s]=b;return true;}
 bool erase(const char* k){if(fail)return false;blobs.erase(k);return true;}
 rpg::Read read(const char* k,uint8_t* b){auto it=blobs.find(k);if(it==blobs.end())return rpg::Read::Missing;memcpy(b,it->second.data(),rpg::SAVE_SIZE);return rpg::Read::Ok;}
 bool write(const char* k,const uint8_t* b){if(fail)return false;blobs[k]=std::vector<uint8_t>(b,b+rpg::SAVE_SIZE);return true;}} nvs;
using NvsBackend=SlotBackend<Store>;NvsBackend backend(nvs);
struct SerialMock {void println(const char*){}template<class... T>void printf(const char*,T...){}} Serial;
uint32_t now=0;uint32_t millis(){return now;}uint32_t esp_random(){return 13;}
bool startNetworkTest(){updateInfo.busy=menu.connected;return menu.connected;}void resetUpdateScreen(){updateInfo=updater::Info{};}bool startUpdate(bool install){if(!menu.connected)return false;updateInfo.busy=true;updateInfo.state=install?updater::State::Downloading:updater::State::Checking;return true;}void paint(uint32_t){}bool openClub(){return true;}void closeClub(){}void clubTap(int,int){}void recoverClubReservation(){}ArtStatus loadSdArt(){return ArtStatus::Ready;}
bool rememberSlot(uint8_t slot){menu.activeSlot=slot;return true;}bool saveBrightness(uint8_t n){menu.brightness=n;return true;}
void applyBrightness(){}void showSavedNetworks(){menu.savedNetworks=true;}void connectSavedNetwork(){menu.connecting=true;}bool deleteSavedNetwork(){--menu.savedCount;return true;}void testMemory(){menu.memoryTest=1;}void searchNetworks(){menu.scanning=true;}void networkChoice(){}void connectNetwork(){menu.connecting=true;}void forgetConnection(){menu.connected=false;menu.connecting=false;}
struct SdFake {} sdReader;CardInfo checkCard(SdFake&){CardInfo c;c.status=CardStatus::Verified;return c;}
rpg::Journal<NvsBackend> journal(backend);
rpg::Game game;ViewState view;Page afterSave=Page::Home,helpReturn=Page::Home;TouchGate gate;
bool lcdReady=false,touchReady=false,dirty=true;
bool buffered=false;
uint32_t pollAt=0,drawAt=0,enemyAt=0,reidentifyAt=0,reportAt=0;
uint32_t busErrors=0,shortReads=0,countErrors=0,rangeErrors=0;
CombatFx combatFx;
bool pendingTouch=false;int pendingX=0,pendingY=0;Page pendingPage=Page::Home;rpg::Phase pendingPhase=rpg::Phase::Home;
uint32_t loopAt=0,loopGap=0,frameMax=0,serialDropped=0,wifiPollAt=0;
uint32_t acceptedTouches=0,maxPollGap=0,touchFeedbackUntil=0;bool polledOnce=false;
char message[40]="";
void beginEffect(Effect effect,bool onHero){
  if(view.page==Page::SaveError)return;
  combatFx.start(effect,onHero,millis());view.page=rpg::inDungeon(game)?Page::Dungeon:Page::Battle;dirty=true;
}
void say(const char* s){snprintf(message,sizeof(message),"%s",s);view.message=message;dirty=true;}
Page currentPage(){if(rpg::inDungeon(game))return Page::Dungeon;if(game.phase==rpg::Phase::Home&&game.tripStage)return game.tripStage==1?Page::TravelRoll:Page::Travel;return game.phase==rpg::Phase::Home?(game.tutorial?Page::Home:Page::Help):(game.phase==rpg::Phase::Hero||game.phase==rpg::Phase::Enemy)?Page::Battle:Page::Result;}
void activateTripPage(){menu.journey=Journey{};menu.rollReady=false;if(view.page==Page::TravelRoll){menu.rollStarted=millis();}else if(view.page==Page::Travel&&game.tripStage==3)menu.journey.start(game.city,game.tripTo,millis());pendingTouch=false;gate=TouchGate{};}
void savedTransition(Page next){
  afterSave=next;
  if(!journal.save(game)){view.page=Page::SaveError;Serial.println("SAVE FALHOU; jogo pausado para nova tentativa");}
  else{view.page=next;enemyAt=millis()+850;if(next==Page::TravelRoll||next==Page::Travel)activateTripPage();}
  dirty=true;
}
void action(rpg::Action a){
  const char* err=rpg::act(game,a);if(err){say(err);return;}
  if(a==rpg::Action::Life||a==rpg::Action::Mana)snprintf(message,sizeof(message),"Recuperou %u %s",game.damage,a==rpg::Action::Life?"HP":"MP");
  else if(a==rpg::Action::Defensive)snprintf(message,sizeof(message),"%s ativado",rpg::skillName(game.p.cls,true));
  else if(a==rpg::Action::Flee)snprintf(message,sizeof(message),"%s",game.phase==rpg::Phase::Fled?"Fuga bem-sucedida":"Fuga falhou!");
  else snprintf(message,sizeof(message),game.dodge?"Inimigo esquivou!":game.crit?"Critico! -%u HP":"Voce causou %u de dano",game.damage);
  view.message=message;savedTransition(currentPage());
  if(a==rpg::Action::Attack||a==rpg::Action::Offensive)beginEffect(a==rpg::Action::Offensive?(game.p.cls==0?Effect::Lightning:game.p.cls==3?Effect::Rage:Effect::Slash):Effect::Slash,false);
  else if(a==rpg::Action::Defensive)beginEffect(Effect::Shield,true);
}
void refreshSlots(){uint8_t old=backend.slot;for(uint8_t i=0;i<3;++i){backend.slot=i;rpg::Journal<NvsBackend> preview(backend);menu.slots[i]=preview.load(menu.previews[i]);}backend.slot=old;}
void showMap(){menu.destination=game.city;view.page=Page::Map;say("");}
void recoverClubReservation();
void selectSlot(uint8_t slot){
  if(slot>2)return;if(!rememberSlot(slot)){menu.notice="Falha ao trocar slot";dirty=true;return;}
  backend.slot=slot;game=rpg::Game{};auto loaded=journal.load(game);view.recovered=loaded==rpg::Load::Recovered;view.choice=0;helpReturn=Page::Home;
  view.page=loaded==rpg::Load::Blocked?Page::Blocked:loaded==rpg::Load::Empty?Page::Race:currentPage();combatFx.kind=Effect::None;pendingTouch=false;gate=TouchGate{};enemyAt=millis()+1200;activateTripPage();say("");if(loaded==rpg::Load::Ok||loaded==rpg::Load::Recovered)recoverClubReservation();
}
bool openClub();void closeClub();void clubTap(int,int);void recoverClubReservation();
uint32_t lastActivity=0,clockPollAt=0;Page clockReturn=Page::Home;
bool tickIdleClock(uint32_t now,bool blocked){if(blocked){lastActivity=now;return false;}if(view.page==Page::Clock)return false;if(uint32_t(now-lastActivity)<60000)return false;clockReturn=view.page;view.page=Page::Clock;menu.clockIdle=true;applyBrightness();dirty=true;return true;}
void tapped(int x,int y){
  lastActivity=millis();if(view.page==Page::Clock){view.page=clockReturn;menu.clockIdle=false;applyBrightness();dirty=true;return;}
  if(combatFx.active()||menu.journey.active)return;
  if(view.page==Page::DungeonEntry||view.page==Page::CrystalBuy||view.page==Page::DungeonExit){
    Page page=view.page;if(hit(x,y,14,272,102)){view.page=page==Page::DungeonExit?Page::Dungeon:page==Page::CrystalBuy?Page::CityGoods:Page::Ruins;say("");}
    else if(hit(x,y,124,272,102)){if(page==Page::DungeonExit){if(rpg::leaveDungeon(game)){say("Saque preservado");savedTransition(Page::Ruins);}}
      else {const char* err=page==Page::CrystalBuy?rpg::buyCrystal(game):rpg::enterDungeon(game);if(err)say(err);else {say(page==Page::CrystalBuy?"Cristal comprado":"Explore e encontre o selo");savedTransition(page==Page::CrystalBuy?Page::CityGoods:Page::Dungeon);}}}return;}
  if(view.page==Page::DungeonMenu){
    if(hit(x,y,14,278,212)){view.page=Page::Dungeon;say("");return;}
    if(hit(x,y,14,230,212)){if(game.phase==rpg::Phase::Home){view.page=Page::DungeonExit;say("");}else say("Termine o combate primeiro");return;}
    int a=hit(x,y,14,86,102)?1:hit(x,y,124,86,102)?2:hit(x,y,14,134,102)?3:hit(x,y,124,134,102)?4:hit(x,y,14,182,212)?5:-1;
    if(a<0)return;if(game.phase==rpg::Phase::Hero){action(rpg::Action(a));return;}
    if(game.phase!=rpg::Phase::Home||a<3||a>4){say("Use durante o combate");return;}
    bool mana=a==4;auto& amount=mana?game.p.mana:game.p.life;auto& value=mana?game.p.mp:game.p.hp;unsigned heal=rpg::recovery(value,mana?game.p.maxmp:game.p.maxhp,mana);
    if(!amount||!heal){say(!amount?"Sem pocoes":"Ja esta cheio");return;}--amount;value+=heal;say("Pocao usada");savedTransition(Page::Dungeon);return;}
  if(view.page==Page::Dungeon){
    if(game.phase==rpg::Phase::Enemy)return;
    if(hit(x,y,160,296,76,22)){view.page=Page::DungeonMenu;say("");return;}
    if(y<172||hit(x,y,4,296,152,22)){
      if(game.phase==rpg::Phase::Hero){action(rpg::Action::Attack);return;}
      if(game.phase!=rpg::Phase::Home){rpg::dungeonResolve(game);say("");savedTransition(rpg::inDungeon(game)?Page::Dungeon:Page::Ruins);return;}
      if(rpg::dungeonCell(rpg::dungeonFloor(game),rpg::dungeonX(game),rpg::dungeonY(game))=='E'){view.page=Page::DungeonExit;say("");return;}
      uint8_t before=game.dungeonLoot;const char* notice=rpg::dungeonCollect(game);if(before!=game.dungeonLoot){say(notice);savedTransition(Page::Dungeon);return;}if(notice){say(notice);return;}
      if(rpg::dungeonStairs(game)){say("Escadas: novo andar");savedTransition(Page::Dungeon);return;}
      int enemy=rpg::dungeonEnemyAhead(game);if(enemy>=0){rpg::begin(game,rpg::dungeonSpawns[enemy].id);say("Seu turno");savedTransition(Page::Dungeon);return;}
      say("Nada para interagir aqui");return;
    }
    if(game.phase!=rpg::Phase::Home)return;
    int forward=0,side=0,turn=0;
    if(hit(x,y,4,208,74,39))turn=-1;else if(hit(x,y,82,208,74,39))forward=1;else if(hit(x,y,160,208,76,39))turn=1;
    else if(hit(x,y,4,252,74,39))side=-1;else if(hit(x,y,82,252,74,39))forward=-1;else if(hit(x,y,160,252,76,39))side=1;else return;
    const char* err=rpg::dungeonMove(game,forward,side,turn);if(err)say(err);else {say(game.phase==rpg::Phase::Hero?"Inimigo! Seu turno":"");savedTransition(Page::Dungeon);}return;
  }
  if(view.page==Page::NetworkTest){if(updateInfo.busy)return;if(hit(x,y,14,272,102)){view.page=Page::Updates;dirty=true;}else if(hit(x,y,124,272,102)){startNetworkTest();dirty=true;}return;}
  if(view.page==Page::TravelRoll){if(menu.rollReady&&hit(x,y,14,272,212)&&rpg::acceptTrip(game)){say("");savedTransition(currentPage());}return;}
  if(view.page==Page::Club||view.page==Page::ClubBattle||view.page==Page::ClubResult){clubTap(x,y);return;}
  if(view.page==Page::Updates){if(updateInfo.busy)return;
    if(updateInfo.state==updater::State::Available||updateInfo.canResume){if(hit(x,y,14,272,102)){updateInfo=updater::Info{};resetUpdateScreen();view.page=Page::Settings;dirty=true;}else if(hit(x,y,14,218,212)){view.page=Page::NetworkTest;startNetworkTest();dirty=true;}else if(hit(x,y,124,272,102)){if(!startUpdate(true))say("Conecte o Wi-Fi para atualizar");dirty=true;}}
    else if(hit(x,y,14,173,212)){view.page=Page::NetworkTest;startNetworkTest();dirty=true;}else if(hit(x,y,14,218,212)){if(!startUpdate(false)){updateInfo.state=updater::State::Error;snprintf(updateInfo.message,sizeof(updateInfo.message),"Conecte o Wi-Fi primeiro");}dirty=true;}
    else if(hit(x,y,14,272,212)){view.page=Page::Settings;dirty=true;}return;}
  if(view.page==Page::Menu){
    if(hit(x,y,14,86,212)){if(journal.blocked||journal.active<0)say("Escolha ou crie um personagem");else showMap();}
    else if(hit(x,y,14,132,212)){refreshSlots();view.page=Page::Slots;menu.notice="";dirty=true;}
    else if(hit(x,y,14,178,212)){view.page=Page::Settings;menu.notice="";dirty=true;}
    else if(hit(x,y,14,224,102)){if(!journal.blocked&&journal.active>=0){view.page=game.city==1?Page::Ruins:Page::Village;say("");}}
    else if(hit(x,y,124,224,102)||hit(x,y,14,272,212)){view.page=journal.blocked?Page::Blocked:journal.active<0?Page::Race:currentPage();say("");}return;
  }
  if(view.page==Page::Slots){if(hit(x,y,14,272,212)){view.page=Page::Menu;say("");return;}for(uint8_t i=0;i<3;++i)if(hit(x,y,14,55+i*61,212,56)){menu.slotChoice=i;view.page=Page::SlotConfirm;menu.notice="";dirty=true;return;}return;}
  if(view.page==Page::SlotConfirm){if(hit(x,y,14,272,212)){view.page=Page::Slots;dirty=true;}
    else if(hit(x,y,14,178,212)&&menu.slots[menu.slotChoice]!=rpg::Load::Blocked)selectSlot(menu.slotChoice);
    else if(hit(x,y,14,224,212)&&menu.slots[menu.slotChoice]!=rpg::Load::Empty){view.page=Page::DeleteSlot;dirty=true;}return;}
  if(view.page==Page::DeleteSlot){if(hit(x,y,14,272,102)){view.page=Page::SlotConfirm;dirty=true;}
    else if(hit(x,y,124,272,102)){uint8_t old=backend.slot;backend.slot=menu.slotChoice;bool ok=backend.remove();backend.slot=old;if(!ok){menu.notice="Falha; personagem preservado";dirty=true;return;}refreshSlots();menu.notice="Slot apagado";
      if(menu.slotChoice==old){game=rpg::Game{};journal.load(game);view.choice=0;view.page=Page::Race;say("");}else{view.page=Page::Slots;dirty=true;}}return;}
  if(view.page==Page::Settings){if(hit(x,y,14,78,102)||hit(x,y,124,78,102)){int level=menu.brightness+(x<120?-10:10);level=std::max(10,std::min(100,level));menu.notice=saveBrightness(level)?"":"Falha ao salvar brilho";dirty=true;}
    else if(hit(x,y,14,130,212)){view.page=Page::Wifi;showSavedNetworks();menu.notice="";dirty=true;}
    else if(hit(x,y,14,176,102)){view.page=Page::Card;say("");}
    else if(hit(x,y,124,176,102)){view.page=Page::Tests;dirty=true;}
    else if(hit(x,y,14,222,212)){view.page=Page::Updates;resetUpdateScreen();dirty=true;}
    else if(hit(x,y,14,278,212)){view.page=Page::Menu;say("");}return;}
  if(view.page==Page::Tests){if(hit(x,y,14,222,212)){testMemory();pendingTouch=false;gate=TouchGate{};dirty=true;}else if(hit(x,y,14,272,212)){view.page=Page::Settings;dirty=true;}return;}
  if(view.page==Page::Wifi){if(hit(x,y,14,276,102)){view.page=Page::Settings;dirty=true;}
    else if(hit(x,y,124,276,102)){forgetConnection();dirty=true;}
    else if(hit(x,y,14,80,102)){showSavedNetworks();dirty=true;}
    else if(hit(x,y,124,80,102)){searchNetworks();dirty=true;}
    else if(hit(x,y,14,221,102)){if(menu.savedNetworks&&menu.savedCount&&!menu.connecting&&!menu.scanning)view.page=Page::ForgetWifi;dirty=true;}
    else if(hit(x,y,124,221,102)&&!menu.connecting&&!menu.scanning){if(menu.savedNetworks&&menu.savedCount)connectSavedNetwork();else if(menu.networkCount){menu.password[0]=0;menu.keyboard=menu.keyPage=0;menu.notice="";view.page=Page::Keyboard;}else menu.notice="Busque e escolha uma rede";dirty=true;}
    else if(!menu.connecting&&!menu.scanning&&(hit(x,y,14,174,102)||hit(x,y,124,174,102))){if(menu.savedNetworks&&menu.savedCount)menu.savedIndex=(menu.savedIndex+menu.savedCount+(x<120?-1:1))%menu.savedCount;else menu.networkIndex+=x<120?-1:1;networkChoice();dirty=true;}return;}
  if(view.page==Page::ForgetWifi){if(hit(x,y,14,272,102)){view.page=Page::Wifi;dirty=true;}else if(hit(x,y,124,272,102)){if(deleteSavedNetwork())view.page=Page::Wifi;dirty=true;}return;}
  if(view.page==Page::Keyboard){size_t n=strlen(menu.password);menu.notice="";
    if(hit(x,y,4,266,76,50)){memset(menu.password,0,sizeof(menu.password));view.page=Page::Wifi;dirty=true;return;}
    if(hit(x,y,160,266,76,50)){connectNetwork();if(menu.connecting)view.page=Page::Wifi;dirty=true;return;}
    if(hit(x,y,82,266,76,50)){if(n)menu.password[n-1]=0;}
    else if(hit(x,y,4,222,76)){menu.keyboard=(menu.keyboard+1)%3;menu.keyPage=0;}
    else if(hit(x,y,82,222,76))menu.keyPage=(menu.keyPage+2)%3;
    else if(hit(x,y,160,222,76))menu.keyPage=(menu.keyPage+1)%3;
    else if(x>=4&&x<238&&y>=38&&y<218){unsigned col=(x-4)/78,row=(y-38)/45;if(col<3&&row<4&&n<63){menu.password[n]=keyboardChars(menu.keyboard)[menu.keyPage*12+row*3+col];menu.password[n+1]=0;}}
    dirty=true;return;}
  if(view.page==Page::Map){for(uint8_t i=0;i<4;++i)if(hit(x,y,places[i].x-30,places[i].y-14,60,42)){menu.destination=i;dirty=true;return;}
    if(hit(x,y,124,272,102)){view.page=Page::Menu;say("");}
    else if(hit(x,y,14,272,102)){if(menu.destination==game.city){view.page=game.city==1?Page::Ruins:Page::Village;say("");}
      else {const char* err=rpg::prepareTrip(game,menu.destination);if(err)say(err);else {say("");savedTransition(Page::TravelRoll);}}}
    return;
  }
  if(view.page==Page::Blocked){if(hit(x,y,14,250,212)){view.page=Page::Menu;say("");}return;}
  if(view.page==Page::SaveError){if(hit(x,y,14,250,212))savedTransition(afterSave);return;}
  if(view.page==Page::Card){
    if(hit(x,y,14,272,212)){view.page=Page::Settings;say("");}
    else if(hit(x,y,14,224,212)){
      say("Verificando...");if(lcdReady)paint(millis());
      view.card=checkCard(sdReader);
      // A slow mount must not carry a held press into the next page/action.
      gate=TouchGate{};pendingTouch=false;say("");
      Serial.printf("CARTAO estado=%u capacidade=%lu MiB; teste somente leitura\n",unsigned(view.card.status),(unsigned long)view.card.capacityMiB);
    }return;
  }
  if(view.page==Page::Race){if(hit(x,y,14,263,102)){view.page=Page::Menu;say("");}else if(hit(x,y,124,263,102)){view.page=Page::Choose;dirty=true;}else for(unsigned i=0;i<4;++i)if(hit(x,y,14+(i%2)*110,46+(i/2)*104,102,98)){menu.draftRace=i;dirty=true;}return;}
  if(view.page==Page::Clothes){if(hit(x,y,14,272,102)){view.page=Page::Choose;dirty=true;}else if(hit(x,y,124,272,102)){game=rpg::create(view.choice,esp_random());game.race=menu.draftRace;game.shirt=menu.draftShirt;game.trousers=menu.draftPants;savedTransition(Page::Help);}else if(hit(x,y,14,177,102)||hit(x,y,124,177,102)){menu.draftShirt=(menu.draftShirt+(x<120?7:1))%8;dirty=true;}else if(hit(x,y,14,224,102)||hit(x,y,124,224,102)){menu.draftPants=(menu.draftPants+(x<120?7:1))%8;dirty=true;}return;}
  if(view.page==Page::Guild){if(hit(x,y,14,272,212)){view.page=Page::Village;say("");}else if(hit(x,y,14,170,212)){view.page=game.guildMember?Page::GuildMissions:Page::GuildJoin;say("");}else if(hit(x,y,14,218,212)){if(!game.guildMember)say("Cadastre-se primeiro");else if(game.p.level<5)say("Clube: nivel 5 necessario");else {if(openClub()){view.page=Page::Club;say("");}else say("Nao foi possivel abrir o radio");}}return;}
  if(view.page==Page::GuildJoin){if(hit(x,y,14,272,102)){view.page=Page::Guild;say("");}else if(hit(x,y,124,272,102)){if(!rpg::joinGuild(game)){say("Cadastro concluido");savedTransition(Page::Guild);}else say("Ouro insuficiente para cadastro");}return;}
  if(view.page==Page::Tavern){if(hit(x,y,14,220,212)){view.page=Page::Guild;say("");}else if(hit(x,y,14,272,212)){view.page=game.city==1?Page::Ruins:Page::Village;say("");}return;}
  if(view.page==Page::Choose){
    if(hit(x,y,0,0,36)){view.page=Page::Menu;say("");return;}
    if(hit(x,y,14,211,102))view.choice=(view.choice+3)%4;
    else if(hit(x,y,124,211,102))view.choice=(view.choice+1)%4;
    else if(hit(x,y,14,263,212)){view.page=Page::Clothes;}dirty=true;return;
  }
  if(view.page==Page::Help){if(hit(x,y,14,268,212)){say("");if(!game.tutorial){game.tutorial=true;savedTransition(helpReturn);}else{view.page=helpReturn;dirty=true;}}return;}
  if(view.page==Page::Home){
    if(hit(x,y,14,220,212)){showMap();}
    else if(hit(x,y,14,270,102)){if(game.p.hp!=game.p.maxhp||game.p.mp!=game.p.maxmp){rpg::rest(game);say("HP e MP recuperados");savedTransition(Page::Home);}else say("Voce ja esta recuperado");}
    else if(hit(x,y,124,270,102)){view.page=Page::Menu;say("");}return;
  }
  if(view.page==Page::Ruins){
    if(hit(x,y,14,52,212)){view.page=Page::DungeonEntry;say("");return;}
    if(hit(x,y,14,262,102)){showMap();return;}if(hit(x,y,124,262,102)){view.page=Page::Market;say("");return;}
    bool boss=hit(x,y,14,214,212);if(!boss&&!hit(x,y,14,166,212))return;
    if(boss&&game.ruinsWins<3){say("Venca 3 encontros primeiro");return;}
    if(rpg::explore(game,boss)){say("Seu turno");savedTransition(Page::Battle);}return;
  }
  if(view.page==Page::Village){
    if(hit(x,y,14,126,102)){view.page=Page::Market;say("");}
    else if(hit(x,y,124,126,102)){view.page=Page::Inventory;say("");}
    else if(hit(x,y,14,170,102)){view.page=Page::Character;dirty=true;}
    else if(hit(x,y,124,170,102)){view.page=Page::Explore;say("");}
    else if(hit(x,y,14,214,102)){view.page=Page::Guild;say("");}
    else if(hit(x,y,124,214,102)){if(game.p.hp!=game.p.maxhp||game.p.mp!=game.p.maxmp){rpg::rest(game);say("HP e MP recuperados");savedTransition(Page::Village);}else say("Voce ja esta recuperado");}
    else if(hit(x,y,14,258,102)){showMap();}
    else if(hit(x,y,124,258,102)){view.page=Page::Menu;say("");}return;
  }
  if(view.page==Page::Explore){if(hit(x,y,14,270,212)){view.page=Page::Village;say("");}else if(hit(x,y,14,220,212)&&rpg::explore(game)){say("Seu turno");savedTransition(Page::Battle);}return;}
  if(view.page==Page::CityGoods){if(game.city==1&&hit(x,y,124,218,102)){view.page=Page::CrystalBuy;say("");return;}if(hit(x,y,14,270,212)){view.page=Page::Market;say("");}else if(hit(x,y,14,218,212)){view.page=Page::GoodsBuy;say("");}return;}
  if(view.page==Page::GoodsBuy){if(hit(x,y,14,270,102)){view.page=Page::CityGoods;say("");}else if(hit(x,y,124,270,102)){auto err=rpg::buyGood(game);if(err)say(err);else {say("Suprimento comprado");savedTransition(Page::CityGoods);}}return;}
  if(view.page==Page::GuildMissions){
    if(!game.guildMember){view.page=Page::Guild;say("Cadastre-se primeiro");return;}
    if(hit(x,y,14,272,212)){view.page=Page::Guild;say("");return;}
    for(uint8_t id=1;id<=3;++id)if(hit(x,y,14,124+(id-1)*44,212)){view.questChoice=id;view.page=Page::Contract;say("");return;}return;
  }
  if(view.page==Page::Contract){
    if(hit(x,y,14,272,212)){view.page=Page::GuildMissions;say("");return;}
    if(!hit(x,y,14,220,212))return;
    if(game.questId&&game.questId!=view.questChoice){say("Outra missao ativa");return;}
    if(game.questId&&rpg::questComplete(game)&&game.p.gold>999999u-rpg::contractGold(game.questId,game.questLevel)){say("Gaste ouro antes de receber");return;}
    view.questAction=game.questId?(rpg::questComplete(game)?1:2):0;view.page=Page::QuestConfirm;say("");return;
  }
  if(view.page==Page::QuestConfirm){
    if(hit(x,y,14,270,102)){view.page=Page::Contract;say("Acao cancelada");}
    else if(hit(x,y,124,270,102)){
      if(view.questAction&&game.questId!=view.questChoice){say("Contrato mudou");return;}
      uint16_t xp=rpg::contractXp(game.questId,game.questLevel),gold=rpg::contractGold(game.questId,game.questLevel);
      const char* err=view.questAction==0?rpg::acceptQuest(game,view.questChoice):view.questAction==1?rpg::claimQuest(game):rpg::abandonQuest(game);
      if(err){say(err);return;}
      if(view.questAction==1){snprintf(message,sizeof(message),"+%u XP / +%u ouro",xp,gold);view.message=message;}
      else say(view.questAction==0?"Missao aceita":"Missao abandonada");savedTransition(Page::GuildMissions);
    }return;
  }
  if(view.page==Page::Character){if(hit(x,y,14,270,212)){view.page=game.city==1?Page::Ruins:Page::Village;say("");}return;}
  if(view.page==Page::Market||view.page==Page::Inventory){
    bool shop=view.page==Page::Market;
    if(!shop&&hit(x,y,14,272,102)){view.page=Page::Menu;say("");return;}
    if(hit(x,y,14,shop?138:180,212)){view.page=shop?Page::Shop:Page::TownBag;say("");}
    else if(hit(x,y,14,shop?182:226,212)){view.gearIndex=0;view.page=shop?Page::GearShop:Page::GearBag;say("");}
    else if(shop&&hit(x,y,14,226,102)){view.page=Page::Forge;say("");}
    else if(shop&&hit(x,y,124,226,102)){view.page=Page::CityGoods;say("");}
    else if(shop?hit(x,y,14,270,212):hit(x,y,124,272,102)){view.page=game.city==1?Page::Ruins:Page::Village;say("");}return;
  }
  if(view.page==Page::Forge){
    if(hit(x,y,124,224,102)){view.page=Page::Market;say("");return;}
    int slot=hit(x,y,14,174,102)?0:hit(x,y,124,174,102)?1:hit(x,y,14,224,102)?2:-1;if(slot<0)return;
    view.forgeSlot=slot;view.page=Page::Upgrade;say("");return;
  }
  if(view.page==Page::Upgrade){
    if(hit(x,y,14,270,102)){view.page=Page::Forge;say("Acao cancelada");}
    else if(game.forge[view.forgeSlot]<3&&hit(x,y,124,270,102)){
      const char* err=rpg::upgradeForge(game,view.forgeSlot);if(err){say(err);return;}
      say("Melhoria aplicada");savedTransition(Page::Forge);
    }return;
  }
  if(view.page==Page::GearShop||view.page==Page::GearBag){
    bool shop=view.page==Page::GearShop;uint8_t count=shop?rpg::cityGearCount(game):rpg::gearOwnedCount(game.owned);
    if(hit(x,y,14,272,212)){view.page=shop?Page::Market:Page::Inventory;say("");return;}
    if(!count){if(hit(x,y,14,226,212)){view.page=Page::GearShop;view.gearIndex=0;say("");}return;}
    if(hit(x,y,14,226,102)){view.gearIndex=(view.gearIndex+count-1)%count;say("");return;}
    if(hit(x,y,124,226,102)){view.gearIndex=(view.gearIndex+1)%count;say("");return;}
    if(!hit(x,y,14,180,212))return;
    view.itemId=shop?rpg::cityGearOffer(game,view.gearIndex%count):rpg::gearOwnedAt(game.owned,view.gearIndex%count);
    if(shop){const char* err=rpg::gearBuyError(game,view.itemId);if(err){say(err);return;}}
    view.page=shop?Page::GearBuy:Page::GearEquip;say("");return;
  }
  if(view.page==Page::GearBuy||view.page==Page::GearEquip){
    bool buy=view.page==Page::GearBuy;Page back=buy?Page::GearShop:Page::GearBag;
    if(hit(x,y,14,270,102)){view.page=back;say("Acao cancelada");}
    else if(hit(x,y,124,270,102)){
      const char* err=buy?rpg::buyGear(game,view.itemId):rpg::equipGear(game,view.itemId);
      if(err){say(err);return;}
      say(buy?"Item comprado":game.equipped[rpg::gearSlot(view.itemId)]==view.itemId?"Item equipado":"Item retirado");savedTransition(back);
    }return;
  }
  if(view.page==Page::Shop){
    if(hit(x,y,14,270,212)){view.page=Page::Market;say("");return;}
    int item=hit(x,y,14,172,212)?0:hit(x,y,14,218,212)?1:-1;if(item<0)return;
    if((item?game.p.mana:game.p.life)>=99){say("Bolsa cheia");return;}
    if(game.p.gold<rpg::potionPrice(game,item)){say("Ouro insuficiente");return;}
    view.choice=item;view.page=Page::Buy;say("");return;
  }
  if(view.page==Page::Buy){
    if(hit(x,y,14,270,102)){view.page=Page::Shop;say("Compra cancelada");}
    else if(hit(x,y,124,270,102)){
      const char* err=rpg::buyPotion(game,view.choice==1);if(err){say(err);return;}
      say(view.choice?"+1 pocao de mana":"+1 pocao de vida");savedTransition(Page::Shop);
    }return;
  }
  if(view.page==Page::TownBag){
    if(hit(x,y,14,270,212)){view.page=Page::Inventory;say("");return;}
    int item=hit(x,y,14,172,212)?0:hit(x,y,14,218,212)?1:-1;if(item<0)return;
    uint16_t before=item?game.p.mp:game.p.hp;const char* err=rpg::usePotionAtHome(game,item==1);if(err){say(err);return;}
    snprintf(message,sizeof(message),"Recuperou %u %s",(item?game.p.mp:game.p.hp)-before,item?"MP":"HP");view.message=message;savedTransition(Page::TownBag);return;
  }
  if(view.page==Page::Result){if(hit(x,y,14,268,212)&&rpg::home(game)){say("");savedTransition(currentPage());}return;}
  if(game.phase!=rpg::Phase::Hero)return;
  if(view.page==Page::Skills||view.page==Page::Bag){
    bool bag=view.page==Page::Bag;
    if(hit(x,y,14,172,212))action(bag?rpg::Action::Life:rpg::Action::Offensive);
    else if(hit(x,y,14,218,212))action(bag?rpg::Action::Mana:rpg::Action::Defensive);
    else if(hit(x,y,14,270,212)){view.page=Page::Battle;say("Seu turno");}return;
  }
  if(hit(x,y,14,220,102))action(rpg::Action::Attack);
  else if(hit(x,y,124,220,102)){view.page=Page::Skills;say("");}
  else if(hit(x,y,14,270,102)){view.page=Page::Bag;say("");}
  else if(hit(x,y,124,270,102))action(rpg::Action::Flee);
}

int main(){
 auto loaded=journal.load(game);assert(loaded==rpg::Load::Empty);view.page=Page::Race;tapped(170,280);
 // First character, tutorial, menu, and refusal to create a phantom hero from an empty slot.
 tapped(130,220);assert(view.choice==1);tapped(100,280);tapped(170,290);assert(view.page==Page::Help&&journal.active>=0);tapped(100,280);assert(view.page==Page::Home&&game.tutorial);
 auto first=game;tapped(170,285);assert(view.page==Page::Menu);tapped(100,150);assert(view.page==Page::Slots&&menu.slots[0]==rpg::Load::Ok);
 tapped(80,130);assert(view.page==Page::SlotConfirm&&menu.slotChoice==1);tapped(80,190);assert(view.page==Page::Race&&menu.activeSlot==1);
 view.page=Page::Race;tapped(30,280);assert(view.page==Page::Menu);tapped(80,100);assert(view.page==Page::Menu&&journal.active<0);tapped(80,285);assert(view.page==Page::Race);tapped(170,280);
 tapped(130,220);tapped(130,220);tapped(100,280);tapped(170,290);tapped(100,280);assert(game.p.cls==2&&view.page==Page::Home);
 tapped(170,285);tapped(100,150);tapped(80,75);tapped(80,190);assert(menu.activeSlot==0&&game.p.cls==first.p.cls&&game.tutorial);
 // Cancellation must preserve both blobs. Confirmed deletion targets only slot2.
 view.page=Page::Menu;tapped(100,150);tapped(80,130);tapped(80,240);assert(view.page==Page::DeleteSlot);auto before=nvs.blobs;tapped(50,290);assert(nvs.blobs==before&&view.page==Page::SlotConfirm);
 tapped(80,240);tapped(170,290);assert(menu.activeSlot==0&&view.page==Page::Slots&&menu.slots[1]==rpg::Load::Empty&&menu.slots[0]==rpg::Load::Ok);
 // Failed explicit deletion leaves the character unchanged and stays on confirmation.
 tapped(80,75);tapped(80,240);nvs.fail=true;tapped(170,290);assert(view.page==Page::DeleteSlot&&!nvs.tomb[0]);nvs.fail=false;tapped(50,290);
 // Card test moved into settings, all controls open their matching rendered page.
 view.page=Page::Menu;tapped(80,195);assert(view.page==Page::Settings);tapped(170,90);assert(menu.brightness==90);tapped(170,90);tapped(170,90);assert(menu.brightness==100);
 tapped(50,190);assert(view.page==Page::Card);tapped(80,235);assert(view.card.status==CardStatus::Verified);tapped(80,285);assert(view.page==Page::Settings);
 tapped(160,190);assert(view.page==Page::Tests);tapped(80,240);assert(menu.memoryTest==1);tapped(80,285);tapped(80,145);assert(view.page==Page::Wifi);tapped(170,100);assert(menu.scanning);
 menu.scanning=false;menu.savedNetworks=false;menu.networkCount=1;tapped(170,240);assert(view.page==Page::Keyboard);tapped(20,50);assert(!strcmp(menu.password,"a"));tapped(180,240);assert(menu.keyPage==1);tapped(20,50);assert(!strcmp(menu.password,"am"));tapped(100,290);assert(!strcmp(menu.password,"a"));tapped(50,290);assert(view.page==Page::Wifi&&!*menu.password);
 // Idle clock: threshold, blocked work, wake consumes tap, no character mutation.
 auto idleSaves=nvs.blobs;view.page=Page::Wifi;lastActivity=0;assert(!tickIdleClock(59999,false));assert(tickIdleClock(60000,false)&&view.page==Page::Clock&&menu.clockIdle);now=60001;tapped(170,240);assert(view.page==Page::Wifi&&!menu.clockIdle&&nvs.blobs==idleSaves);assert(!tickIdleClock(now+60000,true)&&view.page==Page::Wifi);assert(!tickIdleClock(now+60001,false));lastActivity=UINT32_MAX-200;assert(tickIdleClock(60000,false));tapped(10,10);assert(view.page==Page::Wifi);
 // Saved-network connection and explicit forgetting preserve all character saves.
 menu.savedNetworks=true;menu.savedCount=2;auto wifiSaves=nvs.blobs;tapped(170,240);assert(menu.connecting&&view.page==Page::Wifi);tapped(170,290);assert(!menu.connecting);tapped(50,240);assert(view.page==Page::ForgetWifi);tapped(50,290);assert(view.page==Page::Wifi&&menu.savedCount==2);tapped(50,240);tapped(170,290);assert(view.page==Page::Wifi&&menu.savedCount==1&&nvs.blobs==wifiSaves);
 // Update requires consent; busy update ignores navigation, no saves changed.
 view.page=Page::Settings;tapped(80,240);assert(view.page==Page::Updates&&!updateInfo.busy);auto updateSaves=nvs.blobs;menu.connected=false;tapped(80,230);assert(updateInfo.state==updater::State::Error&&!updateInfo.busy);menu.connected=true;tapped(80,230);assert(updateInfo.busy&&updateInfo.state==updater::State::Checking);tapped(80,285);assert(view.page==Page::Updates&&nvs.blobs==updateSaves);
 updateInfo.busy=false;updateInfo.state=updater::State::Available;tapped(170,285);assert(updateInfo.busy&&updateInfo.state==updater::State::Downloading&&nvs.blobs==updateSaves);updateInfo.busy=false;updateInfo.state=updater::State::Available;tapped(50,285);assert(view.page==Page::Settings&&!updateInfo.busy);menu.connected=false;
 // Dice persists before feedback; a failed save retries the same result.
 view.page=Page::Map;menu.destination=3;nvs.fail=true;tapped(50,290);assert(view.page==Page::SaveError&&game.tripStage==1&&!menu.journey.active);auto rolled=game.tripRoll;nvs.fail=false;tapped(80,270);assert(view.page==Page::TravelRoll&&game.tripRoll==rolled);tapped(80,290);assert(game.tripStage==1);menu.rollReady=true;tapped(80,290);
 if(game.tripStage==2){assert(view.page==Page::Battle);game.enemyHp=0;rpg::finish(game);rpg::home(game);savedTransition(currentPage());}
 assert(view.page==Page::Travel&&menu.journey.active&&game.city==0);tapped(100,150);assert(view.page==Page::Travel);assert(menu.journey.tick(now+3000));rpg::arriveTrip(game);savedTransition(Page::Village);
 rpg::Journal<NvsBackend> restart(backend);rpg::Game copy;assert(restart.load(copy)==rpg::Load::Ok&&copy.city==3&&!copy.tripStage);
 // New city controls and prices use the actual controller.
 view.page=Page::Village;tapped(170,190);assert(view.page==Page::Explore);tapped(80,285);assert(view.page==Page::Village);tapped(50,140);tapped(170,240);assert(view.page==Page::CityGoods);tapped(80,235);assert(view.page==Page::GoodsBuy);game.p.gold=100;tapped(170,285);assert(view.page==Page::CityGoods&&game.charms==1&&game.p.gold==75);
 menu.journey=Journey{};rpg::clearTrip(game);view.page=Page::Home;

 // Creation is preview-only until the final confirmation, with outfit and race per slot.
 auto savedBefore=nvs.blobs;view.page=Page::Race;tapped(150,170);assert(menu.draftRace==3&&nvs.blobs==savedBefore);tapped(170,280);tapped(80,280);assert(view.page==Page::Clothes&&nvs.blobs==savedBefore);tapped(170,190);tapped(170,235);assert(menu.draftShirt==1&&menu.draftPants==1&&nvs.blobs==savedBefore);
 // Do not replace the existing active hero in this test; cancel personalization.
 tapped(40,290);assert(view.page==Page::Choose&&nvs.blobs==savedBefore);
 game=rpg::create(1,33);game.p.gold=99;view.page=Page::Guild;tapped(80,190);assert(view.page==Page::GuildJoin);tapped(170,290);assert(!game.guildMember&&game.p.gold==99&&view.page==Page::GuildJoin);
 game.p.gold=100;tapped(170,290);assert(game.guildMember&&game.p.gold==0&&view.page==Page::Guild);auto joined=nvs.blobs;tapped(80,190);assert(view.page==Page::GuildMissions);tapped(80,290);assert(view.page==Page::Guild&&game.p.gold==0&&nvs.blobs==joined);
 tapped(80,240);assert(view.page==Page::Guild);view.page=Page::Tavern;tapped(80,235);assert(view.page==Page::Guild);tapped(80,190);tapped(80,135);assert(view.page==Page::Contract);
 game=rpg::create(0,42);game.city=1;game.crystals=1;game.tutorial=true;view.page=Page::Ruins;journal.blocked=false;
 tapped(100,70);assert(view.page==Page::DungeonEntry);nvs.fail=true;tapped(180,290);assert(view.page==Page::SaveError&&game.crystals==0&&rpg::inDungeon(game));
 nvs.fail=false;tapped(100,265);assert(view.page==Page::Dungeon&&game.crystals==0);tapped(30,230);auto sequence=journal.seq;tapped(110,230);assert(journal.seq==sequence);tapped(190,230);assert(rpg::dungeonHeading(game)==1&&journal.seq==sequence+1);
 tapped(110,230);tapped(110,230);assert(game.phase==rpg::Phase::Hero&&game.enemyId==2&&view.page==Page::Dungeon);combatFx.kind=Effect::None;game.enemyHp=1;game.p.atk=99;tapped(80,305);assert(view.page==Page::Dungeon||view.page==Page::SaveError);combatFx.kind=Effect::None;
 game.phase=rpg::Phase::Won;game.enemyHp=0;assert(journal.save(game));tapped(80,305);assert(game.phase==rpg::Phase::Home&&game.dungeonEnemies&1);tapped(190,305);assert(view.page==Page::DungeonMenu);tapped(100,250);assert(view.page==Page::DungeonExit);tapped(180,290);assert(view.page==Page::Ruins&&!rpg::inDungeon(game));
 puts("PASS: actual sketch controller; create/tutorial, slot switch/delete/cancel/failure, empty-slot guard, settings/card/test/keyboard controls, travel input lock, saved destination and save retry.");
}
