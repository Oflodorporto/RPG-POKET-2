#pragma once
#include "World.h"
#include "WorldClock.h"
#include "AssetStore.h"
#include "UpdateModel.h"
struct MenuState {
  bool hasContinue=false,sessionStarted=false,creationFromTitle=false,newGameSlots=false;int slotsReturn=27;
  uint32_t renderNow=0,letterStarted=0,eventCheckAt=0,eventDay=0;uint8_t eventHour=0;int eventReturn=27;
  uint8_t storyIndex=0,chapterIndex=0,personIndex=0,regionIndex=0;bool storyReplay=false;int storyReturn=27;
  uint8_t draftRace=0,draftShirt=0,draftPants=0;
  uint32_t frameMs=0,pollMs=0,loopMs=0;
  uint8_t activeSlot=0,slotChoice=0,brightness=80,keyboard=0,keyPage=0;
  rpg::Load slots[3]={rpg::Load::Empty,rpg::Load::Empty,rpg::Load::Empty};rpg::Game previews[3];
  bool connected=false,connecting=false,scanning=false,openNetwork=false;
  bool savedNetworks=false;uint8_t savedCount=0,savedIndex=0,signalBars=0;int signalDbm=-100,networkDbm=-100,networkChannel=0;
  int networkCount=0,networkIndex=0;char network[33]="",password[64]="",ip[20]="";
  int8_t utcOffset=-3;bool clockAutomatic=true,dayCycle=true;worldClock::Period worldPeriod=worldClock::Period::Day;
  uint8_t clockField=0;int clockDraft[5]={2026,1,1,12,0};
  bool clockIdle=false,clockValid=false;char clockTime[9]="--:--:--",clockDate[11]="--/--/----";
  uint32_t rollStarted=0,campStarted=0;bool rollReady=false,campRation=false,campKit=false,campShopReturn=false;
  ArtStatus art=ArtStatus::Fallback;Journey journey;uint8_t destination=0;
  uint32_t touchErrors=0,touches=0;uint8_t flashMiB=0,ramMiB=0;int memoryTest=-1;const char* notice="";
};
inline MenuState menu;
inline uint8_t wifiBars(int dbm){return dbm>=-55?4:dbm>=-67?3:dbm>=-75?2:1;}
inline const char* keyboardChars(uint8_t mode){static const char* keys[]={"abcdefghijklmnopqrstuvwxyz0123456789","ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789","!@#$%^&*()-_=+[]{};:,.?/\\|~`'\"<>    "};return keys[mode%3];}
template<class Canvas,class Text,class Center,class Box,class Button,class Portrait>
bool renderMenu(Canvas& c,const rpg::Game& g,int page,Text text,Center center,Box box,Button button,Portrait portrait,unsigned frame){
  // New pages are appended to Page after Card (26), kept independent of View.
  char b[64];
  if(page==27){center(12,"MENU",2,UI_GOLD);snprintf(b,sizeof(b),"Slot %u / %s",menu.activeSlot+1,rpg::className(g.p.cls));center(53,b);
    button(14,86,212,"Mapa de Aeldra");button(14,132,212,"Personagens");button(14,178,212,"Tela de titulo");button(14,224,102,"Diario");button(124,224,102,"Pessoas");button(14,272,102,"Cartas");button(124,272,102,"Voltar");return true;}
  if(page==28){center(12,menu.newGameSlots?"NOVO PERSONAGEM":"PERSONAGENS",2,UI_GOLD);for(unsigned i=0;i<3;++i){int y=55+i*61;box(14,y,212,56);snprintf(b,sizeof(b),"SLOT %u%s",i+1,i==menu.activeSlot?" / ATIVO":"");text(24,y+8,b,1,UI_GOLD);
      if(menu.slots[i]==rpg::Load::Empty)snprintf(b,sizeof(b),"Vazio / criar personagem");else if(menu.slots[i]==rpg::Load::Blocked)snprintf(b,sizeof(b),"Save protegido / ver opcoes");else snprintf(b,sizeof(b),"%s / Nivel %u",rpg::className(menu.previews[i].p.cls),menu.previews[i].p.level);text(24,y+29,b);}
    center(247,*menu.notice?menu.notice:menu.newGameSlots?"Libere um slot para criar":"",1,UI_RED);button(14,272,212,"Voltar");return true;}
  if(page==29||page==30){bool del=page==30;snprintf(b,sizeof(b),"SLOT %u",menu.slotChoice+1);center(12,b,2,UI_GOLD);
    if(menu.slots[menu.slotChoice]==rpg::Load::Ok||menu.slots[menu.slotChoice]==rpg::Load::Recovered){portrait(menu.previews[menu.slotChoice].p.cls,80,48,&menu.previews[menu.slotChoice]);snprintf(b,sizeof(b),"%s / Nivel %u",rpg::className(menu.previews[menu.slotChoice].p.cls),menu.previews[menu.slotChoice].p.level);center(131,b);}
    else center(101,menu.slots[menu.slotChoice]==rpg::Load::Empty?"Slot vazio":"Save protegido",2);
    if(del){center(176,"APAGAR ESTE PERSONAGEM?",1,UI_RED);center(202,"A exclusao e permanente.");center(224,"Os outros slots ficam preservados.");center(245,menu.notice,1,UI_RED);button(14,272,102,"Cancelar");button(124,272,102,"Apagar");}
    else{center(160,menu.notice,1,UI_RED);if(menu.slots[menu.slotChoice]!=rpg::Load::Blocked&&(!menu.newGameSlots||menu.slots[menu.slotChoice]==rpg::Load::Empty))button(14,178,212,menu.slots[menu.slotChoice]==rpg::Load::Empty?"Criar":"Entrar");if(menu.slots[menu.slotChoice]!=rpg::Load::Empty)button(14,224,212,"Excluir slot");button(14,272,212,"Voltar");}return true;}
  if(page==31){center(12,"CONFIGURACOES",2,UI_GOLD);snprintf(b,sizeof(b),"Brilho: %u%%",menu.brightness);center(52,b,2);button(14,78,102,"- Brilho");button(124,78,102,"+ Brilho");
    button(14,130,212,"Internet / Wi-Fi");button(14,176,102,"Cartao");button(124,176,102,"Testes");button(14,222,102,"Atualizar");button(124,222,102,"Horario");center(265,menu.notice,1,UI_RED);button(14,278,102,"Voltar");button(124,278,102,"Guia");return true;}
  if(page==71){center(12,"HORA E MUNDO",2,UI_GOLD);center(46,menu.clockValid?menu.clockDate:"Hora nao sincronizada");center(65,menu.clockTime,2);snprintf(b,sizeof(b),"Fuso UTC%+d",int(menu.utcOffset));center(94,b);button(14,113,102,"- Fuso");button(124,113,102,"+ Fuso");button(14,159,212,menu.clockAutomatic?"Hora: automatica":"Hora: manual");button(14,205,212,menu.dayCycle?"Dia/noite: SIM":"Dia/noite: NAO");center(251,menu.notice,1,UI_RED);button(14,272,102,"Voltar");button(124,272,102,"Acertar");return true;}
  if(page==72){static const char* fields[]={"ANO","MES","DIA","HORA","MINUTO"};center(12,"ACERTAR A HORA",2,UI_GOLD);center(49,"Selecione e ajuste cada campo");snprintf(b,sizeof(b),"%02d/%02d/%04d %02d:%02d",menu.clockDraft[2],menu.clockDraft[1],menu.clockDraft[0],menu.clockDraft[3],menu.clockDraft[4]);center(78,b);center(112,fields[menu.clockField],2,UI_GOLD);snprintf(b,sizeof(b),"%02d",menu.clockDraft[menu.clockField]);center(142,b,3);button(14,181,102,"- Valor");button(124,181,102,"+ Valor");button(14,227,102,"Anterior");button(124,227,102,"Proximo");button(14,272,102,"Cancelar");button(124,272,102,"Aplicar");return true;}
  if(page==45){center(26,"ATUALIZACAO",2,UI_GOLD);center(58,"Via GitHub / Wi-Fi");center(88,FW_VERSION,1,UI_MUTED);
    if(updateInfo.busy){center(124,updater::stage(updateInfo.state));snprintf(b,sizeof(b),"%u%%",updateInfo.progress);center(158,b,2,UI_GREEN);box(14,192,212,20);c.fillRect(17,195,206*std::min(100u,updateInfo.progress)/100,14,UI_GREEN);center(234,updateInfo.message);center(270,"Nao retire o cartao ou a energia.");}
    else{center(121,*updateInfo.version?updateInfo.version:menu.connected?"Conectado / pronto para verificar":"Conecte o Wi-Fi primeiro",1,UI_GOLD);center(153,updateInfo.message,1,updateInfo.state==updater::State::Error?UI_RED:UI_WHITE);
      if(updateInfo.state==updater::State::Available||updateInfo.canResume){center(189,updateInfo.artOnly?"Baixar e reparar as artes?":"Instalar a nova versao e artes?");button(14,272,102,"Cancelar");button(124,272,102,updateInfo.canResume?"Continuar":"Instalar");button(14,218,212,"Testar conexao");}
      else{button(14,173,212,"Testar conexao");button(14,218,212,"Verificar versao");button(14,272,212,"Voltar");}}
    return true;}
  if(page==52){center(12,"TESTE DE CONEXAO",2,UI_GOLD);center(48,"Sinal e servidor do GitHub");
    if(updateInfo.busy){center(111,"Testando HTTPS...");snprintf(b,sizeof(b),"Tentativa %u/3",updateInfo.netTrials);center(150,b);center(203,"Aguarde o resultado.");}
    else if(updateInfo.tested){snprintf(b,sizeof(b),"HTTPS: %u/%u respostas OK",updateInfo.netSuccess,updateInfo.netTrials);center(100,b,1,updateInfo.netSuccess==3?UI_GREEN:UI_RED);snprintf(b,sizeof(b),"Falhas: %u / Wi-Fi caiu: %u",updateInfo.netTrials-updateInfo.netSuccess,updateInfo.netDrop);center(132,b);snprintf(b,sizeof(b),"Sinal: %d a %d dBm",updateInfo.minDbm,updateInfo.maxDbm);center(164,b);snprintf(b,sizeof(b),"Resposta media: %lums",(unsigned long)updateInfo.avgMs);center(196,b);center(235,"Teste HTTPS; nao mede ping.");}
    else center(141,updateInfo.message,1,UI_RED);if(!updateInfo.busy){button(14,272,102,"Voltar");button(124,272,102,"Testar");}return true;}
  if(page==51){c.fillRect(0,0,240,320,0);if(menu.sessionStarted&&g.eventStage==1){unsigned color=frame%8<4?UI_GOLD:UI_WHITE;c.fillRect(102,48,36,34,0xbdd0);c.drawRect(100,46,40,38,color);for(unsigned i=0;i<3;++i)c.fillRect(109,57+i*7,22,2,0x4208);c.fillRect(116,88,8,8,color);}c.setTextColor(UI_WHITE);c.setTextSize(3);c.setCursor(75,112);char hm[6];snprintf(hm,sizeof(hm),"%.5s",menu.clockTime);c.print(hm);c.setTextSize(1);c.setCursor(114,148);c.print(menu.clockTime+6);c.setTextSize(2);c.setCursor(60,184);c.print(menu.clockDate);c.setTextSize(1);c.setCursor(90,226);c.print(menu.clockValid?worldClock::name(menu.worldPeriod):"Sem hora");c.setCursor(30,278);c.print(menu.sessionStarted&&g.eventStage==1?"Toque para ler a carta":menu.sessionStarted?"Toque para voltar ao jogo":"Toque para voltar ao titulo");if(!menu.clockValid){c.setCursor(45,249);c.print("Wi-Fi ou ajuste em Horario");}return true;}
  if(page==32){center(12,"INTERNET / WI-FI",2,UI_GOLD);center(43,menu.connecting?"Conectando...":menu.connected?"Wi-Fi conectado":"Wi-Fi desconectado",1,menu.connected?UI_GREEN:UI_WHITE);
    if(menu.connected){snprintf(b,sizeof(b),"Sinal %d dBm / %u barras",menu.signalDbm,menu.signalBars);center(64,b);}else center(64,"Redes de 2.4 GHz");
    button(14,80,102,"Salvas");button(124,80,102,menu.scanning?"Buscando":"Buscar");
    unsigned count=menu.savedNetworks?menu.savedCount:menu.networkCount;
    if(count){char shortName[25];snprintf(shortName,sizeof(shortName),"%.24s",menu.network);center(130,shortName);
      if(menu.savedNetworks)snprintf(b,sizeof(b),"Salva %u/%u",menu.savedIndex+1,menu.savedCount);else snprintf(b,sizeof(b),"%d/%d Ch%d %ddBm",menu.networkIndex+1,menu.networkCount,menu.networkChannel,menu.networkDbm);center(151,b);
      button(14,174,102,"Anterior");button(124,174,102,"Proxima");
    }else center(139,menu.scanning?"Buscando em todos os canais...":menu.savedNetworks?"Nenhuma rede salva":"Toque em Buscar");
    button(14,221,102,menu.savedNetworks?"Esquecer":"2.4 GHz");button(124,221,102,menu.connecting?"Aguarde":"Conectar");center(260,menu.notice,1,UI_RED);
    button(14,276,102,"Voltar");button(124,276,102,menu.connecting?"Cancelar":"Desligar");return true;}
  if(page==50){center(20,"ESQUECER REDE",2,UI_GOLD);char name[25];snprintf(name,sizeof(name),"%.24s",menu.network);center(105,name);center(158,"Remover a senha salva?");center(191,"Seus personagens ficam intactos.");center(237,menu.notice,1,UI_RED);button(14,272,102,"Cancelar");button(124,272,102,"Esquecer");return true;}
  if(page==33){char bar[31];unsigned n=strlen(menu.password);snprintf(bar,sizeof(bar),"%u: %.*s",n,22,menu.password+(n>22?n-22:0));box(4,2,232,30);text(10,12,*menu.notice?menu.notice:bar,1,*menu.notice?UI_RED:UI_WHITE);
    const char* chars=keyboardChars(menu.keyboard);for(unsigned i=0;i<12;++i){int x=4+(i%3)*78,y=38+(i/3)*45;box(x,y,76,43);char ch[2]={chars[menu.keyPage*12+i],0};text(x+31,y+12,ch,2);}
    button(4,222,76,menu.keyboard==0?"abc":menu.keyboard==1?"ABC":"#+@");button(82,222,76,"<- Pag");button(160,222,76,"Pag >");
    box(4,266,76,50);text(14,278,"Voltar");box(82,266,76,50);text(97,278,"Apagar");box(160,266,76,50);text(173,278,"Conectar");return true;}
  if(page==34){center(12,"TESTES DO APARELHO",2,UI_GOLD);snprintf(b,sizeof(b),"Flash: %u MiB / PSRAM: %u MiB",menu.flashMiB,menu.ramMiB);center(57,b);
    snprintf(b,sizeof(b),"Toques: %lu / erros: %lu",(unsigned long)menu.touches,(unsigned long)menu.touchErrors);center(83,b);center(112,"Um toque deve contar uma vez.");
    center(144,menu.art==ArtStatus::Ready?"Artes do cartao: OK":"Artes simplificadas: ativas",1,menu.art==ArtStatus::Ready?UI_GREEN:UI_GOLD);
    snprintf(b,sizeof(b),"Tela %lums / toque %lums",(unsigned long)menu.frameMs,(unsigned long)menu.pollMs);center(198,b);center(177,menu.memoryTest<0?"Teste PSRAM: ainda nao feito":menu.memoryTest?"Teste 64 KiB PSRAM: OK":"Teste PSRAM: falhou",1,menu.memoryTest==1?UI_GREEN:UI_WHITE);
    button(14,222,212,"Testar memoria");button(14,272,212,"Voltar");return true;}
  if(page==35||page==14||page==46){bool roll=page==46;bool travel=page==35;if(roll||travel){c.fillRect(0,272,240,48,UI_INK);c.fillRect(53,245,134,26,UI_INK);box(35,4,170,48);}center(21,roll?"TESTE DE VIAGEM":travel?"EM VIAGEM":"AELDRA",1,UI_GOLD);
    if(!roll&&!travel){box(150,30,80,24);text(158,38,"Continente");text(8,38,"Valdaria");}
    auto line=[&](Point p,Point q,uint16_t color){int dx=abs(q.x-p.x),sx=p.x<q.x?1:-1,dy=-abs(q.y-p.y),sy=p.y<q.y?1:-1,err=dx+dy;for(;;){c.fillRect(p.x-1,p.y-1,3,3,color);if(p.x==q.x&&p.y==q.y)break;int e=2*err;if(e>=dy){err+=dy;p.x+=sx;}if(e<=dx){err+=dx;p.y+=sy;}}};
    for(unsigned i=0;i<9;++i)line(roadPoints[i],roadPoints[i+1],UI_GOLD);
    for(unsigned i=0;i<4;++i){const auto& p=places[i];c.drawRect(p.x-8,p.y-8,17,17,i==g.city?UI_GREEN:i==menu.destination?UI_GOLD:UI_BLUE);}
    if(roll){
      // Dice box overlays the lower-right map, with a tumbling D20 outline.
      box(150,126,80,91);unsigned die=menu.rollReady?g.tripRoll:1+(frame*7)%20;int bounce=menu.rollReady?0:int(frame%3)*3;
      int cx=190+(menu.rollReady?0:(int(frame%3)-1)*4),cy=163-bounce;
      auto edge=[&](int a,int b,int d,int e){line({a,b},{d,e},UI_GOLD);};
      edge(cx,cy-26,cx+24,cy-12);edge(cx+24,cy-12,cx+23,cy+16);edge(cx+23,cy+16,cx,cy+27);edge(cx,cy+27,cx-24,cy+14);edge(cx-24,cy+14,cx-23,cy-12);edge(cx-23,cy-12,cx,cy-26);
      edge(cx-23,cy-12,cx+24,cy-12);edge(cx-23,cy-12,cx,cy+27);edge(cx+24,cy-12,cx,cy+27);
      snprintf(b,sizeof(b),"%u",die);text(cx-int(strlen(b))*6,cy-7,b,2,UI_WHITE);text(158,201,menu.rollReady?(rpg::tripSafe(g)?"SEGURO":"ENCONTRO"):"ROLANDO",1,menu.rollReady&&rpg::tripSafe(g)?UI_GREEN:UI_GOLD);
      snprintf(b,sizeof(b),"D20 + Sobrev %u + Sorte %u",g.tripSurvival,g.tripLuck);center(231,b);
      if(menu.rollReady){snprintf(b,sizeof(b),"%u + %u + %u = %u / CD %u",g.tripRoll,g.tripSurvival,g.tripLuck,g.tripTotal,g.tripDifficulty);center(250,b,1,UI_GOLD);button(14,272,212,rpg::tripSafe(g)?"Continuar viagem":"Enfrentar inimigo");}
      else center(281,"Rolando o dado...",1,UI_GOLD);
    }else if(travel){Point p=menu.journey.position();for(int y=0;y<24;++y)for(int x=0;x<22;++x){auto color=personalPixel(g,frame%6,(y*5)*112+x*5);if(color!=SPRITE_KEY)c.fillRect(p.x-11+x,p.y-27+y-int(frame%2),1,1,color);}c.fillRect(p.x-6-int(frame%3),p.y+1,3,2,UI_MUTED);
      snprintf(b,sizeof(b),"%s -> %s",placeName(menu.journey.from),placeName(menu.journey.to));center(245,b);box(14,277,212,20);c.fillRect(17,280,206*menu.journey.progress/1000,14,UI_GREEN);}
    else{snprintf(b,sizeof(b),"Destino Nv %u+ / CD %u",rpg::cityLevel(menu.destination),rpg::routeDifficulty(g.city,menu.destination));center(231,b,1,g.p.level<rpg::cityLevel(menu.destination)?UI_RED:UI_WHITE);snprintf(b,sizeof(b),"Farol: %s",menu.destination==0?"Raiz":menu.destination==1?"Memoria":menu.destination==2?"Caminho":"Juramento");center(250,b,1,UI_GOLD);button(14,272,102,menu.destination==g.city?"Entrar":"Viajar");button(124,272,102,"Voltar");}return true;}
  return false;
}
