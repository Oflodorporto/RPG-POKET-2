#pragma once
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "ClubUi.h"
#include "CoopConfig.h"
#if RPG_ENABLE_COOP_TRAINING
#include "Coop/RadioOwner.h"
#endif
// Radio callbacks only enqueue; all admission, saving and commands run in loop().
struct ClubInbox {uint8_t mac[6];club::Packet packet;};
inline StaticQueue_t clubQueueState;inline uint8_t clubQueueBytes[8*sizeof(ClubInbox)];inline QueueHandle_t clubQueue=nullptr;
struct ClubRadioState {bool lease=false,wifiTouched=false,radioReady=false,recvRegistered=false;wifi_mode_t oldMode=WIFI_MODE_NULL;wifi_second_chan_t oldSecondary=WIFI_SECOND_CHAN_NONE;uint8_t oldChannel=0;bool oldAuto=false,oldConnected=false;};
inline ClubRadioState clubRadioState;
inline void clubReceive(const esp_now_recv_info_t* info,const uint8_t* data,int len){if(!clubQueue||!info||!info->src_addr||!data||(len!=sizeof(club::Packet)&&len!=44))return;if((len==44&&data[8]!=4)||(len==sizeof(club::Packet)&&data[8]!=club::VERSION))return;ClubInbox entry{};memcpy(entry.mac,info->src_addr,6);memcpy(&entry.packet,data,len);xQueueSend(clubQueue,&entry,0);}
inline void clubSend(void*,const uint8_t* mac,const club::Packet& p){if(!arena.opened)return;if(!esp_now_is_peer_exist(mac)){esp_now_peer_info_t peer{};memcpy(peer.peer_addr,mac,6);peer.channel=6;peer.ifidx=WIFI_IF_STA;esp_now_add_peer(&peer);}esp_now_send(mac,reinterpret_cast<const uint8_t*>(&p),sizeof(p));}
inline bool clubPersist(){if(journal.save(game))return true;afterSave=view.page;view.page=Page::SaveError;arena.notice="Falha ao salvar; duelo pausado";dirty=true;return false;}
inline bool clubAdmission(void*,uint32_t id,const club::Peer&){if(arena.begun||view.page==Page::SaveError)return false;auto before=game;const char* err=rpg::reserveFight(game,id);if(err){arena.notice=err;return false;}if(!clubPersist()){game=before;return false;}return true;}
inline void clubMoveSend(void*,uint16_t seq,uint8_t action,bool ack){arena.session.sendMove(seq,action,ack);}
inline void clubResultSend(void*,uint16_t seq,int8_t winner,uint32_t digest){arena.session.sendResult(seq,winner,digest);}
inline void clubResultReceive(void*,uint16_t seq,int8_t winner,uint32_t digest,uint32_t now){if(!arena.begun||view.page==Page::SaveError||view.page==Page::ClubResult)return;arena.duel.receiveResult(seq,winner,digest,now);arena.visual.capture(arena.duel,now);dirty=true;}
inline bool clubBeginMoney(){if(game.clubStage==2)return true;if(!rpg::startFight(game,arena.session.sessionId()))return false;return clubPersist();}
inline void clubMoveReceive(void*,uint16_t seq,uint8_t action,bool ack,uint32_t now){
 if(!arena.begun||view.page==Page::SaveError||(ack&&view.page==Page::ClubResult))return;
 if(!ack&&arena.duel.acceptsMove(seq,action)&&!clubBeginMoney())return;
 arena.duel.receive(seq,action,ack,now);arena.visual.capture(arena.duel,now);dirty=true;
}
inline bool restoreClubWifi(){
 auto& radio=clubRadioState;if(!radio.wifiTouched)return true;
 if(radio.oldMode==WIFI_MODE_NULL){if(!WiFi.mode(WIFI_MODE_NULL))return false;}
 else {
  if(radio.oldChannel&&esp_wifi_set_channel(radio.oldChannel,radio.oldSecondary)!=ESP_OK)return false;
  if(esp_wifi_set_storage(WIFI_STORAGE_FLASH)!=ESP_OK)return false;
  if(radio.oldConnected&&WiFi.status()!=WL_CONNECTED&&!WiFi.reconnect())return false;
 }
 WiFi.persistent(true);if(!WiFi.setAutoReconnect(radio.oldAuto))return false;radio.wifiTouched=false;return true;
}
inline bool closeClub(){
 auto& radio=clubRadioState;if(!radio.lease)return true;
#if RPG_ENABLE_COOP_TRAINING
 if(coopradio::owner!=coopradio::Owner::Pvp)return false;
#endif
 if(arena.opened){arena.session.cancel(millis());arena.opened=false;}
 // Keep the queue reachable until the callback has been unregistered.
 if(radio.recvRegistered){if(esp_now_unregister_recv_cb()!=ESP_OK)goto pending;radio.recvRegistered=false;}
 clubQueue=nullptr;
 if(radio.radioReady){if(esp_now_deinit()!=ESP_OK)goto pending;radio.radioReady=false;}
 if(!restoreClubWifi())goto pending;
 arena.begun=false;arena.cleanupPending=false;radio.lease=false;
#if RPG_ENABLE_COOP_TRAINING
 coopradio::release(coopradio::Owner::Pvp);
#endif
 menu.connected=WiFi.status()==WL_CONNECTED;menu.connecting=menu.scanning=false;return true;
pending:
 arena.cleanupPending=true;arena.notice="Radio ainda em limpeza";view.page=Page::Club;
 dirty=true;return false;
}
inline bool failClubOpen(const char* notice){arena.notice=notice;closeClub();return false;}
inline bool openClub(){
 if(clubRadioState.lease||game.phase!=rpg::Phase::Home||!game.guildMember||game.p.level<5)return false;
#if RPG_ENABLE_COOP_TRAINING
 if(!coopradio::acquire(coopradio::Owner::Pvp))return false;
#endif
 auto& radio=clubRadioState;radio=ClubRadioState{};radio.lease=true;arena=ClubUi{};
 clubQueue=xQueueCreateStatic(8,sizeof(ClubInbox),clubQueueBytes,&clubQueueState);if(!clubQueue)return failClubOpen("Fila do radio indisponivel");
 radio.oldMode=WiFi.getMode();if(radio.oldMode!=WIFI_MODE_NULL&&radio.oldMode!=WIFI_MODE_STA)return failClubOpen("Modo Wi-Fi ocupado");
 radio.oldAuto=WiFi.getAutoReconnect();radio.oldConnected=WiFi.status()==WL_CONNECTED;
 if(radio.oldMode!=WIFI_MODE_NULL){
  if(esp_wifi_get_channel(&radio.oldChannel,&radio.oldSecondary)!=ESP_OK)return failClubOpen("Canal Wi-Fi indisponivel");
  if(esp_wifi_set_storage(WIFI_STORAGE_RAM)!=ESP_OK)return failClubOpen("Wi-Fi indisponivel");
 }
 radio.wifiTouched=true;if(!WiFi.setAutoReconnect(false))return failClubOpen("Auto Wi-Fi indisponivel");WiFi.persistent(false);
 const bool started=WiFi.mode(WIFI_STA);WiFi.persistent(true);if(!started)return failClubOpen("Modo Wi-Fi indisponivel");
 if(esp_wifi_set_storage(WIFI_STORAGE_RAM)!=ESP_OK)return failClubOpen("Wi-Fi indisponivel");
 if(!WiFi.disconnect(false,false))return failClubOpen("Desconexao Wi-Fi falhou");
 WiFi.scanDelete();menu.scanning=menu.connecting=menu.connected=false;
 if(esp_wifi_set_channel(6,WIFI_SECOND_CHAN_NONE)!=ESP_OK)return failClubOpen("Canal do radio indisponivel");
 uint8_t mac[6]{};if(esp_wifi_get_mac(WIFI_IF_STA,mac)!=ESP_OK)return failClubOpen("MAC do radio indisponivel");
 bool nonzero=false;for(auto octet:mac)nonzero|=octet!=0;if(!nonzero||(mac[0]&1))return failClubOpen("MAC do radio indisponivel");
 if(esp_now_init()!=ESP_OK)return failClubOpen("Radio indisponivel");radio.radioReady=true;
 if(esp_now_register_recv_cb(clubReceive)!=ESP_OK)return failClubOpen("Callback do radio indisponivel");radio.recvRegistered=true;
 char name[12];const char* radioNames[]={"HUMANO","ELFO","ANAO","ORC"};snprintf(name,sizeof(name),"%s",radioNames[game.race%4]);arena.session.begin(mac,name,game.p.cls,game.p.level,clubSend,nullptr,millis());arena.session.setAppearance(game.race,game.shirt,game.trousers);arena.session.setAdmission(clubAdmission,nullptr);arena.session.setMoveReceiver(clubMoveReceive,nullptr);arena.session.setResultReceiver(clubResultReceive,nullptr);arena.opened=true;return true;
}
inline void recoverClubReservation(){if(game.clubStage==1){rpg::refundFight(game);savedTransition(currentPage());}else if(game.clubStage==2){rpg::settleFight(game,game.clubSession,-1);savedTransition(currentPage());}}
inline void finishClub(int result,bool unconfirmed=false){
 arena.finishedAt=millis();arena.result=result;arena.resultUnconfirmed=unconfirmed&&game.clubStage==2;if(game.clubStage==1){rpg::refundFight(game);arena.result=2;}else if(game.clubStage==2)rpg::settleFight(game,game.clubSession,result);view.page=Page::ClubResult;clubPersist();dirty=true;
}
inline void tickClub(uint32_t now){
 if(!arena.opened||view.page==Page::SaveError)return;
 ClubInbox entry;for(unsigned i=0;i<8&&xQueueReceive(clubQueue,&entry,0)==pdTRUE;++i){
  arena.session.receive(entry.mac,entry.packet,now);
  if(view.page==Page::SaveError)return;
 }
 arena.session.setAvailable(game.clubStage==1||game.clubStage==2||game.p.gold>=25);arena.session.tick(now);
 if(view.page==Page::SaveError)return;
 if(arena.session.state==club::State::Linked&&!arena.begun){arena.begun=true;auto& peer=arena.session.opponent;arena.duel.begin(arena.session.sessionId(),arena.session.isInitiator(),game.p.cls,game.p.level,peer.classId,peer.level,clubMoveSend,nullptr,now);arena.duel.setResultSender(clubResultSend);view.page=Page::ClubBattle;dirty=true;}
 if(arena.begun){
  // A proved terminal outcome is immutable even if Cancel/Lost follows its ACK
  // in the same queue drain. A merely predicted lethal hit is never rewarded.
  if(view.page!=Page::ClubResult){
   if(arena.duel.terminalConfirmed)finishClub(arena.duel.winner<0?0:arena.duel.winner==arena.duel.local?1:-1);
   else if(arena.session.state!=club::State::Linked){
    if((arena.duel.finished&&arena.duel.waiting)||arena.duel.surrenderRequested){
     // Honor the original pending-command deadline. Closing cannot erase a
     // proof already queued or briefly delayed, and it cannot extend the wait.
     arena.notice="Confirmando resultado...";arena.duel.tick(now);
     if(arena.duel.failed)finishClub(-1,true);
    }else {finishClub(-1,true);return;}
   }
   else {arena.duel.tick(now);if(arena.duel.failed)finishClub(-1,true);}
  }else arena.duel.tick(now);
 }
 if(arena.session.state==club::State::Notice&&!arena.begun){if(game.clubStage==1){rpg::refundFight(game);if(!clubPersist())return;}arena.notice="Convite encerrado; valor devolvido";}
 const bool wasAnimating=arena.visual.active();arena.visual.tick(now);if(wasAnimating&&!arena.visual.active())dirty=true;
 if(uint32_t(now-arena.redraw)>=(arena.visual.active()?60u:200u)){arena.redraw=now;dirty=true;}
}
inline uint32_t clubSessionId(uint32_t value){return value?value:1;}
inline void clubTap(int x,int y){
 uint32_t now=millis();
 if(arena.cleanupPending){if(hit(x,y,14,272,212)&&closeClub()){view.page=Page::Guild;say("");}return;}
 if(view.page==Page::ClubResult){if(hit(x,y,14,272,212)){if(uint32_t(now-arena.finishedAt)<11000){arena.notice="Confirmando resultado...";dirty=true;return;}if(closeClub()){view.page=Page::Guild;say("");}}return;}
 if(view.page==Page::ClubBattle){
  for(unsigned i=0;i<4;++i)if(hit(x,y,14+(i%2)*110,220+(i/2)*50,102)){
   auto a=club::Action(i);
   if(!arena.duel.canPlay(a)){
    if(arena.duel.ready&&!arena.duel.finished&&!arena.duel.surrenderRequested&&!arena.duel.waiting&&arena.duel.turn==arena.duel.local&&arena.duel.fighters[arena.duel.local].mp<arena.duel.cost(a))arena.notice="Mana insuficiente";
    dirty=true;return;
   }
   if(!clubBeginMoney())return;
   arena.notice="";arena.duel.play(a,now);dirty=true;return;
  }
  return;
 }
 if(hit(x,y,14,272,212)){if(game.clubStage==1){rpg::refundFight(game);if(!clubPersist())return;}if(closeClub()){view.page=Page::Guild;say("");}return;}
 auto state=arena.session.state;
 if(state==club::State::Incoming){if(hit(x,y,14,218,102))arena.session.decline(now);else if(hit(x,y,124,218,102))arena.session.accept(now);}
 else if(state==club::State::Listing||state==club::State::Notice){if(state==club::State::Notice)arena.session.dismiss();for(unsigned i=0;i<6;++i)if(hit(x,y,14,54+i*25,212,24))arena.choice=i;
 if(hit(x,y,14,218,212)){if(!arena.session.invite(arena.choice,clubSessionId(esp_random()),now))arena.notice=arena.choice>=0&&arena.choice<club::MAX_PEERS&&arena.session.peers[arena.choice].used&&arena.session.peers[arena.choice].version!=club::VERSION?"Atualize as duas placas":"Escolha uma placa disponivel";}}
 dirty=true;
}
