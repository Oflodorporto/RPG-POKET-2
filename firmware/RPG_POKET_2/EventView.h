#pragma once
#include "EventCatalog.h"
template<class Canvas,class Text,class Center,class Box,class Button>bool renderEvent(Canvas& c,const rpg::Game& g,int page,Text text,Center center,Box box,Button button,unsigned frame){
 if((page<73||page>77)&&page!=int(Page::EventMission)&&page!=int(Page::EventAbandon)&&page!=int(Page::EventChoice))return false;char b[64];
 if(page==73){center(18,"CARTAS DA GUILDA",2,UI_GOLD);center(52,"Maelis Voss / Aeldra");center(83,g.eventStage==1?"Uma carta aguarda sua resposta":g.eventStage==2?"Missao em andamento":g.eventStage==3?"Missao concluida":g.eventStage==4?"Carta recusada":g.eventStage>=5?"Missao encerrada":"Nenhuma carta hoje");
 if(g.eventStage==1||g.eventStage==2){center(120,rpg::eventTitle(g),1,UI_GOLD);center(146,rpg::eventPlace(g.eventTier));snprintf(b,sizeof(b),"Carta %u de 2 / %s",unsigned(g.eventOfferSlot)+1,g.eventOfferSlot?"entardecer":"manha");center(175,b);button(14,216,212,g.eventStage==2?"Retomar missao":"Abrir carta");}
 else {center(125,"Cartas a partir das 09h e 18h.");center(148,"Uma carta espera sua resposta.");center(171,"Nao precisa ficar conectado.");center(194,"Requer hora local valida.");}button(14,272,212,"Voltar");return true;}
 if(page==74){unsigned elapsed=uint32_t(menu.renderNow-menu.letterStarted);if(elapsed<450){int h=std::max(8u,177*elapsed/450);c.fillRect(10,130-h/2,220,h,0xcdd4);c.drawRect(10,130-h/2,220,h,0x8a65);center(11,"CARTA DA GUILDA",2,UI_GOLD);return true;}c.fillRect(10,41,220,177,0xcdd4);c.drawRect(10,41,220,177,0x8a65);
  auto ink=[&](int y,const char* words,uint16_t color=0x4208){c.setTextColor(color);c.setTextSize(1);c.setCursor((240-int(strlen(words))*6)/2,y);c.print(words);};
  center(11,"CARTA DA GUILDA",2,UI_GOLD);npcPortrait(c,eventSpeaker(g),18,45,24);ink(51,eventSender(g),0x8a65);ink(74,rpg::eventTitle(g),0x7800);
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
  }else if(g.eventKind>=3){for(unsigned i=0;i<4;++i)ink(114+i*17,eventBrief(g,i));ending=eventBrief(g,4);
  }else {ink(114,"Um hipogrifo assustado ameaca");ink(131,rpg::eventPlace(g.eventTier));ink(148,"Afaste-o antes que alguem se fira.");ink(165,"Ha uma luz estranha nas penas.");}
  ink(183,*menu.notice?menu.notice:ending,*menu.notice?0xa800:0x4208);snprintf(b,sizeof(b),"Recompensa: %u ouro / %u XP",rpg::eventGold(g),rpg::eventXp(g));ink(206,b,0x0320);button(14,222,212,"Fechar carta");
  button(14,272,102,"Recusar");button(124,272,102,"Aceitar");return true;}
 if(page==75){center(12,"RECUSAR CARTA?",2,UI_GOLD);center(99,"Voce pode recusar sem punicao.");center(124,"Esta carta sera encerrada.");center(149,"Nao ha troca da mesma oferta.");center(185,g.eventOfferSlot?"Nova oportunidade em outro dia.":"A segunda carta chega apos 18h.");button(14,272,102,"Cancelar");button(124,272,102,"Recusar");return true;}
 if(page==76){center(18,"PASSAGEM DA GUILDA",2,UI_GOLD);center(100,"Maelis providenciou sua passagem.");center(132,rpg::eventPlace(g.eventTier));center(170,g.eventKind>=3?"Seu contato espera por voce...":g.eventKind==1?"A busca comeca nas trilhas...":g.eventKind==2?"Seu passageiro aguarda...":"Prepare sua arma...");c.fillRect(34,210,172,3,UI_GOLD);unsigned progress=std::min(700u,unsigned(uint32_t(menu.renderNow-menu.letterStarted)));c.fillRect(31+172*progress/700,202,9,18,UI_WHITE);return true;}
 if(page==int(Page::EventChoice)){
  center(18,"SUA DECISAO",2,UI_GOLD);npcPortrait(c,eventSpeaker(g),108,44,24);center(83,eventSender(g),1,UI_GOLD);

  if(g.eventKind==6){for(unsigned i=0;i<3;++i)center(107+i*16,eventSealClue(g.eventTier,i));center(156,*menu.notice?menu.notice:g.eventProgress?"Qual ordem liberta as lembrancas?":"Examine cada selo antes de ordenar.");
    for(unsigned i=0;i<3;++i){snprintf(b,sizeof(b),"%s%s",g.eventProgress?eventOrder(i):(i==0?"Ler selo da Raiz":i==1?"Ler selo da Memoria":"Ler selo do Caminho"),!g.eventProgress&&(g.eventFlags&(1u<<i))?" / lido":"");int y=177+i*31;c.fillRect(14,y,212,28,UI_PANEL);c.drawRect(14,y,212,28,UI_GOLD);c.setTextColor(UI_WHITE);c.setTextSize(1);c.setCursor((240-int(strlen(b))*6)/2,y+10);c.print(b);}}
  else {center(111,g.eventKind==4?"D20 + saber ou sobrevivencia.":g.eventKind==5?"A pocao ajuda o viajante ferido.":"Defesas protegem os dois ataques.");
    center(132,g.eventKind==4?"Ajuste melhora a cada tentativa.":g.eventKind==5?"Pocao: +4 no retorno ao abrigo.":"Uma racao alimenta as tochas.");
    const char* a=g.eventKind==4?"Estudar / Inteligencia":g.eventKind==5?"Improvisar curativo / gratis":"Erguer barricadas / gratis";
    const char* z=g.eventKind==4?"Encaixar / Sobrevivencia":g.eventKind==5?"Usar 1 pocao de vida":"Tochas / usar 1 racao";
    button(14,177,212,a);button(14,220,212,z);}
  if(*menu.notice&&g.eventKind!=6)center(263,menu.notice,1,UI_RED);button(14,276,212,"Voltar");return true;
 }
 if(page==int(Page::EventMission)){
  center(18,"CARTA EM ANDAMENTO",2,UI_GOLD);npcPortrait(c,eventSpeaker(g),108,46,24);center(85,rpg::eventTitle(g),1,UI_GOLD);
  if(g.eventKind==1){center(110,rpg::eventPlace(g.eventTier));center(137,"Procure a carga pelas trilhas.");center(154,"Inimigos podem cruzar seu caminho.");snprintf(b,sizeof(b),"Cargas da missao: %u / 2",g.eventProgress);center(179,b,1,UI_GREEN);button(14,216,212,"Procurar carga");}
  else if(g.eventKind==2){snprintf(b,sizeof(b),"%s -> %s",rpg::eventCityName(g.eventTier),rpg::eventCityName(rpg::eventDestination(g)));center(115,b,1,UI_GOLD);center(140,"Seu passageiro confia em voce.");center(162,"D20 decide os perigos da viagem.");center(184,"Chegue ao destino para entrega.");button(14,216,212,"Iniciar escolta");}
  else {center(110,rpg::eventCityName(g.eventTier));const char* lines[5][4]={
   {"O sino chama por baixo das pedras.","Encontre os ecos e a corda do sino.","Toque na cena para interagir.","Entrada paga; sem cristal pessoal."},
   {"Uma pequena luz guia os viajantes.","A lente precisa voltar ao encaixe.","Quando acender, proteja o marco.","Nao altera os farois da campanha."},
   {"Alguem deixou rastros na estrada.","Ha viajantes precisando de ajuda.","Curativos improvisados sao gratis.","Pocao opcional melhora o retorno."},
   {"Tres selos guardam uma lembranca.","Leia todos; as pistas dao a ordem.","Erro acorda um eco; pode tentar.","Cada eco so e enfrentado uma vez."},
   {"O grupo se reuniu ao redor do fogo.","Prepare o abrigo; proteja duas ondas.","Defesas reduzem o primeiro golpe.","Recompensa extra: 1 racao (max9)."}};
   for(unsigned i=0;i<3;++i)center(131+i*17,lines[g.eventKind-3][i]);
   snprintf(b,sizeof(b),"Etapa %u/%u%s",g.eventProgress+1,rpg::eventGoal(g.eventKind),rpg::eventNight(g)?" / noite":" / dia");center(184,b,1,UI_GREEN);button(14,216,212,eventActionName(g));
   if(g.eventRoll&&g.eventKind!=6){snprintf(b,sizeof(b),"Ultimo D20: %u",g.eventRoll);center(252,b,1,UI_GOLD);}
  }
  if(*menu.notice)center(201,menu.notice,1,UI_RED);button(14,272,102,"Bolsa");button(124,272,102,"Retornar");return true;
 }
 if(page==int(Page::EventAbandon)){center(18,"ENCERRAR MISSAO?",2,UI_GOLD);center(99,"Voce voltara ao local de origem.");center(128,"A tarefa ficara incompleta.");center(157,"Nao ha recompensa da carta.");center(186,"Itens pessoais sao preservados.");button(14,272,102,"Continuar");button(124,272,102,"Retornar");return true;}
 if(g.eventKind){bool ready=rpg::eventReady(g),battle=g.phase==rpg::Phase::Won;
  center(18,ready?"MISSAO CUMPRIDA":battle?"CAMINHO LIVRE":"MISSAO ENCERRADA",2,ready?UI_GREEN:UI_GOLD);
  if(ready){center(89,eventCompletion(g));center(116,"A Guilda agradece sua ajuda.");snprintf(b,sizeof(b),"+%u ouro",rpg::eventGold(g));center(147,b,2,UI_GOLD);snprintf(b,sizeof(b),"+%u XP",rpg::eventXp(g));center(175,b,2,UI_GOLD);center(225,g.eventKind==7?"Extra: 1 racao na bolsa (max9).":"Retorno ao seu local de origem.");}
  else if(battle){center(90,"O inimigo nao impede mais voce.");snprintf(b,sizeof(b),"+%u XP / +%u ouro",g.gainXp,g.gainGold);center(121,b);center(164,"A tarefa da carta continua.");center(197,"A viagem e a busca nao reiniciam.");}
  else {center(99,"A Guilda providencia seu retorno.");center(134,"Sem recompensa pela carta.");center(169,g.phase==rpg::Phase::Lost?"Voce retorna com 1 HP.":"Seu HP e MP foram preservados.");}
  button(14,272,212,ready?"Receber e voltar":battle?"Continuar missao":"Voltar");return true;
 }
 bool won=g.phase==rpg::Phase::Won;center(14,won?"CIDADE PROTEGIDA":g.phase==rpg::Phase::Lost?"RESGATE DA GUILDA":"RETIRADA SEGURA",2,won?UI_GREEN:UI_GOLD);
 center(81,won?"O hipogrifo fugiu para o ceu.":"A missao terminou sem recompensa.");center(110,won?"A ave foi poupada.":g.phase==rpg::Phase::Lost?"Voce volta com 1 HP.":"Seu HP e MP foram preservados.");
 if(won){snprintf(b,sizeof(b),"%u ouro / %u XP",rpg::eventGold(g),rpg::eventXp(g));center(153,b,2,UI_GOLD);center(186,"Maelis agradece sua ajuda.");}else if(g.phase==rpg::Phase::Lost){snprintf(b,sizeof(b),"Perda de XP: %u",g.gainXp);center(153,b);}
 center(228,"Retorno ao seu local de origem.");button(14,272,212,won?"Receber e voltar":"Voltar");return true;
}
