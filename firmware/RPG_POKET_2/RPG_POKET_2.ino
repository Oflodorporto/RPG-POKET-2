// Waveshare SKU29667 ONLY. Pacote 1 - Origem e rostos. Manual USB upload only.
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
    size_t n=prefs.getBytesLength(key);if(n!=64&&n!=96&&n!=rpg::SAVE_SIZE)return rpg::Read::Error;memset(b,0,rpg::SAVE_SIZE);
    return prefs.getBytes(key,b,n)==n&&rpg::get16(b,6)==n?rpg::Read::Ok:rpg::Read::Error;
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
  static Page paperLastPage=Page::Choose;if(view.page!=paperLastPage){paperLastPage=view.page;view.paperStartFrame=now/120;view.paperAnimate=storyPaperPage(view.page);}
  view.campProgress=game.campStage==3?std::min<uint32_t>(1000u,uint32_t(now-menu.campStarted)*1000/4500):0;
  uint32_t started=millis();
  view.effect=combatFx.kind;view.effectFrame=combatFx.frame(now);view.effectOnHero=combatFx.onHero;view.heroFrame=combatFx.active()&&!combatFx.onHero?1+std::min(4u,combatFx.frame(now)*5/8):0;
  menu.renderNow=now;
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
Page currentPage(){if(game.discovery&&game.phase==rpg::Phase::Home)return game.chestLock?Page::ChestLock:Page::Discovery;if(game.campaignStage)return game.phase==rpg::Phase::Hero||game.phase==rpg::Phase::Enemy?Page::Battle:Page::CampaignResult;if(game.eventStage==2)return game.phase==rpg::Phase::Hero||game.phase==rpg::Phase::Enemy?Page::Battle:Page::EventResult;if(game.campStage==1)return Page::CampRoll;if(game.campStage==3)return Page::CampRest;if(rpg::inDungeon(game))return rpg::dungeonBossWon(game)?Page::DungeonVictory:Page::Dungeon;if(game.phase==rpg::Phase::Home&&game.tripStage)return game.tripStage==1?Page::TravelRoll:Page::Travel;return game.phase==rpg::Phase::Home?(game.tutorial?Page::Home:game.originStory&&game.originPage==8?Page::Help:Page::Prologue):(game.phase==rpg::Phase::Hero||game.phase==rpg::Phase::Enemy)?Page::Battle:Page::Result;}
void activateTripPage(){menu.journey=Journey{};menu.rollReady=false;if(view.page==Page::TravelRoll||view.page==Page::CampRoll){menu.rollStarted=millis();}else if(view.page==Page::CampRest){menu.campStarted=millis();}else if(view.page==Page::Travel&&game.tripStage==3)menu.journey.start(game.city,game.tripTo,millis());pendingTouch=false;gate=TouchGate{};}
void savedTransition(Page next){
  afterSave=next;
  if(!journal.save(game)){view.page=Page::SaveError;Serial.println("SAVE FALHOU; jogo pausado para nova tentativa");}
  else{view.page=next;enemyAt=millis()+850;if(next==Page::TravelRoll||next==Page::Travel||next==Page::CampRoll||next==Page::CampRest)activateTripPage();}
  dirty=true;
}
void action(rpg::Action a){
  if(game.dndProgression&&game.p.cls==0&&a==rpg::Action::Offensive)a=rpg::Action::MagicMissile;
  if(game.dndProgression&&game.p.cls==0&&a==rpg::Action::Defensive)a=rpg::Action::ShieldSpell;
  const char* err=rpg::act(game,a);if(err){say(err);return;}
  if(a==rpg::Action::LayHands||a==rpg::Action::SecondWind)snprintf(message,sizeof(message),"%s: +%u HP",rpg::powerName(a),game.damage);
  else if(a==rpg::Action::ActionSurge)say("Surto: sua proxima acao e extra");
  else if(a==rpg::Action::RagePower)say(rpg::persistentRage(game)?"Furia: ate o fim da luta":"Furia: 3 rodadas; resiste fisico");
  else if(a==rpg::Action::SacredWeapon||a==rpg::Action::TurnUndead)snprintf(message,sizeof(message),"%s ativado",rpg::powerName(a));
  else if(a==rpg::Action::Life||a==rpg::Action::Mana)snprintf(message,sizeof(message),"Recuperou %u %s",game.damage,a==rpg::Action::Life?"HP":"MP");
  else if(a==rpg::Action::Defensive)snprintf(message,sizeof(message),"%s ativado",rpg::skillName(game.p.cls,true));
  else if(a==rpg::Action::Flee)snprintf(message,sizeof(message),"%s",game.phase==rpg::Phase::Fled?"Fuga bem-sucedida":"Fuga falhou!");
  else snprintf(message,sizeof(message),game.dodge?"Inimigo esquivou!":game.crit?"Critico! -%u HP":"Voce causou %u de dano",game.damage);
  view.message=message;savedTransition(currentPage());
  if(!game.dodge&&(a==rpg::Action::DivineSmite||(game.dndProgression&&game.p.cls==1&&game.p.level>=11&&(a==rpg::Action::Attack||a==rpg::Action::Offensive))))beginEffect(Effect::DivineSlash,false);
  else if(a==rpg::Action::DivineSmite)beginEffect(Effect::Slash,false);
  else if(a==rpg::Action::Attack||a==rpg::Action::Offensive)beginEffect(a==rpg::Action::Offensive?(game.p.cls==0?Effect::Lightning:game.p.cls==3?Effect::Rage:Effect::Slash):game.p.cls==0?Effect::Projectile:game.p.cls==2?Effect::Thrust:Effect::Slash,false);
  else if(a==rpg::Action::Defensive||a==rpg::Action::ShieldSpell||a==rpg::Action::LayHands||a==rpg::Action::SecondWind)beginEffect(Effect::Shield,true);
  else if(a==rpg::Action::RagePower||a==rpg::Action::ActionSurge)beginEffect(a==rpg::Action::RagePower?Effect::Rage:Effect::Shield,true);
  else if(a==rpg::Action::MagicMissile)beginEffect(Effect::MagicDarts,false);
  else if(a==rpg::Action::BurningHands||a==rpg::Action::ScorchingRay)beginEffect(Effect::FlameVolley,false);
  else if(a==rpg::Action::Fireball)beginEffect(Effect::FireBurst,false);
  else if(a==rpg::Action::SacredWeapon||a==rpg::Action::TurnUndead)beginEffect(Effect::Radiant,a==rpg::Action::SacredWeapon);
}
void refreshSlots(){uint8_t old=backend.slot;for(uint8_t i=0;i<3;++i){backend.slot=i;rpg::Journal<NvsBackend> preview(backend);menu.slots[i]=preview.load(menu.previews[i]);}backend.slot=old;menu.hasContinue=false;for(auto status:menu.slots)if(status==rpg::Load::Ok||status==rpg::Load::Recovered)menu.hasContinue=true;}
void openCamp(){menu.campRation=menu.campKit=false;view.page=Page::CampSetup;say("");}
void showMap(){menu.destination=game.city;view.page=Page::Map;say("");}
void recoverClubReservation();
void selectSlot(uint8_t slot){
  if(slot>2)return;if(!rememberSlot(slot)){menu.notice="Falha ao trocar slot";dirty=true;return;}
  backend.slot=slot;menu.sessionStarted=true;game=rpg::Game{};auto loaded=journal.load(game);view.recovered=loaded==rpg::Load::Recovered;view.choice=0;view.campaignScene=0;helpReturn=Page::Home;menu.storyIndex=game.originStory?std::min(7u,unsigned(game.originPage)):0;menu.storyReplay=false;
  view.page=loaded==rpg::Load::Blocked?Page::Blocked:loaded==rpg::Load::Empty?Page::Race:currentPage();combatFx.kind=Effect::None;pendingTouch=false;gate=TouchGate{};enemyAt=millis()+1200;activateTripPage();say("");if(loaded==rpg::Load::Ok||loaded==rpg::Load::Recovered)recoverClubReservation();
}
void openTitle(){
  menu.sessionStarted=false;menu.creationFromTitle=false;menu.newGameSlots=false;menu.notice="";
  menu.journey=Journey{};combatFx.kind=Effect::None;pendingTouch=false;gate=TouchGate{};refreshSlots();view.page=Page::Title;say("");
}
bool canOfferEvents(){return !game.discovery&&menu.sessionStarted&&!journal.blocked&&journal.active>=0;}
void bootTitle(){auto loaded=journal.load(game);view.recovered=loaded==rpg::Load::Recovered;openTitle();}
void continueTitle(){
  if(!menu.hasContinue)return;uint8_t slot=menu.activeSlot;
  if(menu.slots[slot]!=rpg::Load::Ok&&menu.slots[slot]!=rpg::Load::Recovered){for(uint8_t i=0;i<3;++i)if(menu.slots[i]==rpg::Load::Ok||menu.slots[i]==rpg::Load::Recovered){slot=i;break;}}
  menu.creationFromTitle=false;menu.newGameSlots=false;selectSlot(slot);
}
void newTitle(){
  refreshSlots();menu.creationFromTitle=true;menu.newGameSlots=true;menu.slotsReturn=int(Page::Title);menu.draftRace=menu.draftShirt=menu.draftPants=0;view.choice=0;
  for(uint8_t i=0;i<3;++i)if(menu.slots[i]==rpg::Load::Empty){selectSlot(i);return;}
  view.page=Page::Slots;menu.notice="Escolha um slot para liberar";dirty=true;
}
bool openClub();void closeClub();void clubTap(int,int);void recoverClubReservation();
uint32_t lastActivity=0,clockPollAt=0;Page clockReturn=Page::Home;
bool tickIdleClock(uint32_t now,bool blocked){if(blocked){lastActivity=now;return false;}if(view.page==Page::Clock)return false;if(uint32_t(now-lastActivity)<60000)return false;clockReturn=view.page;view.page=Page::Clock;menu.clockIdle=true;applyBrightness();dirty=true;return true;}
void tapped(int x,int y){
  lastActivity=millis();if(view.page==Page::Clock){if(menu.sessionStarted&&game.eventStage==1){menu.eventReturn=int(clockReturn);menu.notice="";menu.letterStarted=millis();view.page=Page::Letter;menu.clockIdle=false;applyBrightness();dirty=true;return;}view.page=clockReturn;menu.clockIdle=false;applyBrightness();dirty=true;return;}
  if(view.page==Page::Title){
    if(hit(x,y,82,170,150))continueTitle();
    else if(hit(x,y,82,216,150))newTitle();
    else if(hit(x,y,82,262,150)){view.page=Page::Settings;menu.notice="";dirty=true;}return;
  }
  if(combatFx.active()||menu.journey.active)return;
  if(view.page==Page::CampRest)return;
  if(view.page==Page::ChestLock){
    if(hit(x,y,14,288,212,28)){rpg::clearDiscovery(game);say("Bau deixado para tras");savedTransition(game.city==1?Page::Ruins:Page::Explore);return;}
    if(game.chestLock==2){if(hit(x,y,14,256,212,28)){auto err=rpg::collectDiscovery(game);if(err)say(err);else {say("Guardado na bolsa");savedTransition(currentPage());}}return;}
    if(game.chestLock==3)return;
    bool pick=hit(x,y,14,256,212,28);if(!pick&&!hit(x,y,14,224,212,28))return;
    auto err=rpg::attemptLock(game,pick);if(err)say(err);else {say(rpg::lockSuccess(game)?"Fechadura aberta":game.chestTrap?"Armadilha: confira seu HP":"Tentativa falhou");savedTransition(Page::ChestLock);}return;
  }
  if(view.page==Page::Lockpicks){if(hit(x,y,14,270,102)){view.page=view.picksReturn;say("");}else if(hit(x,y,124,270,102)){auto err=rpg::buyGazua(game);if(err)say(err);else {say("Uma gazua guardada na bolsa");savedTransition(Page::Lockpicks);}}return;}
  if(view.page==Page::Discovery){
    if(hit(x,y,14,270,102)){rpg::clearDiscovery(game);say("Exploracao concluida");savedTransition(game.city==1?Page::Ruins:Page::Explore);}
    else if(hit(x,y,124,270,102)){if(game.discovery==5||game.discovery==7){rpg::clearDiscovery(game);say("");savedTransition(game.city==1?Page::Ruins:Page::Explore);}else {auto err=rpg::collectDiscovery(game);if(err)say(err);else {say(game.discovery==4?"O bau era um mimico!":game.discovery==7?"Carga guardada para a guilda":"Guardado na bolsa");savedTransition(currentPage());}}}return;}
  if(view.page==Page::Scrap){if(hit(x,y,14,270,102)){view.page=Page::TownBag;say("");}else if(hit(x,y,124,270,102)||hit(x,y,14,216,212)){auto err=rpg::sellScrap(game,y<260);if(err)say(err);else {say(y<260?"Sucata descartada":"Sucata vendida");savedTransition(Page::Scrap);}}return;}
  if(view.page==Page::Prologue){if(hit(x,y,14,272,102)||hit(x,y,124,272,102)){
    bool finish=x<120?menu.storyIndex==0:menu.storyIndex+1>=story::originCount(game);
    if(finish){if(menu.storyReplay){view.page=Page(menu.storyReturn);menu.storyIndex=0;say("");}
      else {if(game.originStory)game.originPage=8;menu.storyIndex=0;say("");if(game.originStory)savedTransition(Page::Help);else view.page=Page::Help;}}
    else {menu.storyIndex+=x<120?-1:1;say("");if(!menu.storyReplay&&game.originStory){game.originPage=menu.storyIndex;savedTransition(Page::Prologue);}}
   }return;}
  if((view.page==Page::Home&&hit(x,y,10,140,220,30))||(view.page==Page::Village&&game.city==0&&hit(x,y,10,80,220,30))||(view.page==Page::Ruins&&hit(x,y,10,94,220,30))){view.objectiveReturn=view.page;view.page=Page::Campaign;say("");return;}
  if(view.page==Page::Journal){if(hit(x,y,14,272,102))view.page=Page::Menu;
    else if(hit(x,y,124,272,102)){view.objectiveReturn=Page::Journal;view.page=Page::Campaign;}
    else if(hit(x,y,14,224,102))menu.chapterIndex=(menu.chapterIndex+5)%6;
    else if(hit(x,y,124,224,102))menu.chapterIndex=(menu.chapterIndex+1)%6;say("");return;}
  if(view.page==Page::Campaign){
    if(game.campaignFlags&4){if(game.campaignEnding==2&&hit(x,y,14,224,212)){view.campaignScene=0;view.page=Page::Epilogue;say("");return;}if(hit(x,y,124,272,102)){view.page=view.objectiveReturn;say("");return;}if(hit(x,y,14,272,102)){view.page=Page::Journal;menu.chapterIndex=4;say("");}else if(hit(x,y,14,224,212)){unsigned city=rpg::contributionNextCity(game);if(game.city==city){menu.storyReturn=int(Page::Campaign);view.page=Page::People;}else {showMap();menu.destination=city;}say("");}return;}

    if(hit(x,y,14,198,212,24)){menu.storyReplay=true;menu.storyReturn=int(Page::Campaign);menu.storyIndex=0;view.page=Page::Prologue;say("");return;}
    if(hit(x,y,14,224,212)){
      if(!game.tutorial){helpReturn=view.objectiveReturn;view.page=Page::Help;}
      else if(game.campaignFlags&4){unsigned city=rpg::contributionNextCity(game);if(game.city==city){menu.storyReturn=int(Page::Campaign);view.page=Page::People;}else{showMap();menu.destination=city;}}
      else if((game.campaignFlags&3)==3){if(game.city==3){menu.storyReturn=int(Page::Campaign);view.page=Page::People;}else {showMap();menu.destination=3;}}
      else if(story::arconteKnown(game)){if(game.city==2){menu.storyReturn=int(Page::Campaign);view.page=Page::People;}else {showMap();menu.destination=2;}}
      else if(game.guardianDefeated){if(game.city==1)view.page=Page::DungeonEntry;else {showMap();menu.destination=1;}}
      else if(game.city==1)view.page=Page::Ruins;
      else if(game.p.level<5&&game.city==0)view.page=Page::Explore;
      else {showMap();menu.destination=1;}
    }
    else if(hit(x,y,14,272,102))view.page=Page::Journal;
    else if(hit(x,y,124,272,102))view.page=view.objectiveReturn;say("");return;
  }
  if(view.page==Page::People){if(hit(x,y,14,272,212))view.page=Page(menu.storyReturn);
    else for(unsigned i=0;i<3;++i)if(hit(x,y,14,78+i*54,212,48)){menu.personIndex=i;view.dialoguePage=0;view.dialogueAnimate=true;view.dialogueStartFrame=millis()/120;view.page=Page::Dialogue;break;}say("");return;}
  if(view.page==Page::CampaignTask){
    if(hit(x,y,14,272,102)){view.page=Page::Dialogue;say("");}
    else if(hit(x,y,124,272,102)){const char* err=rpg::startCampaign(game,view.campaignChoice);if(err)say(err);else {view.campaignScene=0;say("");savedTransition(rpg::campaignDonation(view.campaignChoice)?Page::ContributionResult:Page::Battle);}}return;
  }
  if(view.page==Page::CampaignResult){
    if(!hit(x,y,14,272,212))return;
    bool won=game.phase==rpg::Phase::Won,lost=game.phase==rpg::Phase::Lost;
    if(won&&view.campaignScene+1<(game.campaignStage>=8?story::finalePages(game,game.campaignStage):game.campaignStage>=4?story::contributionPages(game.campaignStage):rpg::campaignScenes(game.campaignStage))){++view.campaignScene;say("");return;}
    if(rpg::resolveCampaign(game)){view.campaignScene=0;say(won?"Descoberta registrada no diario":"A missao pode ser tentada de novo");savedTransition(lost?Page::Recovery:Page::People);menu.storyReturn=int(game.city==1?Page::Ruins:Page::Village);}return;
  }
  if(view.page==Page::Epilogue){if(hit(x,y,14,272,212)){if(view.campaignScene+1<story::finalePages(game,9))++view.campaignScene;else view.page=Page::Campaign;say("");}return;}
  if(view.page==Page::ContributionResult){if(hit(x,y,14,272,102)){if(view.campaignScene)--view.campaignScene;else view.page=Page::People;}else if(hit(x,y,124,272,102)){if(view.campaignScene+1<story::contributionPages(view.campaignChoice))++view.campaignScene;else view.page=Page::People;}say("");return;}
  if(view.page==Page::Dialogue){if(hit(x,y,8,8,224,210)&&view.dialogueAnimate){view.dialogueAnimate=false;say("");return;}if(((menu.personIndex==rpg::campaignPerson(rpg::campaignMission(game))&&rpg::campaignContact(game))||(game.city==3&&menu.personIndex==0&&(game.campaignFlags&4)&&game.campaignEnding!=2))&&hit(x,y,14,225,212,32)){view.campaignChoice=game.city==3&&menu.personIndex==0&&(game.campaignFlags&4)?rpg::finaleMission(game):rpg::campaignMission(game);view.page=Page::CampaignTask;say("");return;}if(hit(x,y,14,272,102)){if(view.dialoguePage)--view.dialoguePage;else view.page=Page::People;}else if(hit(x,y,124,272,102)){if(view.dialoguePage+1<story::conversationPages(story::conversation(game,menu.personIndex)))++view.dialoguePage;else {view.dialoguePage=0;view.page=Page::People;}}say("");return;}
  if(view.page==Page::Continent){if(hit(x,y,14,272,102))showMap();
    else if(hit(x,y,124,272,102)){if(!menu.regionIndex)showMap();else say("Mapa de regiao futura");}
    else if(hit(x,y,14,224,102))menu.regionIndex=(menu.regionIndex+7)%8;
    else if(hit(x,y,124,224,102))menu.regionIndex=(menu.regionIndex+1)%8;dirty=true;return;}

  if(view.page==Page::CampRoll){if(menu.rollReady&&hit(x,y,14,272,212)&&rpg::acceptCamp(game)){say(rpg::campSafe(game)?"Descanso seguro":"Inimigo da regiao!");savedTransition(currentPage());}return;}
  if(view.page==Page::CampSetup){
    if(hit(x,y,14,272,102)){view.page=game.city==1?Page::Ruins:Page::Explore;say("");}
    else if(hit(x,y,14,207,212,33)){if(!game.rations){say("Sem racoes");return;}menu.campRation=!menu.campRation;say("");}
    else if(hit(x,y,14,244,212,26)){if(!game.sleepKit){menu.campShopReturn=false;view.page=Page::CampKit;say("");}else {menu.campKit=!menu.campKit;say("");}}
    else if(hit(x,y,124,272,102)){auto err=rpg::startCamp(game,menu.campRation,menu.campKit);if(err)say(err);else {say("");savedTransition(Page::CampRoll);}}return;
  }
  if(view.page==Page::CampKit||view.page==Page::GearSell){bool selling=view.page==Page::GearSell;Page back=selling?Page::BagGear:menu.campShopReturn?Page::CityGoods:Page::CampSetup;
    if(hit(x,y,14,272,102)){view.page=back;say("");}
    else if(hit(x,y,124,272,102)){auto err=selling?rpg::sellGear(game,view.itemId):rpg::buySleepKit(game);if(err)say(err);else {if(!selling)menu.campKit=true;say(selling?"Item vendido":"Kit comprado");savedTransition(back);}}return;
  }
  if(view.page==Page::IslandEntry){
    if(hit(x,y,14,272,102)){showMap();return;}
    if(hit(x,y,124,272,102)){auto err=rpg::enterIsland(game);if(err)say(err);else {say("Explore o santuario");savedTransition(Page::Dungeon);}}return;
  }
  if(view.page==Page::DungeonEntry||view.page==Page::CrystalBuy||view.page==Page::DungeonExit){
    Page page=view.page;if(hit(x,y,14,272,102)){view.page=page==Page::DungeonExit?Page::Dungeon:page==Page::CrystalBuy?Page::CityGoods:Page::Ruins;say("");}
    else if(hit(x,y,124,272,102)){if(page==Page::DungeonExit){bool island=rpg::islandDungeon(game);if(rpg::leaveDungeon(game)){say("Saque preservado");savedTransition(island?Page::Map:Page::Ruins);}}
      else {const char* err=page==Page::CrystalBuy?rpg::buyCrystal(game):rpg::enterDungeon(game);if(err)say(err);else {say(page==Page::CrystalBuy?"Cristal comprado":"Explore e encontre o selo");savedTransition(page==Page::CrystalBuy?Page::CityGoods:Page::Dungeon);}}}return;}
  if(view.page==Page::DungeonVictory){
    if(hit(x,y,14,218,212)||hit(x,y,14,272,212)){bool exit=y>=272;
      bool island=rpg::islandDungeon(game);if(rpg::dungeonResolve(game)){if(exit)rpg::leaveDungeon(game);say(exit?"Saque preservado":"Bau liberado! Explore a sala");savedTransition(exit?(island?Page::Map:Page::Ruins):Page::Dungeon);}}return;
  }
  if(view.page==Page::DungeonLoot){
    if(hit(x,y,14,272,212)){view.page=Page::Dungeon;say("");return;}
    if(hit(x,y,14,218,212)){unsigned count=rpg::gearOwnedCount(game.owned),index=0;auto id=view.itemId;if(!id){view.page=Page::Bag;say("");return;}while(index<count&&rpg::gearOwnedAt(game.owned,index)!=id)++index;view.gearIndex=index/6;view.choice=index%6;view.page=Page::BagGear;say("");}return;
  }
  if(view.page==Page::BagGear){
    if(hit(x,y,10,278,70)){view.page=rpg::inDungeon(game)||game.phase!=rpg::Phase::Home?Page::Bag:Page::TownBag;view.choice=0;say("");return;}
    unsigned count=rpg::gearOwnedCount(game.owned),pages=std::max(1u,(count+5)/6);
    if(hit(x,y,10,200,68,24)||hit(x,y,162,200,68,24)){view.gearIndex=(view.gearIndex+(x<120?pages-1:1))%pages;view.choice=0;say("");return;}
    for(unsigned i=0;i<6;++i)if(hit(x,y,10+(i%3)*76,74+(i/3)*64,68,58)){view.choice=i;say("");return;}
    if(hit(x,y,158,278,72)){unsigned index=view.gearIndex*6+view.choice;if(index>=count){say("Slot vazio");return;}if(rpg::inDungeon(game)||game.phase!=rpg::Phase::Home){say("Venda na cidade");return;}view.itemId=rpg::gearOwnedAt(game.owned,index);view.page=Page::GearSell;say("");return;}
    if(hit(x,y,84,278,70)){unsigned index=view.gearIndex*6+view.choice;if(index>=count){say("Slot vazio");return;}
      if(game.phase!=rpg::Phase::Home){say("Equipe depois do combate");return;}
      auto err=rpg::equipGear(game,rpg::gearOwnedAt(game.owned,index));if(err)say(err);else {say("Equipamento alterado");savedTransition(Page::BagGear);}}return;
  }
  if(view.page==Page::Bag||view.page==Page::TownBag){
    Page back=rpg::inDungeon(game)?Page::Dungeon:view.page==Page::Bag?Page::Battle:view.bagReturn;
    if(panelUi::bagBack.contains(x,y)){view.page=back;say("");return;}
    if(panelUi::bagSlot(8).contains(x,y)){view.picksReturn=view.page;view.page=Page::Lockpicks;say("");return;}
    if(game.phase==rpg::Phase::Home&&panelUi::bagSlot(7).contains(x,y)){view.page=Page::Scrap;say("");return;}
    for(unsigned i=0;i<7;++i)if(panelUi::bagSlot(i).contains(x,y)){view.choice=i;say("");return;}
    if(panelUi::bagGear.contains(x,y)){view.gearIndex=0;view.choice=0;view.page=Page::BagGear;say("");return;}
    if(!panelUi::bagUse.contains(x,y))return;
    if(view.choice>=2){if(view.choice==6&&game.phase==rpg::Phase::Home&&!rpg::inDungeon(game)){openCamp();return;}say(view.choice==2?"Cristal: entrada nas Ruinas":"Racao e kit: use ao acampar");return;}
    bool mana=view.choice==1;
    if(game.phase==rpg::Phase::Hero){action(mana?rpg::Action::Mana:rpg::Action::Life);return;}
    if(game.phase!=rpg::Phase::Home){say("Termine o turno primeiro");return;}
    auto& amount=mana?game.p.mana:game.p.life;auto& value=mana?game.p.mp:game.p.hp;unsigned heal=rpg::recovery(value,mana?game.p.maxmp:game.p.maxhp,mana);
    if(!amount||!heal){say(!amount?"Sem pocoes":"Ja esta cheio");return;}--amount;value+=heal;say("Pocao usada");savedTransition(view.page);return;
  }
  if(view.page==Page::EnemyInfo){if(hit(x,y,14,278,212))view.page=view.enemyReturn;say("");return;}
  if(view.page==Page::DungeonMenu){
    if(game.phase==rpg::Phase::Hero&&hit(x,y,14,0,212,32)){view.enemyReturn=Page::DungeonMenu;view.page=Page::EnemyInfo;say("");return;}
    if(hit(x,y,14,278,212)){view.page=Page::Dungeon;say("");return;}
    if(hit(x,y,14,230,212)){if(game.phase==rpg::Phase::Home){view.page=Page::DungeonExit;say("");}else say("Termine o combate primeiro");return;}
    if(game.dndProgression&&hit(x,y,14,40,212,34)){view.powersReturn=Page::DungeonMenu;view.powerIndex=0;view.page=Page::Powers;say("");return;}
    if(hit(x,y,14,134,212)){view.choice=0;view.page=Page::Bag;say("");return;}
    int a=hit(x,y,14,86,102)?1:hit(x,y,124,86,102)?2:hit(x,y,14,182,212)?5:-1;
    if(a<0)return;if(game.phase==rpg::Phase::Hero)action(rpg::Action(a));else say("Use durante o combate");return;}
  if(view.page==Page::Dungeon){
    if(game.phase==rpg::Phase::Enemy)return;
    if(hit(x,y,160,280,76,38)){view.page=Page::DungeonMenu;say("");return;}
    if(hit(x,y,4,280,152,38)){view.choice=0;view.page=Page::Bag;say("");return;}
    if(y<172){
      if(game.phase==rpg::Phase::Hero){action(rpg::Action::Attack);return;}
      if(game.phase!=rpg::Phase::Home){bool lost=game.phase==rpg::Phase::Lost;rpg::dungeonResolve(game);say("");savedTransition(lost?Page::Recovery:rpg::inDungeon(game)?Page::Dungeon:Page::Ruins);return;}
      if(rpg::dungeonCell(game,rpg::dungeonX(game),rpg::dungeonY(game))=='E'){view.page=Page::DungeonExit;say("");return;}
      int enemy=rpg::dungeonEnemyAhead(game);if(enemy>=0){rpg::begin(game,rpg::dungeonSpawn(game,enemy).id);say("Seu turno");savedTransition(Page::Dungeon);return;}
      if(rpg::dungeonUseLever(game)){say("Um estrondo ecoa ao longe");savedTransition(Page::Dungeon);return;}
      uint8_t before=game.dungeonLoot;uint32_t oldOwned=game.owned;const char* notice=rpg::dungeonCollect(game);view.itemId=0;for(uint8_t id=1;id<=rpg::GEAR_COUNT;++id)if(rpg::gearOwns(game.owned,id)&&!rpg::gearOwns(oldOwned,id))view.itemId=id;if(before!=game.dungeonLoot){say(notice);savedTransition((game.dungeonLoot&rpg::dungeonChestBit(game))&&!(before&rpg::dungeonChestBit(game))?Page::DungeonLoot:Page::Dungeon);return;}if(notice){say(notice);return;}
      if(rpg::dungeonStairs(game)){say("Escadas: novo andar");savedTransition(Page::Dungeon);return;}
      if(rpg::islandDungeon(game)&&rpg::dungeonCell(game,rpg::dungeonX(game),rpg::dungeonY(game))=='S'){say("A escada esta bloqueada");return;}
      say("Nada para interagir aqui");return;
    }
    if(game.phase!=rpg::Phase::Home)return;
    int forward=0,side=0,turn=0;
    if(hit(x,y,4,202,74,35))turn=-1;else if(hit(x,y,82,202,74,35))forward=1;else if(hit(x,y,160,202,76,35))turn=1;
    else if(hit(x,y,4,241,74,35))side=-1;else if(hit(x,y,82,241,74,35))forward=-1;else if(hit(x,y,160,241,76,35))side=1;else return;
    auto trapsBefore=game.islandTraps;const char* err=rpg::dungeonMove(game,forward,side,turn);if(err)say(err);else {if(trapsBefore!=game.islandTraps){char msg[48];snprintf(msg,sizeof(msg),game.damage?"Armadilha: -%u HP":"Armadilha evitada!",game.damage);say(msg);}else say(game.phase==rpg::Phase::Hero?"Inimigo! Seu turno":"");savedTransition(Page::Dungeon);}return;
  }
  if(view.page==Page::NetworkTest){if(updateInfo.busy)return;if(hit(x,y,14,272,102)){view.page=Page::Updates;dirty=true;}else if(hit(x,y,124,272,102)){startNetworkTest();dirty=true;}return;}
  if(view.page==Page::TravelRoll){if(menu.rollReady&&hit(x,y,14,272,212)&&rpg::acceptTrip(game)){say("");savedTransition(currentPage());}return;}
  if(view.page==Page::Club||view.page==Page::ClubBattle||view.page==Page::ClubResult){clubTap(x,y);return;}
  if(view.page==Page::Updates){if(updateInfo.busy)return;
    if(updateInfo.state==updater::State::Available||updateInfo.canResume){if(hit(x,y,14,272,102)){updateInfo=updater::Info{};resetUpdateScreen();view.page=Page::Settings;dirty=true;}else if(hit(x,y,14,218,212)){view.page=Page::NetworkTest;startNetworkTest();dirty=true;}else if(hit(x,y,124,272,102)){if(!startUpdate(true))say("Conecte o Wi-Fi para atualizar");dirty=true;}}
    else if(hit(x,y,14,173,212)){view.page=Page::NetworkTest;startNetworkTest();dirty=true;}else if(hit(x,y,14,218,212)){if(!startUpdate(false)){updateInfo.state=updater::State::Error;snprintf(updateInfo.message,sizeof(updateInfo.message),"Conecte o Wi-Fi primeiro");}dirty=true;}
    else if(hit(x,y,14,272,212)){view.page=Page::Settings;dirty=true;}return;}
  if(view.page==Page::Guide){
    if(launchUi::back.contains(x,y)){view.page=view.guideReturn;say("");}
    else if(launchUi::previous.contains(x,y)||launchUi::next.contains(x,y)){view.guideIndex=(view.guideIndex+(x<120?launchUi::guideCount-1:1))%launchUi::guideCount;say("");}return;
  }
  if((view.page==Page::Menu&&hit(x,y,14,281,102,28))||(view.page==Page::Settings&&hit(x,y,124,278,102))||(view.page==Page::Help&&hit(x,y,14,233,212,27))||(view.page==Page::Skills&&hit(x,y,14,143,48,29))){view.guideReturn=view.page;view.guideIndex=view.page==Page::Skills?1:0;view.page=Page::Guide;say("");return;}
  if(view.page==Page::Bestiary){if(hit(x,y,14,278,212)){view.page=Page::Menu;say("");}else if(hit(x,y,14,114,102,28)||hit(x,y,124,114,102,28)){view.bestiaryIndex=rpg::bestiaryStep(game,view.bestiaryIndex,x<120?-1:1);say("");}return;}
  if(view.page==Page::Menu&&hit(x,y,124,281,102,28)){if(journal.blocked||journal.active<0){say("Escolha ou crie um personagem");return;}view.bestiaryIndex=rpg::bestiaryFirst(game);view.page=Page::Bestiary;say("");return;}
  if(view.page==Page::Menu){int choice=scenicUi::menuChoice(x,y);
    if(choice==0){if(journal.blocked||journal.active<0)say("Escolha ou crie um personagem");else showMap();}
    else if(choice==1){refreshSlots();menu.newGameSlots=false;menu.creationFromTitle=false;menu.slotsReturn=int(Page::Menu);view.page=Page::Slots;menu.notice="";dirty=true;}
    else if(choice==2){openTitle();}
    else if(choice==3){if(!journal.blocked&&journal.active>=0){menu.chapterIndex=story::knownChapter(game);view.page=Page::Journal;say("");}else say("Escolha ou crie um personagem");}
    else if(choice==4){if(!journal.blocked&&journal.active>=0){menu.storyReturn=int(Page::Menu);view.page=Page::People;say("");}else say("Escolha ou crie um personagem");}
    else if(choice==5){if(journal.blocked||journal.active<0)say("Escolha ou crie um personagem");else {view.page=Page::Letters;menu.eventReturn=int(Page::Menu);dirty=true;}}
    else if(choice==6){view.page=journal.blocked?Page::Blocked:journal.active<0?Page::Race:currentPage();say("");}return;
  }
  if(view.page==Page::Slots){if(hit(x,y,14,272,212)){if(menu.slotsReturn==int(Page::Title))openTitle();else {view.page=Page::Menu;say("");}return;}for(uint8_t i=0;i<3;++i)if(hit(x,y,14,55+i*61,212,56)){menu.slotChoice=i;view.page=Page::SlotConfirm;menu.notice="";dirty=true;return;}return;}
  if(view.page==Page::SlotConfirm){if(hit(x,y,14,272,212)){view.page=Page::Slots;dirty=true;}
    else if(hit(x,y,14,178,212)&&menu.slots[menu.slotChoice]!=rpg::Load::Blocked&&(!menu.newGameSlots||menu.slots[menu.slotChoice]==rpg::Load::Empty))selectSlot(menu.slotChoice);
    else if(hit(x,y,14,224,212)&&menu.slots[menu.slotChoice]!=rpg::Load::Empty){view.page=Page::DeleteSlot;dirty=true;}return;}
  if(view.page==Page::DeleteSlot){if(hit(x,y,14,272,102)){view.page=Page::SlotConfirm;dirty=true;}
    else if(hit(x,y,124,272,102)){uint8_t old=backend.slot;backend.slot=menu.slotChoice;bool ok=backend.remove();backend.slot=old;if(!ok){menu.notice="Falha; personagem preservado";dirty=true;return;}refreshSlots();menu.notice="Slot apagado";
      if(menu.newGameSlots){selectSlot(menu.slotChoice);return;}
      if(menu.slotChoice==old){game=rpg::Game{};journal.load(game);view.choice=0;view.page=Page::Race;say("");}else{view.page=Page::Slots;dirty=true;}}return;}
  if(view.page==Page::Letters){if(hit(x,y,14,272,212)){view.page=Page::Menu;dirty=true;}else if(hit(x,y,14,216,212)){if(game.eventStage==2){view.page=currentPage();dirty=true;}else if(game.eventStage==1){menu.notice="";menu.letterStarted=millis();view.page=Page::Letter;dirty=true;}}return;}
  if(view.page==Page::Letter){if(uint32_t(millis()-menu.letterStarted)<450)return;
    if(hit(x,y,14,222,212)){view.page=Page(menu.eventReturn);say("");return;}
    if(hit(x,y,14,272,102)){view.page=Page::LetterRefuse;dirty=true;return;}
    if(hit(x,y,124,272,102)){if(arena.opened||updateInfo.busy){say("Termine a acao atual primeiro");return;}const char* err=rpg::acceptEvent(game,menu.eventReturn);if(err){menu.notice=err;dirty=true;return;}menu.notice="";menu.letterStarted=millis();say("");savedTransition(Page::EventTravel);}return;}
  if(view.page==Page::LetterRefuse){if(hit(x,y,14,272,102)){menu.letterStarted=millis()-450;view.page=Page::Letter;dirty=true;}else if(hit(x,y,124,272,102)&&rpg::refuseEvent(game)){say("");savedTransition(Page::Letters);}return;}
  if(view.page==Page::EventTravel)return;
  if(view.page==Page::EventResult){if(hit(x,y,14,272,212)){auto back=Page(game.eventOriginPage);bool lost=game.phase==rpg::Phase::Lost;if(rpg::finishEvent(game)){say("Missao encerrada");savedTransition(lost?Page::Recovery:back);}}return;}
  if(view.page==Page::TimeSettings){
    if(hit(x,y,14,272,102)){view.page=Page::Settings;dirty=true;return;}
    if(hit(x,y,124,272,102)){prepareClockEdit();view.page=Page::TimeEdit;dirty=true;return;}
    bool ok=true;if(hit(x,y,14,113,102)||hit(x,y,124,113,102)){int offset=std::max(-12,std::min(14,int(menu.utcOffset)+(x<120?-1:1)));ok=saveClockConfig(menu.clockAutomatic,offset,menu.dayCycle);}
    else if(hit(x,y,14,159,212))ok=saveClockConfig(!menu.clockAutomatic,menu.utcOffset,menu.dayCycle);
    else if(hit(x,y,14,205,212))ok=saveClockConfig(menu.clockAutomatic,menu.utcOffset,!menu.dayCycle);else return;
    menu.notice=ok?"Ajuste salvo":"Falha; tente novamente";tickClock();dirty=true;return;
  }
  if(view.page==Page::TimeEdit){if(hit(x,y,14,272,102)){view.page=Page::TimeSettings;dirty=true;return;}
    if(hit(x,y,124,272,102)){if(applyClockDraft()){view.page=Page::TimeSettings;menu.notice="Hora manual / enquanto ligado";tickClock();}else say("Falha ao aplicar a hora");dirty=true;return;}
    if(hit(x,y,14,181,102)||hit(x,y,124,181,102))adjustClockDraft(x<120?-1:1);
    else if(hit(x,y,14,227,102))menu.clockField=(menu.clockField+4)%5;else if(hit(x,y,124,227,102))menu.clockField=(menu.clockField+1)%5;else return;dirty=true;return;
  }
  if(view.page==Page::Settings){if(hit(x,y,14,78,102)||hit(x,y,124,78,102)){int level=menu.brightness+(x<120?-10:10);level=std::max(10,std::min(100,level));menu.notice=saveBrightness(level)?"":"Falha ao salvar brilho";dirty=true;}
    else if(hit(x,y,14,130,212)){view.page=Page::Wifi;showSavedNetworks();menu.notice="";dirty=true;}
    else if(hit(x,y,14,176,102)){view.page=Page::Card;say("");}
    else if(hit(x,y,124,176,102)){view.page=Page::Tests;dirty=true;}
    else if(hit(x,y,124,222,102)){view.page=Page::TimeSettings;menu.notice="";tickClock();dirty=true;}
    else if(hit(x,y,14,222,102)){view.page=Page::Updates;resetUpdateScreen();dirty=true;}
    else if(hit(x,y,14,278,212)){openTitle();}return;}
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
  if(view.page==Page::Recovery){
    if(hit(x,y,14,98,212)){view.bagReturn=Page::Recovery;view.choice=0;view.page=Page::TownBag;say("");}
    else if(hit(x,y,14,148,212)){openCamp();}
    else if(hit(x,y,14,198,212)){showMap();}
    else if(launchUi::back.contains(x,y)){view.page=game.city==1?Page::Ruins:Page::Explore;say("");}return;
  }
  if(view.page==Page::TravelConfirm){
    if(launchUi::cancel.contains(x,y)){view.page=Page::Map;say("Viagem cancelada");}
    else if(launchUi::confirm.contains(x,y)){auto err=rpg::tripError(game,view.tripDestination);if(!err)err=rpg::prepareTrip(game,view.tripDestination);if(err)say(err);else {say("");savedTransition(Page::TravelRoll);}}return;
  }
  if(view.page==Page::Map){if(rpg::islandUnlocked(game)&&hit(x,y,12,132,40,44)){view.page=Page::IslandEntry;say("");return;}if(scenicUi::atlasChoice(x,y)){menu.regionIndex=0;view.page=Page::Continent;say("");return;}for(uint8_t i=0;i<4;++i)if(hit(x,y,places[i].x-30,places[i].y-14,60,42)){menu.destination=i;dirty=true;return;}
    if(scenicUi::mapButtons[1].contains(x,y)){view.page=Page::Menu;say("");}
    else if(scenicUi::mapButtons[0].contains(x,y)){if(menu.destination==game.city){view.page=game.city==1?Page::Ruins:Page::Village;say("");}
      else {const char* err=rpg::tripError(game,menu.destination);if(err)say(err);else {view.tripDestination=menu.destination;view.page=Page::TravelConfirm;say("");}}}
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
  if(view.page==Page::Race){if(hit(x,y,14,263,102)){if(menu.creationFromTitle)openTitle();else {view.page=Page::Menu;say("");}}else if(hit(x,y,124,263,102)){view.page=Page::Choose;dirty=true;}else for(unsigned i=0;i<4;++i)if(hit(x,y,14+(i%2)*110,46+(i/2)*104,102,98)){menu.draftRace=i;dirty=true;}return;}
  if(view.page==Page::Clothes){if(hit(x,y,14,272,102)){view.page=Page::Choose;dirty=true;}else if(hit(x,y,124,272,102)){if(journal.blocked||journal.active>=0){say("Crie em um slot vazio");return;}menu.sessionStarted=true;game=rpg::create(view.choice,esp_random());game.originStory=true;game.race=menu.draftRace;game.shirt=menu.draftShirt;game.trousers=menu.draftPants;menu.storyIndex=0;menu.storyReplay=false;savedTransition(Page::Prologue);}else if(hit(x,y,14,177,102)||hit(x,y,124,177,102)){menu.draftShirt=(menu.draftShirt+(x<120?7:1))%8;dirty=true;}else if(hit(x,y,14,224,102)||hit(x,y,124,224,102)){menu.draftPants=(menu.draftPants+(x<120?7:1))%8;dirty=true;}return;}
  if(view.page==Page::Guild){if(hit(x,y,14,272,212)){view.page=Page::Village;say("");}else if(hit(x,y,14,170,212)){view.page=game.guildMember?Page::GuildMissions:Page::GuildJoin;say("");}else if(hit(x,y,14,218,212)){if(!game.guildMember)say("Cadastre-se primeiro");else if(game.p.level<5)say("Clube: nivel 5 necessario");else {if(openClub()){view.page=Page::Club;say("");}else say("Nao foi possivel abrir o radio");}}return;}
  if(view.page==Page::GuildJoin){if(hit(x,y,14,272,102)){view.page=Page::Guild;say("");}else if(hit(x,y,124,272,102)){if(!rpg::joinGuild(game)){say("Cadastro concluido");savedTransition(Page::Guild);}else say("Ouro insuficiente para cadastro");}return;}
  if(view.page==Page::Tavern){if(hit(x,y,14,220,212)){view.page=Page::Guild;say("");}else if(hit(x,y,14,272,212)){view.page=game.city==1?Page::Ruins:Page::Village;say("");}return;}
  if(view.page==Page::Choose){
    if(hit(x,y,0,0,36)){if(menu.creationFromTitle)openTitle();else {view.page=Page::Menu;say("");}return;}
    if(hit(x,y,14,211,102))view.choice=(view.choice+3)%4;
    else if(hit(x,y,124,211,102))view.choice=(view.choice+1)%4;
    else if(hit(x,y,14,263,212)){view.page=Page::Clothes;}dirty=true;return;
  }
  if(view.page==Page::Help){if(hit(x,y,14,268,212)){say("");if(!game.tutorial){game.tutorial=true;savedTransition(helpReturn);}else{view.page=helpReturn;dirty=true;}}return;}
  if(view.page==Page::Home){int choice=scenicUi::homeChoice(x,y);
    if(choice==0){showMap();}
    else if(choice==1){if(game.p.hp!=game.p.maxhp||game.p.mp!=game.p.maxmp||rpg::powersSpent(game)){rpg::rest(game);say("HP, MP e poderes recuperados");savedTransition(Page::Home);}else say("Voce ja esta recuperado");}
    else if(choice==2){view.page=Page::Menu;say("");}return;
  }
  if(view.page==Page::Ruins){int choice=scenicUi::ruinsChoice(x,y);
    if(choice==0){menu.storyReturn=int(Page::Ruins);view.page=Page::People;say("");return;}
    if(choice==1){view.page=Page::DungeonEntry;say("");return;}
    if(choice==5){showMap();return;}if(choice==6){view.page=Page::Market;say("");return;}
    if(choice==4){openCamp();return;}
    bool boss=choice==2;if(!boss&&choice!=3)return;
    if(boss&&game.ruinsWins<3){say("Venca 3 encontros primeiro");return;}
    if(boss?rpg::explore(game,true):rpg::startDiscovery(game,menu.clockValid&&menu.dayCycle&&menu.worldPeriod==worldClock::Period::Night)){say(game.discovery?"":"Seu turno");savedTransition(currentPage());}return;
  }
  if(view.page==Page::Village&&game.city==0){
    int choice=-1;for(int i=0;i<9;++i)if(panelUi::cityButtons[i].contains(x,y))choice=i;
    if(choice==0){menu.storyReturn=int(Page::Village);view.page=Page::People;}
    else if(choice==1)view.page=Page::Market;
    else if(choice==2){view.bagReturn=Page::Village;view.choice=0;view.page=Page::TownBag;}
    else if(choice==3)view.page=Page::Character;
    else if(choice==4)view.page=Page::Explore;
    else if(choice==5)view.page=Page::Guild;
    else if(choice==6){openCamp();return;}
    else if(choice==7){showMap();return;}
    else if(choice==8)view.page=Page::Menu;
    say("");return;
  }
  if(view.page==Page::Village){
    if(hit(x,y,14,78,212,26)){menu.storyReturn=int(Page::Village);view.page=Page::People;say("");return;}
    if(hit(x,y,14,126,102)){view.page=Page::Market;say("");}
    else if(hit(x,y,124,126,102)){view.page=Page::Inventory;say("");}
    else if(hit(x,y,14,170,102)){view.page=Page::Character;dirty=true;}
    else if(hit(x,y,124,170,102)){view.page=Page::Explore;say("");}
    else if(hit(x,y,14,214,102)){view.page=Page::Guild;say("");}
    else if(hit(x,y,124,214,102)){openCamp();}
    else if(hit(x,y,14,258,102)){showMap();}
    else if(hit(x,y,124,258,102)){view.page=Page::Menu;say("");}return;
  }
  if(view.page==Page::Explore){if(hit(x,y,14,270,102)){openCamp();return;}if(hit(x,y,124,270,102)){view.page=Page::Village;say("");}else if(hit(x,y,14,220,212)&&rpg::startDiscovery(game,menu.clockValid&&menu.dayCycle&&menu.worldPeriod==worldClock::Period::Night)){say(game.discovery?"":"Seu turno");savedTransition(currentPage());}return;}
  if(view.page==Page::CityGoods){if(hit(x,y,14,181,212,30)){view.picksReturn=Page::CityGoods;view.page=Page::Lockpicks;say("");return;}if(hit(x,y,124,270,102)){menu.campShopReturn=true;view.page=Page::CampKit;say("");return;}if(game.city==1&&hit(x,y,124,218,102)){view.page=Page::CrystalBuy;say("");return;}if(hit(x,y,14,270,102)){view.page=Page::Market;say("");}else if(hit(x,y,14,218,212)){view.page=Page::GoodsBuy;say("");}return;}
  if(view.page==Page::GoodsBuy){if(hit(x,y,14,270,102)){view.page=Page::CityGoods;say("");}else if(hit(x,y,124,270,102)){auto err=rpg::buyGood(game);if(err)say(err);else {say("Suprimento comprado");savedTransition(Page::CityGoods);}}return;}
  if(view.page==Page::GuildMissions){
    if(!game.guildMember){view.page=Page::Guild;say("Cadastre-se primeiro");return;}
    if(hit(x,y,14,272,212)){view.page=Page::Guild;say("");return;}
    for(unsigned slot=0;slot<3;++slot)if(hit(x,y,14,124+slot*44,212)){view.questChoice=rpg::contractVisibleOffer(game,slot);view.page=Page::Contract;say("");return;}return;
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
      uint16_t xp=rpg::contractXp(game,game.questId,game.questLevel),gold=rpg::contractGold(game.questId,game.questLevel);
      const char* err=view.questAction==0?rpg::acceptQuest(game,view.questChoice):view.questAction==1?rpg::claimQuest(game):rpg::abandonQuest(game);
      if(err){say(err);return;}
      if(view.questAction==1){snprintf(message,sizeof(message),"+%u XP / +%u ouro",xp,gold);view.message=message;}
      else say(view.questAction==0?"Missao aceita":"Missao abandonada");savedTransition(Page::GuildMissions);
    }return;
  }
  if(view.page==Page::OathConfirm){
    if(powersUi::back.contains(x,y)){view.page=Page::Powers;say("");return;}
    if(powersUi::use.contains(x,y)){auto err=rpg::swearDevotion(game);if(err)say(err);else {say("Juramento da Devocao firmado");savedTransition(Page::Powers);}}return;
  }
  if(view.page==Page::Powers){
    if(powersUi::back.contains(x,y)){view.page=view.powersReturn;say("");return;}
    unsigned count=powersUi::count(game.p.cls);if(!game.dndProgression||!count)return;
    if(powersUi::previous.contains(x,y)||powersUi::next.contains(x,y)){view.powerIndex=(view.powerIndex+(x<120?count-1:1))%count;say("");return;}
    if(powersUi::use.contains(x,y)){if(powersUi::passive(game.p.cls,view.powerIndex)){say("Efeito passivo: uso automatico");return;}auto a=powersUi::action(game.p.cls,view.powerIndex);
      if(game.p.cls==1&&!game.oath&&game.p.level>=3&&game.phase==rpg::Phase::Home&&(a==rpg::Action::SacredWeapon||a==rpg::Action::TurnUndead)){view.page=Page::OathConfirm;say("");return;}
      if(game.phase==rpg::Phase::Hero){action(a);return;}
      auto err=rpg::usePower(game,a);if(err)say(err);else {say(game.p.cls==2?"Segundo folego: HP recuperado":"Impor as maos: HP recuperado");savedTransition(Page::Powers);}}return;
  }
  if(view.page==Page::Progression){
    if(!game.dndProgression){if(hit(x,y,14,278,212,32)){view.page=Page::Character;say("");}return;}
    if(hit(x,y,14,94,102,32)){view.evolutionLevel=std::max(1,int(view.evolutionLevel)-1);say("");}
    else if(hit(x,y,124,94,102,32)){view.evolutionLevel=std::min(20,int(view.evolutionLevel)+1);say("");}
    else if(hit(x,y,14,263,102,32)){view.page=Page::Evolution;say("");}
    else if(hit(x,y,124,263,102,32)){view.powersReturn=Page::Progression;view.powerIndex=0;view.page=Page::Powers;say("");}
    else if(hit(x,y,10,298,220,22)){view.page=Page::Character;say("");}return;
  }
  if(view.page==Page::AttributeInfo){if(hit(x,y,14,278,102,32)){view.page=Page::Evolution;say("");}else if(hit(x,y,124,278,102,32)){auto err=view.attributeIndex==6?rpg::learnTough(game):rpg::improveAttribute(game,view.attributeIndex);if(err)say(err);else {say("Evolucao confirmada");savedTransition(Page::Evolution);}}return;}
  if(view.page==Page::Evolution){
    if(hit(x,y,124,92,102,28)){view.powersReturn=Page::Evolution;view.powerIndex=0;view.page=Page::Powers;say("");return;}
    if(hit(x,y,14,272,102)){view.page=Page::Character;say("");}
    else if(hit(x,y,124,272,102)){view.attributeIndex=6;view.page=Page::AttributeInfo;say("");}
    else if(hit(x,y,14,92,102)){view.evolutionLevel=std::min(20u,unsigned(game.p.level)+1);view.page=Page::Progression;say("");}
    
    else for(unsigned i=0;i<6;++i)if(hit(x,y,14+(i%2)*110,156+(i/2)*34,102,30)){view.attributeIndex=i;view.page=Page::AttributeInfo;say("");}return;
  }
  if(view.page==Page::Character){if(panelUi::evolution.contains(x,y)){view.evolutionLevel=std::min(20u,unsigned(game.p.level)+1);view.page=Page::Progression;say("");return;}if(hit(x,y,70,277,101,24)){view.page=game.city==1?Page::Ruins:Page::Village;say("");}return;}
  if(view.page==Page::Market||view.page==Page::Inventory){
    bool shop=view.page==Page::Market;
    if(!shop&&hit(x,y,14,272,102)){view.page=Page::Menu;say("");return;}
    if(hit(x,y,14,shop?138:180,212)){if(!shop)view.bagReturn=Page::Inventory;view.page=shop?Page::Shop:Page::TownBag;say("");}
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
    if(hit(x,y,14,270,102)){view.page=Page::Market;say("");return;}
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
  if(view.page==Page::Result){if(game.campStage==2){if(hit(x,y,14,268,212)&&rpg::resolveCamp(game)){say(game.campStage?"Descanso depois da batalha":"Acampamento interrompido");savedTransition(game.campStage?Page::CampRest:game.phase==rpg::Phase::Home&&game.p.hp==1?Page::Recovery:game.city==1?Page::Ruins:Page::Explore);}return;}if(hit(x,y,14,268,212)){bool lost=game.phase==rpg::Phase::Lost;if(rpg::home(game)){say("");savedTransition(lost?Page::Recovery:game.tripStage?currentPage():game.city==1?Page::Ruins:Page::Explore);}}return;}
  if(game.phase!=rpg::Phase::Hero)return;
  if(view.page==Page::Skills){
    if(game.dndProgression&&hit(x,y,26,54,188,28)){view.powersReturn=Page::Skills;view.powerIndex=0;view.page=Page::Powers;say("");return;}
    if(hit(x,y,14,172,212))action(rpg::Action::Offensive);
    else if(hit(x,y,14,218,212))action(rpg::Action::Defensive);
    else if(hit(x,y,14,270,212)){view.page=Page::Battle;say("Seu turno");}return;
  }
  if(view.page!=Page::Battle)return;
  if(game.phase==rpg::Phase::Hero&&hit(x,y,10,0,220,90)){view.enemyReturn=Page::Battle;view.page=Page::EnemyInfo;say("");return;}
  if(panelUi::battleButtons[0].contains(x,y))action(rpg::Action::Attack);
  else if(panelUi::battleButtons[1].contains(x,y)){view.page=Page::Skills;say("");}
  else if(panelUi::battleButtons[2].contains(x,y)){view.choice=0;view.page=Page::Bag;say("");}
  else if(panelUi::battleButtons[3].contains(x,y))action(rpg::Action::Flee);
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
  bootTitle();
  lastActivity=millis();enemyAt=millis()+1200;
  if(lcdReady){paint(millis());applyBrightness();dirty=false;}
  Serial.printf("RPG POKET 2.0 2026.10.07-titulo1 LCD=%d touch=%d flash=%u PSRAM=%u load=%u\n",lcdReady,touchReady,ESP.getFlashChipSize(),ESP.getPsramSize(),unsigned(journal.active>=0));
}
void loop(){
  uint32_t now=millis();
  menu.frameMs=frameMax;menu.pollMs=maxPollGap;menu.loopMs=loopGap;menu.touches=acceptedTouches;menu.touchErrors=busErrors+shortReads+countErrors+rangeErrors;
  if(loopAt)loopGap=std::max(loopGap,uint32_t(now-loopAt));loopAt=now;
  if(tickUpdater())dirty=true;
  tickClub(now);
  if(!arena.opened&&uint32_t(now-wifiPollAt)>=250){wifiPollAt=now;if(tickWifi())dirty=true;}
  if(menu.journey.tick(now)){rpg::arriveTrip(game);pendingTouch=false;gate=TouchGate{};savedTransition(game.city==1?Page::Ruins:Page::Village);}
  if((view.page==Page::TravelRoll||view.page==Page::CampRoll)&&!menu.rollReady&&uint32_t(now-menu.rollStarted)>=1800){menu.rollReady=true;pendingTouch=false;gate=TouchGate{};dirty=true;}
  if(view.page==Page::CampRest&&game.campStage==3&&uint32_t(now-menu.campStarted)>=4500){rpg::finishCamp(game);say("Descanso concluido");savedTransition(game.city==1?Page::Ruins:Page::Explore);}
  if(view.page==Page::EventTravel&&uint32_t(now-menu.letterStarted)>=700){view.page=currentPage();pendingTouch=false;gate=TouchGate{};dirty=true;}
  if(combatFx.expire(now)){view.page=currentPage();dirty=true;}
  if(view.touchFeedback&&int32_t(now-touchFeedbackUntil)>=0){view.touchFeedback=false;dirty=true;}
  if(!touchReady&&uint32_t(now-reidentifyAt)>=1000){reidentifyAt=now;uint8_t id=0;touchReady=readRegs(0xa7,&id,1)&&id==0xb6;}
  sampleTouch();
  if(pendingTouch){pendingTouch=false;if(pendingPage==view.page&&pendingPhase==game.phase)tapped(pendingX,pendingY);}
  now=millis();
  if(menu.clockValid&&uint32_t(now-menu.eventCheckAt)>=1000){menu.eventCheckAt=now;if(canOfferEvents()&&!updateInfo.busy&&!arena.opened&&view.page!=Page::SaveError&&view.page!=Page::Letter&&view.page!=Page::LetterRefuse&&rpg::offerEvent(game,menu.eventDay,menu.eventHour)){say("Nova carta de Maelis");savedTransition(view.page);}}
  tickIdleClock(now,updateInfo.busy||arena.opened||menu.connecting||menu.scanning||menu.journey.active||combatFx.active()||view.page==Page::EventTravel||view.page==Page::Letter||view.page==Page::LetterRefuse||view.page==Page::CampRest||(view.page==Page::CampRoll&&!menu.rollReady)||(view.page==Page::TravelRoll&&!menu.rollReady)||((view.page==Page::Battle||view.page==Page::Dungeon)&&game.phase==rpg::Phase::Enemy));
  if(uint32_t(now-clockPollAt)>=1000){clockPollAt=now;auto before=menu.worldPeriod;bool wasValid=menu.clockValid;
    bool safe=game.phase==rpg::Phase::Home&&!game.tripStage&&!game.campStage&&!combatFx.active()&&!arena.opened;
    bool changed=tickClock(safe);if((changed&&(view.page==Page::Clock||view.page==Page::TimeSettings))||before!=menu.worldPeriod||wasValid!=menu.clockValid)dirty=true;}
  if(!combatFx.active()&&(view.page==Page::Battle||view.page==Page::Dungeon)&&game.phase==rpg::Phase::Enemy&&int32_t(now-enemyAt)>=0){
    auto intent=rpg::enemyIntent(game);rpg::enemy(game);snprintf(message,sizeof(message),game.dodge?"Voce esquivou!":game.crit?"Critico inimigo! -%u HP":"Inimigo causou %u de dano",game.damage);
    if(intent==rpg::Intent::Prepare)snprintf(message,sizeof(message),"Inimigo prepara um golpe forte");else if(intent==rpg::Intent::Mend)snprintf(message,sizeof(message),"Inimigo recomposto: %u HP",game.enemyHp);else if(intent==rpg::Intent::Drain&&!game.dodge)snprintf(message,sizeof(message),"Drenagem: -%u HP; confira MP",game.damage);
    view.message=message;savedTransition(currentPage());beginEffect(intent==rpg::Intent::Mend?Effect::Radiant:intent==rpg::Intent::Prepare?Effect::Shield:game.enemyId==5?Effect::Projectile:Effect::Slash,intent!=rpg::Intent::Mend&&intent!=rpg::Intent::Prepare);
  }
  if(lcdReady&&(dirty||(((storyPaperPage(view.page)&&view.paperAnimate&&unsigned(now/120-view.paperStartFrame)<7)||(view.page==Page::Dialogue&&view.dialogueAnimate&&unsigned(now/120-view.dialogueStartFrame)<7)||view.page==Page::Title||view.page==Page::Home||view.page==Page::Menu||view.page==Page::Ruins||view.page==Page::Battle||view.page==Page::Dungeon||view.page==Page::CampRoll||view.page==Page::EventTravel||view.page==Page::Letter||view.page==Page::LetterRefuse||view.page==Page::CampRest||view.page==Page::Travel||view.page==Page::TravelRoll||(view.page==Page::Clock&&game.eventStage==1)||view.page==Page::Letter||view.page==Page::EventTravel||view.page==Page::Tests||view.page==Page::Updates)&&uint32_t(now-drawAt)>=(combatFx.active()?60u:120u)))){drawAt=now;paint(now);dirty=false;}
  if(uint32_t(now-reportAt)>=5000){reportAt=now;char report[160];int n=snprintf(report,sizeof(report),"TOQUE erros=%lu aceitos=%lu gap=%lu frame=%lu loop=%lu drop=%lu\n",(unsigned long)(busErrors+shortReads+countErrors+rangeErrors),(unsigned long)acceptedTouches,(unsigned long)maxPollGap,(unsigned long)frameMax,(unsigned long)loopGap,(unsigned long)serialDropped);if(Serial&&Serial.availableForWrite()>=n)Serial.write(reinterpret_cast<const uint8_t*>(report),n);else ++serialDropped;maxPollGap=frameMax=loopGap=0;}

  delay(1);
}

