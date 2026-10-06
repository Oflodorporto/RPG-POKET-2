#pragma once
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "ClubUi.h"
// Radio callbacks only enqueue; all admission, saving and commands run in loop().
struct ClubInbox {uint8_t mac[6];club::Packet packet;};
inline StaticQueue_t clubQueueState;inline uint8_t clubQueueBytes[8*sizeof(ClubInbox)];inline QueueHandle_t clubQueue=nullptr;
inline void clubReceive(const esp_now_recv_info_t* info,const uint8_t* data,int len){if(!clubQueue||len!=sizeof(club::Packet))return;ClubInbox entry;memcpy(entry.mac,info->src_addr,6);memcpy(&entry.packet,data,len);xQueueSend(clubQueue,&entry,0);}
inline void clubSend(void*,const uint8_t* mac,const club::Packet& p){if(!arena.opened)return;if(!esp_now_is_peer_exist(mac)){esp_now_peer_info_t peer{};memcpy(peer.peer_addr,mac,6);peer.channel=6;peer.ifidx=WIFI_IF_STA;esp_now_add_peer(&peer);}esp_now_send(mac,reinterpret_cast<const uint8_t*>(&p),sizeof(p));}
inline bool clubPersist(){if(journal.save(game))return true;afterSave=view.page;view.page=Page::SaveError;arena.notice="Falha ao salvar; duelo pausado";dirty=true;return false;}
inline bool clubAdmission(void*,uint32_t id,const club::Peer&){if(arena.begun)return false;auto before=game;const char* err=rpg::reserveFight(game,id);if(err){arena.notice=err;return false;}if(!clubPersist()){game=before;return false;}return true;}
inline void clubMoveSend(void*,uint16_t seq,uint8_t action,bool ack){arena.session.sendMove(seq,action,ack);}
inline bool clubBeginMoney(){if(game.clubStage==2)return true;if(!rpg::startFight(game,arena.session.sessionId()))return false;return clubPersist();}
inline void clubMoveReceive(void*,uint16_t seq,uint8_t action,bool ack,uint32_t now){if(!arena.begun||view.page==Page::SaveError)return;if(!ack&&seq>0&&seq==arena.duel.revision+1&&arena.duel.ready&&!arena.duel.finished&&arena.duel.turn!=arena.duel.local&&action<=3&&arena.duel.fighters[arena.duel.turn].mp>= (action==0||action==3?0:action==2?(arena.duel.fighters[arena.duel.turn].classId==2?3:4):(arena.duel.fighters[arena.duel.turn].classId==1||arena.duel.fighters[arena.duel.turn].classId==3?4:3))&&!clubBeginMoney())return;arena.duel.receive(seq,action,ack,now);dirty=true;}
inline bool openClub(){
 if(game.phase!=rpg::Phase::Home||!game.guildMember||game.p.level<5)return false;
 WiFi.disconnect(false,false);WiFi.scanDelete();menu.scanning=menu.connecting=menu.connected=false;WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(false);esp_wifi_set_channel(6,WIFI_SECOND_CHAN_NONE);
 arena=ClubUi{};clubQueue=xQueueCreateStatic(8,sizeof(ClubInbox),clubQueueBytes,&clubQueueState);if(esp_now_init()!=ESP_OK){arena.notice="Radio indisponivel";resumeConnection();return false;}
 arena.opened=true;esp_now_register_recv_cb(clubReceive);uint8_t mac[6];esp_wifi_get_mac(WIFI_IF_STA,mac);char name[12];const char* radioNames[]={"HUMANO","ELFO","ANAO","ORC"};snprintf(name,sizeof(name),"%s",radioNames[game.race%4]);arena.session.begin(mac,name,game.p.cls,game.p.level,clubSend,nullptr,millis());arena.session.setAdmission(clubAdmission,nullptr);arena.session.setMoveReceiver(clubMoveReceive,nullptr);return true;
}
inline void closeClub(){
 if(!arena.opened)return;arena.session.cancel(millis());arena.opened=false;esp_now_unregister_recv_cb();esp_now_deinit();clubQueue=nullptr;arena.begun=false;resumeConnection();
}
inline void recoverClubReservation(){if(game.clubStage==1){rpg::refundFight(game);savedTransition(currentPage());}else if(game.clubStage==2){rpg::settleFight(game,game.clubSession,-1);savedTransition(currentPage());}}
inline void finishClub(int result){
 arena.finishedAt=millis();arena.result=result;if(game.clubStage==1){rpg::refundFight(game);arena.result=2;}else if(game.clubStage==2)rpg::settleFight(game,game.clubSession,result);view.page=Page::ClubResult;clubPersist();dirty=true;
}
inline void tickClub(uint32_t now){
 if(!arena.opened||view.page==Page::SaveError)return;
 ClubInbox entry;for(unsigned i=0;i<8&&xQueueReceive(clubQueue,&entry,0)==pdTRUE;++i)arena.session.receive(entry.mac,entry.packet,now);
 arena.session.setAvailable(game.clubStage==1||game.clubStage==2||game.p.gold>=25);arena.session.tick(now);
 if(arena.session.state==club::State::Linked&&!arena.begun){arena.begun=true;auto& peer=arena.session.opponent;arena.duel.begin(arena.session.sessionId(),arena.session.isInitiator(),game.p.cls,game.p.level,peer.classId,peer.level,clubMoveSend,nullptr,now);view.page=Page::ClubBattle;dirty=true;}
 if(arena.begun&&view.page!=Page::ClubResult){if(arena.session.state!=club::State::Linked){finishClub(-1);return;}arena.duel.tick(now);if(arena.duel.failed)finishClub(-1);else if(arena.duel.finished&&!arena.duel.waiting)finishClub(arena.duel.winner<0?0:arena.duel.winner==arena.duel.local?1:-1);}
 if(arena.session.state==club::State::Notice&&!arena.begun){if(game.clubStage==1){rpg::refundFight(game);if(!clubPersist())return;}arena.notice="Convite encerrado; valor devolvido";}
 if(uint32_t(now-arena.redraw)>=200){arena.redraw=now;dirty=true;}
}
inline uint32_t clubSessionId(uint32_t value){return value?value:1;}
inline void clubTap(int x,int y){
 uint32_t now=millis();
 if(view.page==Page::ClubResult){if(hit(x,y,14,272,212)){if(uint32_t(now-arena.finishedAt)<11000){arena.notice="Confirmando resultado...";dirty=true;return;}closeClub();view.page=Page::Guild;say("");}return;}
 if(view.page==Page::ClubBattle){if(!arena.duel.ready||arena.duel.waiting||arena.duel.turn!=arena.duel.local)return;for(unsigned i=0;i<4;++i)if(hit(x,y,14+(i%2)*110,220+(i/2)*50,102)){auto a=club::Action(i);if(arena.duel.fighters[arena.duel.local].mp<arena.duel.cost(a)){arena.notice="Mana insuficiente";dirty=true;return;}if(!clubBeginMoney())return;arena.duel.play(a,now);dirty=true;return;}return;}
 if(hit(x,y,14,272,212)){if(game.clubStage==1){rpg::refundFight(game);if(!clubPersist())return;}closeClub();view.page=Page::Guild;say("");return;}
 auto state=arena.session.state;
 if(state==club::State::Incoming){if(hit(x,y,14,218,102))arena.session.decline(now);else if(hit(x,y,124,218,102))arena.session.accept(now);}
 else if(state==club::State::Listing||state==club::State::Notice){if(state==club::State::Notice)arena.session.dismiss();for(unsigned i=0;i<6;++i)if(hit(x,y,14,54+i*25,212,24))arena.choice=i;
 if(hit(x,y,14,218,212)){if(!arena.session.invite(arena.choice,clubSessionId(esp_random()),now))arena.notice="Escolha uma placa disponivel";}}
 dirty=true;
}
