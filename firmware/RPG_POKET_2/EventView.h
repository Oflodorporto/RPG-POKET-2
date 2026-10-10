#pragma once
#include "GuildEvents.h"
template<class Canvas,class Text,class Center,class Box,class Button>bool renderEvent(Canvas& c,const rpg::Game& g,int page,Text text,Center center,Box box,Button button,unsigned frame){
 if((page<73||page>77)&&page!=int(Page::EventMission)&&page!=int(Page::EventAbandon))return false;char b[64];
 if(page==73){center(18,"CARTAS DA GUILDA",2,UI_GOLD);center(52,"Maelis Voss / Aeldra");center(83,g.eventStage==1?"Uma carta aguarda sua resposta":g.eventStage==2?"Missao em andamento":g.eventStage==3?"Missao concluida":g.eventStage==4?"Carta recusada":g.eventStage>=5?"Missao encerrada":"Nenhuma carta hoje");
 if(g.eventStage==1||g.eventStage==2){center(120,rpg::eventTitle(g),1,UI_GOLD);center(146,rpg::eventPlace(g.eventTier));snprintf(b,sizeof(b),"Carta %u de 2 / %s",unsigned(g.eventOfferSlot)+1,g.eventOfferSlot?"entardecer":"manha");center(175,b);button(14,216,212,g.eventStage==2?"Retomar missao":"Abrir carta");}
 else {center(125,"Cartas a partir das 09h e 18h.");center(148,"Uma carta espera sua resposta.");center(171,"Nao precisa ficar conectado.");center(194,"Requer hora local valida.");}button(14,272,212,"Voltar");return true;}
 if(page==74){unsigned elapsed=uint32_t(menu.renderNow-menu.letterStarted);if(elapsed<450){int h=std::max(8u,177*elapsed/450);c.fillRect(10,130-h/2,220,h,0xcdd4);c.drawRect(10,130-h/2,220,h,0x8a65);center(11,"CARTA DE MAELIS",2,UI_GOLD);return true;}c.fillRect(10,41,220,177,0xcdd4);c.drawRect(10,41,220,177,0x8a65);
  auto ink=[&](int y,const char* words,uint16_t color=0x4208){c.setTextColor(color);c.setTextSize(1);c.setCursor((240-int(strlen(words))*6)/2,y);c.print(words);};
  center(11,"CARTA DE MAELIS",2,UI_GOLD);npcPortrait(c,story::Npc::Maelis,18,45,24);ink(51,"Guilda dos Aventureiros",0x8a65);ink(74,rpg::eventTitle(g),0x7800);
  ink(97,"Caro aventureiro,");
  const char* ending="Passagem paga / 2-4 min";
  if(g.eventKind==1){const char* lines[4][5]={
   {"Uma carroca tombou no pomar.","Nara precisa das ervas perdidas","para tratar duas criancas.","Traga duas cargas. Os lobos","ainda rondam a trilha."},
   {"Iria deixou seus cadernos","ao fugir de um desabamento.","Sem eles, perderemos relatos","dos farois. Recupere dois","volumes entre as pedras."},
   {"Tomas perdeu carga quando o","cais cedeu. As familias do porto","dependem desses sacos. Traga","duas cargas pelas trilhas","junto ao mar."},
   {"Liora escondeu dois registros","para salva-los de uma patrulha.","Eles podem provar abusos da","guarda. Traga as duas cargas.","As familias merecem respostas."}};
   for(unsigned i=0;i<4;++i)ink(114+i*17,lines[g.eventTier][i]);ending=lines[g.eventTier][4];
  }else if(g.eventKind==2){const char* lines[4][5]={
   {"","","","",""},
   {"Iria resgatou uma testemunha.","Ela viu sinais da Mao de Cinza.","Escolte-a de Ruinas a Carvalho.","Preciso ouvir seu relato.","Ela nao ousa viajar sozinha."},
   {"Tomas precisa entregar mapas","que revelam rotas escondidas.","Escolte-o de Mares a Ruinas.","Ha saqueadores na estrada;","ele nao pode perder os mapas."},
   {"Liora nos confiou uma mensagem","destinada aos aliados de Mares.","Escolte o mensageiro de Aurora","a Mares. A guarda segue seus","passos. Nao o deixe sozinho."}};
   for(unsigned i=0;i<4;++i)ink(114+i*17,lines[g.eventTier][i]);ending=lines[g.eventTier][4];
  }else {ink(114,"Um hipogrifo assustado ameaca");ink(131,rpg::eventPlace(g.eventTier));ink(148,"Afaste-o antes que alguem se fira.");ink(165,"Ha uma luz estranha nas penas.");}
  ink(183,*menu.notice?menu.notice:ending,*menu.notice?0xa800:0x4208);snprintf(b,sizeof(b),"Recompensa: %u ouro / %u XP",rpg::eventGold(g),rpg::eventXp(g));ink(206,b,0x0320);button(14,222,212,"Fechar carta");
  button(14,272,102,"Recusar");button(124,272,102,"Aceitar");return true;}
 if(page==75){center(12,"RECUSAR CARTA?",2,UI_GOLD);center(99,"Voce pode recusar sem punicao.");center(124,"Esta carta sera encerrada.");center(149,"Nao ha troca da mesma oferta.");center(185,g.eventOfferSlot?"Nova oportunidade em outro dia.":"A segunda carta chega apos 18h.");button(14,272,102,"Cancelar");button(124,272,102,"Recusar");return true;}
 if(page==76){center(18,"PASSAGEM DA GUILDA",2,UI_GOLD);center(100,"Maelis providenciou sua passagem.");center(132,rpg::eventPlace(g.eventTier));center(170,g.eventKind==1?"A busca comeca nas trilhas...":g.eventKind==2?"Seu passageiro aguarda...":"Prepare sua arma...");c.fillRect(34,210,172,3,UI_GOLD);unsigned progress=std::min(700u,unsigned(uint32_t(menu.renderNow-menu.letterStarted)));c.fillRect(31+172*progress/700,202,9,18,UI_WHITE);return true;}
 if(page==int(Page::EventMission)){
  center(18,"CARTA EM ANDAMENTO",2,UI_GOLD);npcPortrait(c,story::Npc::Maelis,108,46,24);center(85,rpg::eventTitle(g),1,UI_GOLD);
  if(g.eventKind==1){center(110,rpg::eventPlace(g.eventTier));center(137,"Procure a carga pelas trilhas.");center(154,"Inimigos podem cruzar seu caminho.");snprintf(b,sizeof(b),"Cargas da missao: %u / 2",g.eventProgress);center(179,b,1,UI_GREEN);button(14,216,212,"Procurar carga");}
  else {snprintf(b,sizeof(b),"%s -> %s",rpg::eventCityName(g.eventTier),rpg::eventCityName(rpg::eventDestination(g)));center(115,b,1,UI_GOLD);center(140,"Seu passageiro confia em voce.");center(162,"D20 decide os perigos da viagem.");center(184,"Chegue ao destino para entrega.");button(14,216,212,"Iniciar escolta");}
  if(*menu.notice)center(201,menu.notice,1,UI_RED);button(14,272,102,"Bolsa");button(124,272,102,"Retornar");return true;
 }
 if(page==int(Page::EventAbandon)){center(18,"ENCERRAR MISSAO?",2,UI_GOLD);center(99,"Voce voltara ao local de origem.");center(128,"A tarefa ficara incompleta.");center(157,"Nao ha recompensa da carta.");center(186,"Itens pessoais sao preservados.");button(14,272,102,"Continuar");button(124,272,102,"Retornar");return true;}
 if(g.eventKind){bool ready=rpg::eventReady(g),battle=g.phase==rpg::Phase::Won;
  center(18,ready?"MISSAO CUMPRIDA":battle?"CAMINHO LIVRE":"MISSAO ENCERRADA",2,ready?UI_GREEN:UI_GOLD);
  if(ready){center(89,g.eventKind==1?"As duas cargas estao recuperadas.":"Seu passageiro chegou a salvo.");center(116,"Maelis agradece sua ajuda.");snprintf(b,sizeof(b),"+%u ouro",rpg::eventGold(g));center(147,b,2,UI_GOLD);snprintf(b,sizeof(b),"+%u XP",rpg::eventXp(g));center(175,b,2,UI_GOLD);center(225,"Retorno ao seu local de origem.");}
  else if(battle){center(90,"O inimigo nao impede mais voce.");snprintf(b,sizeof(b),"+%u XP / +%u ouro",g.gainXp,g.gainGold);center(121,b);center(164,"A tarefa da carta continua.");center(197,"A viagem e a busca nao reiniciam.");}
  else {center(99,"A Guilda providencia seu retorno.");center(134,"Sem recompensa pela carta.");center(169,g.phase==rpg::Phase::Lost?"Voce retorna com 1 HP.":"Seu HP e MP foram preservados.");}
  button(14,272,212,ready?"Receber e voltar":battle?"Continuar missao":"Voltar");return true;
 }
 bool won=g.phase==rpg::Phase::Won;center(14,won?"CIDADE PROTEGIDA":g.phase==rpg::Phase::Lost?"RESGATE DA GUILDA":"RETIRADA SEGURA",2,won?UI_GREEN:UI_GOLD);
 center(81,won?"O hipogrifo fugiu para o ceu.":"A missao terminou sem recompensa.");center(110,won?"A ave foi poupada.":g.phase==rpg::Phase::Lost?"Voce volta com 1 HP.":"Seu HP e MP foram preservados.");
 if(won){snprintf(b,sizeof(b),"%u ouro / %u XP",rpg::eventGold(g),rpg::eventXp(g));center(153,b,2,UI_GOLD);center(186,"Maelis agradece sua ajuda.");}else if(g.phase==rpg::Phase::Lost){snprintf(b,sizeof(b),"Perda de XP: %u",g.gainXp);center(153,b);}
 center(228,"Retorno ao seu local de origem.");button(14,272,212,won?"Receber e voltar":"Voltar");return true;
}
