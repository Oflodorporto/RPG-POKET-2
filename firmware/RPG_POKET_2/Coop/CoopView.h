#pragma once
// Read-only renderer. No clock, input dispatch, transport, RNG, campaign writes,
// allocations, rotation changes, decoded framebuffer, or owned art workspace.
#include "CoopViewModel.h"
#include "CoopPortraitBridge.h"
#include "../PersonalArt.h"
#include "../EnemyScale.h"
#include "../Ruins7Art.h"
#include <cstdio>
#include <cstring>
namespace coopview {
constexpr uint16_t rgb(unsigned r,unsigned g,unsigned b){return uint16_t((r>>3)<<11|(g>>2)<<5|(b>>3));}
constexpr uint16_t Gold=rgb(232,193,119),White=rgb(246,237,210),Blue=rgb(125,197,238),Green=rgb(122,213,126),Red=rgb(228,107,91),Muted=rgb(178,174,151),Ink=rgb(10,23,26),Disabled=rgb(118,132,137),Target=rgb(123,248,197);
inline const char* className(uint8_t cls){static const char* names[]={"MAGO","CAVALEIRO","GUERREIRO","BARBARO"};return names[cls<4?cls:0];}
inline const char* actorName(const coop::Actor& a){return a.kind==coop::Kind::Player?(a.owner?"J2":"J1"):a.species==0?"GOB":a.species==1?"LOB":a.species==2?"OSS":a.species==3?"GUARDIAO":a.species==4?"JAV":"INI";}
template<class C> void text(C& c,int x,int y,const char* s,uint16_t color=White){c.setTextSize(1);c.setTextColor(color);c.setCursor(x,y);c.print(s);}
template<class C> void center(C& c,const Rect& r,int y,const char* s,uint16_t color=White){text(c,r.x+(r.w-int(std::strlen(s))*6)/2,y,s,color);}
inline uint16_t dim(uint16_t color){return uint16_t((color&0xf7de)>>1);}
template<class C> void panel(C& c,const Rect& r,bool active=false,bool disabled=false){
 // Nine-slice sampling of the existing authored ruins frame, no new bitmap.
 constexpr int edge=5;const int sw=ruins7Art::frameW,sh=ruins7Art::frameH;
 for(int y=0;y<r.h;++y)for(int x=0;x<r.w;++x){
  int sx=x<edge?x:x>=r.w-edge?sw-r.w+x:edge+(x-edge)*(sw-2*edge)/std::max(1,int(r.w)-2*edge);
  int sy=y<edge?y:y>=r.h-edge?sh-r.h+y:edge+(y-edge)*(sh-2*edge)/std::max(1,int(r.h)-2*edge);
  uint8_t index=ruins7Art::framePixels[sy*sw+sx];
  if(index!=255){auto color=ruins7Art::palette[index];c.drawPixel(r.x+x,r.y+y,disabled?dim(color):color);}
 }
 if(active)c.drawRect(r.x+2,r.y+2,r.w-4,r.h-4,Target);
}
template<class C> void button(C& c,const Rect& r,const char* title,const char* hint,bool enabled,bool active=false){
 panel(c,r,active,!enabled&&!active);center(c,r,r.y+7,title,enabled||active?Gold:Disabled);
 if(hint&&*hint)center(c,r,r.y+r.h-13,hint,enabled||active?Muted:Disabled);
}
template<class C> void backdrop(C& c,const Rect& r,bool ready){
 c.fillRect(r.x,r.y,r.w,r.h,Ink);if(!ready||!artMemory)return;
 // Existing bg_battle indexed source: same center crop as approved preview.
 const auto& m=assetMeta[59];const uint8_t* data=assetBytes(59),*palette=assetBytes(58);
 const int cropTop=55,cropHeight=95;
 const int scaledHeight=cropHeight*r.w/int(m.w),cropY=(scaledHeight-r.h)/2;
 for(int y=0;y<r.h;++y)for(int x=0;x<r.w;++x){
  unsigned sx=unsigned(x)*m.w/r.w,sy=cropTop+unsigned(y+cropY)*m.w/r.w;
  if(sy>=m.h)sy=m.h-1;
  const uint8_t i=data[sy*m.w+sx];
  c.drawPixel(r.x+x,r.y+y,indexed4::little16(palette+2*i));
 }
}
struct Crop {int16_t x,y,w,h;};
// Union of the six unchanged authored hero poses; generated from validated pack.
constexpr Crop kHeroCrop[4][4]={
 {{4,40,104,77},{3,56,105,60},{4,46,104,70},{6,32,100,84}},
 {{5,16,106,99},{3,29,108,86},{3,5,108,110},{3,17,105,98}},
 {{11,32,99,83},{7,39,104,74},{3,17,108,95},{3,34,104,78}},
 {{5,11,106,105},{3,24,108,92},{3,19,108,97},{3,22,106,94}}
};
inline Rect fit(const Crop& source,Rect box){
 int w=box.w,h=int(box.w)*source.h/source.w;if(h>box.h){h=box.h;w=int(box.h)*source.w/source.h;}
 return {int16_t(box.x+(box.w-w)/2),int16_t(box.y+box.h-h),int16_t(w),int16_t(h)};
}
template<class C> void hero(C& c,const coop::Actor& a,Rect box,bool ready){
 if(!ready||!artMemory||a.cls>3||a.race>3)return;
 const Crop crop=kHeroCrop[a.race][a.cls];const Rect d=fit(crop,box);
 const auto* data=assetBytes(raceAsset[a.race][a.cls][0]);const auto* mask=assetBytes(raceMask[a.race][a.cls][0]);
 for(int y=0;y<d.h;++y)for(int x=0;x<d.w;++x){
  const unsigned index=unsigned(crop.y+y*crop.h/d.h)*112+crop.x+x*crop.w/d.w;
  const auto color=outfitPixel(indexed4::little16(data+2*index),mask[index],a.shirt,a.trousers);
  if(color!=indexed4::transparent)c.drawPixel(d.x+x,d.y+y,color);
 }
}
template<class C> void enemy(C& c,const coop::Actor& a,Rect box,bool ready){
 if(!ready||!artMemory||a.species>=enemyArt::ENEMY_COUNT)return;
 const auto& p=enemyScale::profile(a.species);const Crop crop{p.left,p.top,int16_t(p.right-p.left),int16_t(p.bottom-p.top)};
 const Rect d=fit(crop,box);const auto f=enemyArt::body(a.species,enemyArt::Pose::Idle);
 if(!enemyArt::valid(f))return;
 const uint8_t* data=assetBytes(f.asset),*pal=assetBytes(f.palette);
 for(int y=0;y<d.h;++y)for(int x=0;x<d.w;++x){
  unsigned sx=crop.x+x*crop.w/d.w,sy=crop.y+y*crop.h/d.h;uint8_t packed=data[sy*f.stride+sx/2];unsigned index=(sx&1)?packed&15:packed>>4;
  if(index)c.drawPixel(d.x+x,d.y+y,indexed4::little16(pal+2*index));
 }
}
template<class C> void bar(C& c,int x,int y,int w,uint16_t value,uint16_t total,uint16_t color){c.fillRect(x,y,w,2,Ink);if(value&&total)c.fillRect(x,y,std::max(1,int(uint32_t(w)*std::min(value,total)/total)),2,color);}
template<class C> void stats(C& c,const coop::Actor& a,uint8_t slot,uint8_t local,bool active){
 const Rect r=kStats[slot];panel(c,r,active);c.fillRect(r.x+3,r.y+3,r.w-6,r.h-6,rgb(5,29,59));char line[40];
 std::snprintf(line,sizeof(line),"J%u %s N%u%s",slot+1,className(a.cls),a.level,slot==local?" *":"");text(c,r.x+5,r.y+3,line,slot==local?Blue:Gold);
 std::snprintf(line,sizeof(line),"HP %u/%u",a.hp,a.maxhp);text(c,r.x+5,r.y+12,line,White);
 std::snprintf(line,sizeof(line),"MP %u/%u",a.mp,a.maxmp);text(c,r.x+5,r.y+21,line,Blue);
 if(a.guard)text(c,r.x+r.w-12,r.y+21,"D",Gold);
}
inline const char* eventLabel(const coopplatform::Training& t){
 const auto& e=t.endpoint;const auto& u=t.ui;
 if(!e.hasState||!coop::validState(e.state))return "AGUARDANDO ESTADO / SEM ENTRADA";
 if(e.paused)return "RECONECTANDO / SEM ENTRADA";
 if(u.mode==coopui::Mode::Pending||e.pending||e.awaitingAck)return "AGUARDANDO CONFIRMACAO / SEM ENTRADA";
 if(e.state.phase!=coop::Phase::Playing)return "TREINO CONCLUIDO";
 if(!u.playable(e))return "AGUARDE A SUA VEZ / SEM ENTRADA";
 if(u.mode==coopui::Mode::Target)return u.selected==coop::Action::Focus?"FOCO TESTE: TOQUE NO INIMIGO":"ATAQUE: TOQUE NO INIMIGO";
 if(u.mode==coopui::Mode::Ability)return "FOCO TESTE / HABILIDADE EXPERIMENTAL";
 if(u.mode==coopui::Mode::Items)return "POCOES COPIADAS SO PARA O TREINO";
 if(u.notice==coopui::Notice::Unavailable||u.notice==coopui::Notice::Rejected)return "ACAO INDISPONIVEL / ESCOLHA OUTRA";
 if(u.notice==coopui::Notice::Confirmed)return "ACAO CONFIRMADA";
 if(u.notice==coopui::Notice::Cancelled)return "ALVO CANCELADO / ESCOLHA UMA ACAO";
 return "SUA VEZ: ESCOLHA UMA ACAO";
}
template<class C> void controls(C& c,const coopplatform::Training& t){
 const auto& e=t.endpoint;const auto& u=t.ui;const bool play=u.playable(e);
 const char* labels[]={"ATAQUE","HABILIDADE","DEFENDER","ITENS"};const char* hints[]={"alvo","Foco teste","guarda","HP / MP"};
 bool enabled[4]={play,play,play,play},active[4]={};
 if(u.mode==coopui::Mode::Target){labels[0]=u.selected==coop::Action::Focus?"FOCO TESTE":"ATAQUE";hints[0]="toque alvo";labels[1]=labels[2]=hints[1]=hints[2]="";labels[3]="CANCELAR";hints[3]="voltar";enabled[0]=enabled[1]=enabled[2]=false;active[0]=play;}
 else if(u.mode==coopui::Mode::Ability){labels[0]="FOCO TESTE";hints[0]="2 MP";labels[1]=labels[2]=hints[1]=hints[2]="";labels[3]="CANCELAR";hints[3]="voltar";enabled[1]=enabled[2]=false;}
 else if(u.mode==coopui::Mode::Items){labels[0]="VIDA";labels[1]="MANA";labels[2]="";labels[3]="CANCELAR";hints[0]="HP";hints[1]="MP";hints[2]="";hints[3]="voltar";enabled[0]=u.actionError(e,coop::Action::Life,e.local)==coop::Error::None;enabled[1]=u.actionError(e,coop::Action::Mana,e.local)==coop::Error::None;enabled[2]=false;}
 if(!play)for(bool& b:enabled)b=false;
 for(unsigned i=0;i<4;++i)button(c,coopui::kButtons[i],labels[i],hints[i],enabled[i],active[i]);
}
template<class C> void playing(C& c,const ViewModel& v,const coopplatform::Training& t){
 const auto& e=t.endpoint;const auto& s=e.state;char line[64];const bool valid=e.hasState&&coop::validState(s);
 if(valid){
  std::snprintf(line,sizeof(line),"TREINO 2P  %s  J%u",s.wave==2?"CHEFE":s.wave?"ONDA 2/2":"ONDA 1/2",e.local+1);text(c,8,8,line,Gold);
  std::snprintf(line,sizeof(line),"VEZ: %s  /  RODADA %u",actorName(s.actors[s.turn]),s.round);text(c,4,25,line,Blue);
  text(c,4,34,"TURNOS INDIVIDUAIS  |  * = LOCAL",Muted);
 }else{text(c,8,8,"TREINO 2P",Gold);text(c,4,25,"SINCRONIZANDO AS DUAS PLACAS",Blue);}
 backdrop(c,kArena,v.artReady);
 if(valid){
  constexpr Rect heroes[2]={{18,54,58,64},{78,76,58,64}};
  for(uint8_t i=0;i<2;++i){const auto& a=s.actors[i];if(a.hp)hero(c,a,heroes[i],v.artReady);text(c,heroes[i].x-12,heroes[i].y+heroes[i].h-8,i?"J2":"J1",Blue);if(i==s.turn&&s.phase==coop::Phase::Playing)c.fillRect(heroes[i].x,heroes[i].y+heroes[i].h,32,2,Green);}
  for(uint8_t slot=3;slot<coop::kActors;++slot){const auto& a=s.actors[slot];if(a.kind==coop::Kind::Empty||!a.hp)continue;
   Rect hit=t.ui.targetRect(e,slot);Rect body{int16_t(hit.x+6),int16_t(hit.y+3),int16_t(hit.w-12),int16_t(hit.h-16)};enemy(c,a,body,v.artReady);
   std::snprintf(line,sizeof(line),"%s %u",actorName(a),a.hp);const int w=int(std::strlen(line))*6;
   const int x=hit.x+(hit.w-w)/2,y=hit.y+hit.h-12;c.fillRect(x-1,y,w+2,10,Ink);text(c,x,y+1,line,a.kind==coop::Kind::Boss?Gold:White);bar(c,x,y+9,w,a.hp,a.maxhp,Red);
   if(t.ui.eligible(e,slot)){c.drawRect(hit.x,hit.y,hit.w,hit.h,Target);c.drawRect(hit.x+1,hit.y+1,hit.w-2,hit.h-2,Target);}
  }
  for(uint8_t i=0;i<2;++i)stats(c,s.actors[i],i,e.local,s.phase==coop::Phase::Playing&&s.turn==i);
 }
 panel(c,kEvent);center(c,kEvent,153,v.artReady&&artMemory?eventLabel(t):"ARTES INDISPONIVEIS / USE SAIR",Gold);controls(c,t);
}
inline const char* endLabel(EndReason e){switch(e){case EndReason::Won:return "TREINO VENCIDO";case EndReason::Lost:return "FIM DO TREINO";case EndReason::PeerLeft:return "A OUTRA PLACA SAIU";case EndReason::LinkLost:return "CONEXAO ENCERRADA";case EndReason::JoinTimeout:return "TEMPO DE CONEXAO ESGOTADO";case EndReason::Protocol:return "SESSAO INCOMPATIVEL";case EndReason::Ineligible:return "VOLTE AO ABRIGO PARA TREINAR";default:return "TREINO ENCERRADO";}}
template<class C> void profileCard(C& c,const coop::Actor& a,Rect r,const char* title,bool ready){
 panel(c,r);center(c,r,r.y+6,title,Blue);hero(c,a,{int16_t(r.x+8),int16_t(r.y+18),int16_t(r.w-16),58},ready);char line[40];
 std::snprintf(line,sizeof(line),"%s N%u",className(a.cls),a.level);center(c,r,r.y+80,line,Gold);
 std::snprintf(line,sizeof(line),"HP %u/%u",a.hp,a.maxhp);center(c,r,r.y+91,line,White);
 std::snprintf(line,sizeof(line),"MP %u/%u",a.mp,a.maxmp);center(c,r,r.y+102,line,Blue);
}
template<class C> void lobby(C& c,const ViewModel& v){
 text(c,8,8,"COOPERATIVO / TREINO",Gold);
 const char* notice=v.notice==LobbyNotice::Declined?"PEDIDO RECUSADO / ESCOLHA UMA SALA":v.notice==LobbyNotice::Busy?"SALA OCUPADA / ESCOLHA OUTRA":v.notice==LobbyNotice::PeerExpired?"SALA EXPIROU / BUSCANDO NOVAMENTE":v.notice==LobbyNotice::SendFailed?"FALHA DE ENVIO / AGUARDE OU SAIA":"DUAS PLACAS / 2 ONDAS + CHEFE";
 text(c,16,27,v.artReady&&artMemory?notice:"ARTES INDISPONIVEIS / USE SAIR",v.notice==LobbyNotice::None?White:Gold);
 if(v.screen==Screen::Ended||v.screen==Screen::Unavailable){
  backdrop(c,{3,42,314,128},v.artReady);panel(c,{14,69,292,93});center(c,{14,69,292,93},81,endLabel(v.end),Gold);
  center(c,{14,69,292,93},103,"PERSONAGENS INALTERADOS",Green);center(c,{14,69,292,93},120,"HP, MP, POCOES E CAMPANHA PRESERVADOS",White);center(c,{14,69,292,93},140,"SEM XP / OURO / ITENS RECEBIDOS",Muted);
  text(c,31,190,"TOQUE SAIR PARA VOLTAR AO RETRATO",Blue);return;
 }
 if(v.hasLocal)profileCard(c,v.localProfile,{16,48,136,121},"SEU HEROI",v.artReady);
 if(v.screen==Screen::Lobby||v.screen==Screen::Searching||v.screen==Screen::JoinReady){
  bool any=false;for(uint8_t i=0;i<4;++i){const auto& p=v.peers[i];if(!p.present)continue;any=true;panel(c,kJoin[i],false,!v.allowJoin||!p.available);char line[40];
   std::snprintf(line,sizeof(line),"%s N%u",className(p.profile.cls),p.profile.level);center(c,kJoin[i],kJoin[i].y+4,line,Gold);
   std::snprintf(line,sizeof(line),"%s #%04X",p.available?"ENTRAR ":"OCUPADA",p.tag);center(c,kJoin[i],kJoin[i].y+15,line,v.allowJoin&&p.available?Blue:Disabled);
  }
  if(!any){text(c,177,88,"BUSCANDO SALAS",Gold);text(c,177,102,"CRIE NA OUTRA",Muted);text(c,177,112,"PLACA OU AQUI",Muted);}
  button(c,kHost,"CRIAR SALA","ser anfitriao",v.allowHost);text(c,175,187,"MESMO CANAL",Muted);text(c,175,199,"ESP-NOW",Blue);
 }else if(v.screen==Screen::ConfirmPeer){
  if(v.hasPeer)profileCard(c,v.peerProfile,{168,48,136,121},"QUER ENTRAR",v.artReady);
  button(c,kAccept,"ACEITAR","iniciar treino",v.allowAccept&&v.hasPeer);button(c,kDecline,"RECUSAR","voltar a sala",true);
 }else{
  panel(c,{168,48,136,121});const bool hosting=v.screen==Screen::Hosting;
  center(c,{168,48,136,121},72,hosting?"SALA CRIADA":"CONECTANDO",Gold);char line[24];std::snprintf(line,sizeof(line),"SALA #%04X",unsigned(v.roomCode&0xffff));center(c,{168,48,136,121},90,line,Blue);
  center(c,{168,48,136,121},111,hosting?"AGUARDE PEDIDO":"AGUARDE ACEITE",White);center(c,{168,48,136,121},132,"SEM ENTRADA",Muted);
  text(c,34,188,hosting?"ACEITE APENAS A PLACA COMBINADA":"AGUARDANDO CONFIRMACAO DAS DUAS PLACAS",Muted);
 }
}
template<class C> void draw(C& c,const ViewModel& v,const coopplatform::Training& t){
 c.setTextWrap(false);c.fillScreen(Ink);panel(c,{3,2,314,20});
 if(v.screen==Screen::Playing)playing(c,v,t);else lobby(c,v);
 panel(c,kExit);center(c,kExit,7,"SAIR",Gold);
 text(c,10,232,"SEM XP/OURO | SEM GASTOS NA CAMPANHA",Muted);
}
}
