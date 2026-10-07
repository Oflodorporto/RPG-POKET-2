// Waveshare SKU29667 ONLY. 2026.10.06-bolsa1. Manual USB upload only.
// Separate NVS namespace pkt2_slice; never imports or clears Heltec saves.
#include <Arduino.h>
#ifndef ARDUINO_ESP32S3_DEV
#error "Use ESP32S3 Dev Module (Espressif). Este sketch nao e para Heltec."
#endif
#ifndef BOARD_HAS_PSRAM
#error "Selecione PSRAM > OPI PSRAM para a Waveshare SKU29667."
#endif
#include <Wire.h>
#include <SPI.h>
#include <Preferences.h>
#include "src/GFX/databus/Arduino_HWSPI.h"
#include "src/GFX/display/Arduino_ST7789.h"
#include "TouchGate.h"
#include "Save.h"
#include "View.h"
#include "FrameBuffer.h"
#include "SdReader.h"
#include "Slots.h"
#include "DeviceSettings.h"
#include "UpdateDevice.h"
Arduino_HWSPI bus(42,45,39,38,40);
Arduino_ST7789 lcd(&bus,-1,0,true,240,320);
FrameBuffer frameBuffer;
SdReader sdReader;
struct NvsStore {
  Preferences prefs;bool ready=false;
  rpg::Read read(const char* key,uint8_t* b){
    if(!ready)return rpg::Read::Error;
    if(!prefs.isKey(key))return rpg::Read::Missing;
    size_t n=prefs.getBytesLength(key);if(n!=64&&n!=rpg::SAVE_SIZE)return rpg::Read::Error;memset(b,0,rpg::SAVE_SIZE);
    return prefs.getBytes(key,b,n)==n?rpg::Read::Ok:rpg::Read::Error;
  }
  bool write(const char* key,const uint8_t* b){return ready&&prefs.putBytes(key,b,rpg::SAVE_SIZE)==rpg::SAVE_SIZE;}
  bool deleted(uint8_t slot){char k[4];snprintf(k,sizeof(k),"t%u",slot);return prefs.getBool(k,false);}
  bool mark(uint8_t slot,bool value){char k[4];snprintf(k,sizeof(k),"t%u",slot);return ready&&prefs.putBool(k,value)==1;}
  bool erase(const char* key){return ready&&(!prefs.isKey(key)||prefs.remove(key));}
} nvs;
using NvsBackend=SlotBackend<NvsStore>;
NvsBackend backend(nvs);
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
void sampleTouch();
void paint(uint32_t now){
  uint32_t started=millis();
  view.effect=combatFx.kind;view.effectFrame=combatFx.frame(now);view.effectOnHero=combatFx.onHero;view.heroFrame=combatFx.active()&&!combatFx.onHero?1+std::min(4u,combatFx.frame(now)*5/8):0;
  if(buffered){render(frameBuffer,game,view,now/120);
    // Short SPI stripes let the touch reader run during each full-screen transfer.
    for(int y=0;y<320;y+=16){{UpdateSpiLock lock;lcd.draw16bitRGBBitmap(0,y,frameBuffer.pixels+y*240,240,16);}sampleTouch();}
  }
  else {UpdateSpiLock lock;render(lcd,game,view,now/120);}
  frameMax=std::max(frameMax,uint32_t(millis()-started));
}
bool readRegs(uint8_t reg,uint8_t* data,size_t n){
  Wire.beginTransmission(0x15);Wire.write(reg);
  if(Wire.endTransmission(true)!=0){++busErrors;return false;}
  if(Wire.requestFrom(uint8_t(0x15),n)!=n||Wire.available()<int(n)){while(Wire.available())Wire.read();++shortReads;return false;}
  for(size_t i=0;i<n;++i)data[i]=Wire.read();return true;
}
bool readTouch(bool& down,uint16_t& x,uint16_t& y){
  uint8_t b[5];if(!readRegs(2,b,5))return false;
  if(b[0]>1){++countErrors;return false;}down=b[0]==1;if(!down)return true;
  x=((b[1]&15)<<8)|b[2];y=((b[3]&15)<<8)|b[4];
  if(x>=240||y>=320){++rangeErrors;return false;}return true;
}
void sampleTouch(){
  uint32_t now=millis();if(uint32_t(now-pollAt)<8)return;
  if(polledOnce)maxPollGap=std::max(maxPollGap,uint32_t(now-pollAt));polledOnce=true;pollAt=now;
  bool down=false;uint16_t x=0,y=0;bool valid=touchReady&&readTouch(down,x,y);
  if(gate.update(valid,down,now)){
    ++acceptedTouches;touchFeedbackUntil=now+140;view.touchFeedback=true;dirty=true;
    // Never carry an input made during an animation/enemy turn into the next turn.
    if(!pendingTouch&&!combatFx.active()&&!menu.journey.active&&!(view.page==Page::Battle&&game.phase!=rpg::Phase::Hero)){
      pendingTouch=true;pendingX=x;pendingY=y;pendingPage=view.page;pendingPhase=game.phase;
    }
  }
}
void beginEffect(Effect effect,bool onHero){
  if(view.page==Page::SaveError)return;
  combatFx.start(effect,onHero,millis());view.page=rpg::inDungeon(game)?Page::Dungeon:Page::Battle;dirty=true;
}
void say(const char* s){snprintf(message,sizeof(message),"%s",s);view.message=message;dirty=true;}
Page currentPage(){if(rpg::inDungeon(game))return game.phase==rpg::Phase::Won&&game.enemyId==8?Page::DungeonVictory:Page::Dungeon;if(game.phase==rpg::Phase::Home&&game.tripStage)return game.tripStage==1?Page::TravelRoll:Page::Travel;return game.phase==rpg::Phase::Home?(game.tutorial?Page::Home:Page::Help):(game.phase==rpg::Phase::Hero||game.phase==rpg::Phase::Enemy)?Page::Battle:Page::Result;}
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
  if(a==rpg::Action::Attack||a==rpg::Action::Offensive)beginEffect(a==rpg::Action::Offensive?(game.p.cls==0?Effect::Lightning:game.p.cls==3?Effect::Rage:Effect::Slash):game.p.cls==0?Effect::Projectile:game.p.cls==2?Effect::Thrust:Effect::Slash,false);
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
  if(view.page==Page::DungeonVictory){
    if(hit(x,y,14,218,212)||hit(x,y,14,272,212)){bool exit=y>=272;
      if(rpg::dungeonResolve(game)){if(exit)rpg::leaveDungeon(game);say(exit?"Saque preservado":"Bau liberado! Explore a sala");savedTransition(exit?Page::Ruins:Page::Dungeon);}}return;
  }
  if(view.page==Page::DungeonLoot){
    if(hit(x,y,14,272,212)){view.page=Page::Dungeon;say("");return;}
    if(hit(x,y,14,218,212)){unsigned count=rpg::gearOwnedCount(game.owned),index=0;auto id=rpg::gearOffer(game.p.cls,1);while(index<count&&rpg::gearOwnedAt(game.owned,index)!=id)++index;view.gearIndex=index/6;view.choice=index%6;view.page=Page::BagGear;say("");}return;
  }
  if(view.page==Page::BagGear){
    if(hit(x,y,14,278,102)){view.page=rpg::inDungeon(game)||game.phase!=rpg::Phase::Home?Page::Bag:Page::TownBag;view.choice=0;say("");return;}
    unsigned count=rpg::gearOwnedCount(game.owned),pages=std::max(1u,(count+5)/6);
    if(hit(x,y,10,200,68,24)||hit(x,y,162,200,68,24)){view.gearIndex=(view.gearIndex+(x<120?pages-1:1))%pages;view.choice=0;say("");return;}
    for(unsigned i=0;i<6;++i)if(hit(x,y,10+(i%3)*76,74+(i/3)*64,68,58)){view.choice=i;say("");return;}
    if(hit(x,y,124,278,102)){unsigned index=view.gearIndex*6+view.choice;if(index>=count){say("Slot vazio");return;}
      if(game.phase!=rpg::Phase::Home){say("Equipe depois do combate");return;}
      auto err=rpg::equipGear(game,rpg::gearOwnedAt(game.owned,index));if(err)say(err);else {say("Equipamento alterado");savedTransition(Page::BagGear);}}return;
  }
  if(view.page==Page::Bag||view.page==Page::TownBag){
    Page back=rpg::inDungeon(game)?Page::Dungeon:view.page==Page::Bag?Page::Battle:Page::Inventory;
    if(hit(x,y,14,278,102)){view.page=back;say("");return;}
    for(unsigned i=0;i<6;++i)if(hit(x,y,10+(i%3)*76,74+(i/3)*64,68,58)){view.choice=i;say("");return;}
    if(hit(x,y,14,230,212)){view.gearIndex=0;view.choice=0;view.page=Page::BagGear;say("");return;}
    if(!hit(x,y,124,278,102))return;
    if(view.choice>=2){say(view.choice==2?"Cristal: entrada nas Ruinas":"Usado automaticamente na viagem");return;}
    bool mana=view.choice==1;
    if(game.phase==rpg::Phase::Hero){action(mana?rpg::Action::Mana:rpg::Action::Life);return;}
    if(game.phase!=rpg::Phase::Home){say("Termine o turno primeiro");return;}
    auto& amount=mana?game.p.mana:game.p.life;auto& value=mana?game.p.mp:game.p.hp;unsigned heal=rpg::recovery(value,mana?game.p.maxmp:game.p.maxhp,mana);
    if(!amount||!heal){say(!amount?"Sem pocoes":"Ja esta cheio");return;}--amount;value+=heal;say("Pocao usada");savedTransition(view.page);return;
  }
  if(view.page==Page::DungeonMenu){
    if(hit(x,y,14,278,212)){view.page=Page::Dungeon;say("");return;}
    if(hit(x,y,14,230,212)){if(game.phase==rpg::Phase::Home){view.page=Page::DungeonExit;say("");}else say("Termine o combate primeiro");return;}
    if(hit(x,y,14,134,212)){view.choice=0;view.page=Page::Bag;say("");return;}
    int a=hit(x,y,14,86,102)?1:hit(x,y,124,86,102)?2:hit(x,y,14,182,212)?5:-1;
    if(a<0)return;if(game.phase==rpg::Phase::Hero)action(rpg::Action(a));else say("Use durante o combate");return;}
  if(view.page==Page::Dungeon){
    if(game.phase==rpg::Phase::Enemy)return;
    if(hit(x,y,160,280,76,38)){view.page=Page::DungeonMenu;say("");return;}
    if(hit(x,y,4,280,152,38)){view.choice=0;view.page=Page::Bag;say("");return;}
    if(y<172){
      if(game.phase==rpg::Phase::Hero){action(rpg::Action::Attack);return;}
      if(game.phase!=rpg::Phase::Home){rpg::dungeonResolve(game);say("");savedTransition(rpg::inDungeon(game)?Page::Dungeon:Page::Ruins);return;}
      if(rpg::dungeonCell(rpg::dungeonFloor(game),rpg::dungeonX(game),rpg::dungeonY(game))=='E'){view.page=Page::DungeonExit;say("");return;}
      int enemy=rpg::dungeonEnemyAhead(game);if(enemy>=0){rpg::begin(game,rpg::dungeonSpawns[enemy].id);say("Seu turno");savedTransition(Page::Dungeon);return;}
      uint8_t before=game.dungeonLoot;const char* notice=rpg::dungeonCollect(game);if(before!=game.dungeonLoot){say(notice);savedTransition((game.dungeonLoot&128)&&!(before&128)?Page::DungeonLoot:Page::Dungeon);return;}if(notice){say(notice);return;}
      if(rpg::dungeonStairs(game)){say("Escadas: novo andar");savedTransition(Page::Dungeon);return;}
      say("Nada para interagir aqui");return;
    }
    if(game.phase!=rpg::Phase::Home)return;
    int forward=0,side=0,turn=0;
    if(hit(x,y,4,202,74,35))turn=-1;else if(hit(x,y,82,202,74,35))forward=1;else if(hit(x,y,160,202,76,35))turn=1;
    else if(hit(x,y,4,241,74,35))side=-1;else if(hit(x,y,82,241,74,35))forward=-1;else if(hit(x,y,160,241,76,35))side=1;else return;
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
  if(view.page==Page::Result){if(hit(x,y,14,268,212)&&rpg::home(game)){say("");savedTransition(game.tripStage?currentPage():game.city==1?Page::Ruins:Page::Explore);}return;}
  if(game.phase!=rpg::Phase::Hero)return;
  if(view.page==Page::Skills){
    if(hit(x,y,14,172,212))action(rpg::Action::Offensive);
    else if(hit(x,y,14,218,212))action(rpg::Action::Defensive);
    else if(hit(x,y,14,270,212)){view.page=Page::Battle;say("Seu turno");}return;
  }
  if(hit(x,y,14,220,102))action(rpg::Action::Attack);
  else if(hit(x,y,124,220,102)){view.page=Page::Skills;say("");}
  else if(hit(x,y,14,270,102)){view.choice=0;view.page=Page::Bag;say("");}
  else if(hit(x,y,124,270,102))action(rpg::Action::Flee);
}
#include "ClubRadio.h"
void setup(){
  Serial.begin(115200);Serial.setTxTimeoutMs(0);pinMode(41,OUTPUT);digitalWrite(41,HIGH);pinMode(1,OUTPUT);digitalWrite(1,LOW);
  Wire.begin(48,47);Wire.setClock(400000);Wire.setTimeOut(20);
  uint8_t id=0;touchReady=readRegs(0xa7,&id,1)&&id==0xb6;
  lcdReady=lcd.begin(40000000);
  buffered=frameBuffer.begin();frameBuffer.onChunk=sampleTouch;
  initUpdater();initSettings();nvs.ready=nvs.prefs.begin("pkt2_slice",false);backend.slot=menu.activeSlot;
  artMemory=static_cast<uint8_t*>(heap_caps_malloc(ART_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!artMemory){Serial.println("Memoria de artes indisponivel; nenhum save alterado");if(lcdReady){lcd.fillScreen(UI_INK);lcd.setTextColor(UI_WHITE);lcd.setTextSize(1);lcd.setCursor(12,130);lcd.print("PSRAM indisponivel");lcd.setCursor(12,156);lcd.print("Confira OPI PSRAM no IDE");lcd.setCursor(12,182);lcd.print("Saves preservados");}while(true)delay(1000);}
  if(lcdReady){lcd.fillScreen(UI_INK);lcd.setTextColor(UI_GOLD);lcd.setTextSize(2);lcd.setCursor(12,130);lcd.print("Carregando artes...");lcd.setTextSize(1);lcd.setCursor(69,177);lcd.print("Saves protegidos");}
  menu.art=loadSdArt();menu.flashMiB=ESP.getFlashChipSize()/1048576;menu.ramMiB=ESP.getPsramSize()/1048576;resumeConnection();
  auto loaded=journal.load(game);view.recovered=loaded==rpg::Load::Recovered;
  view.page=loaded==rpg::Load::Blocked?Page::Blocked:loaded==rpg::Load::Empty?Page::Race:currentPage();
  say(view.recovered?"Checkpoint recuperado":game.phase==rpg::Phase::Enemy?"Retomando turno salvo":"Seu turno");
  if(loaded==rpg::Load::Ok||loaded==rpg::Load::Recovered)recoverClubReservation();
  lastActivity=millis();activateTripPage();enemyAt=millis()+1200;
  if(lcdReady){paint(millis());applyBrightness();dirty=false;}
  Serial.printf("RPG POKET 2.0 2026.10.06-bolsa1 LCD=%d touch=%d flash=%u PSRAM=%u load=%u\n",lcdReady,touchReady,ESP.getFlashChipSize(),ESP.getPsramSize(),unsigned(loaded));
}
void loop(){
  uint32_t now=millis();
  menu.frameMs=frameMax;menu.pollMs=maxPollGap;menu.loopMs=loopGap;menu.touches=acceptedTouches;menu.touchErrors=busErrors+shortReads+countErrors+rangeErrors;
  if(loopAt)loopGap=std::max(loopGap,uint32_t(now-loopAt));loopAt=now;
  if(tickUpdater())dirty=true;
  tickClub(now);
  if(!arena.opened&&uint32_t(now-wifiPollAt)>=250){wifiPollAt=now;if(tickWifi())dirty=true;}
  if(menu.journey.tick(now)){rpg::arriveTrip(game);pendingTouch=false;gate=TouchGate{};savedTransition(game.city==1?Page::Ruins:Page::Village);}
  if(view.page==Page::TravelRoll&&!menu.rollReady&&uint32_t(now-menu.rollStarted)>=1800){menu.rollReady=true;pendingTouch=false;gate=TouchGate{};dirty=true;}
  if(combatFx.expire(now)){view.page=currentPage();dirty=true;}
  if(view.touchFeedback&&int32_t(now-touchFeedbackUntil)>=0){view.touchFeedback=false;dirty=true;}
  if(!touchReady&&uint32_t(now-reidentifyAt)>=1000){reidentifyAt=now;uint8_t id=0;touchReady=readRegs(0xa7,&id,1)&&id==0xb6;}
  sampleTouch();
  if(pendingTouch){pendingTouch=false;if(pendingPage==view.page&&pendingPhase==game.phase)tapped(pendingX,pendingY);}
  now=millis();
  tickIdleClock(now,updateInfo.busy||arena.opened||menu.connecting||menu.scanning||menu.journey.active||combatFx.active()||(view.page==Page::TravelRoll&&!menu.rollReady)||((view.page==Page::Battle||view.page==Page::Dungeon)&&game.phase==rpg::Phase::Enemy));
  if(view.page==Page::Clock&&uint32_t(now-clockPollAt)>=1000){clockPollAt=now;if(tickClock())dirty=true;}
  if(!combatFx.active()&&(view.page==Page::Battle||view.page==Page::Dungeon)&&game.phase==rpg::Phase::Enemy&&int32_t(now-enemyAt)>=0){
    rpg::enemy(game);snprintf(message,sizeof(message),game.dodge?"Voce esquivou!":game.crit?"Critico inimigo! -%u HP":"Inimigo causou %u de dano",game.damage);
    view.message=message;savedTransition(currentPage());beginEffect(game.enemyId==5?Effect::Projectile:Effect::Slash,true);
  }
  if(lcdReady&&(dirty||((view.page==Page::Battle||view.page==Page::Dungeon||view.page==Page::Travel||view.page==Page::TravelRoll||view.page==Page::Tests||view.page==Page::Updates)&&uint32_t(now-drawAt)>=(combatFx.active()?60u:120u)))){drawAt=now;paint(now);dirty=false;}
  if(uint32_t(now-reportAt)>=5000){reportAt=now;char report[160];int n=snprintf(report,sizeof(report),"TOQUE erros=%lu aceitos=%lu gap=%lu frame=%lu loop=%lu drop=%lu\n",(unsigned long)(busErrors+shortReads+countErrors+rangeErrors),(unsigned long)acceptedTouches,(unsigned long)maxPollGap,(unsigned long)frameMax,(unsigned long)loopGap,(unsigned long)serialDropped);if(Serial&&Serial.availableForWrite()>=n)Serial.write(reinterpret_cast<const uint8_t*>(report),n);else ++serialDropped;maxPollGap=frameMax=loopGap=0;}

  delay(1);
}
