#pragma once
#include "GuildEvents.h"
template<class Canvas,class Text,class Center,class Box,class Button>bool renderEvent(Canvas& c,const rpg::Game& g,int page,Text text,Center center,Box box,Button button,unsigned frame){
 if(page<73||page>77)return false;char b[64];
 if(page==73){center(12,"CARTAS DA GUILDA",2,UI_GOLD);center(52,"Maelis Voss / Aeldra");center(83,g.eventStage==1?"Uma carta aguarda sua resposta":g.eventStage==2?"Missao em andamento":g.eventStage==3?"Missao concluida":g.eventStage==4?"Carta recusada":g.eventStage>=5?"Missao encerrada":"Nenhuma carta hoje");
 if(g.eventStage==1||g.eventStage==2){box(14,114,212,76);center(129,"ASAS SOBRE OS TELHADOS",1,UI_GOLD);center(158,rpg::eventPlace(g.eventTier));button(14,216,212,g.eventStage==2?"Retomar missao":"Abrir carta");}
 else {center(140,"Oferta diaria a partir das 09h");center(162,"Requer hora local valida.");center(194,"Piloto: uma missao por dia.");}button(14,272,212,"Voltar");return true;}
 if(page==74){unsigned elapsed=uint32_t(menu.renderNow-menu.letterStarted);if(elapsed<450){int h=std::max(8u,177*elapsed/450);c.fillRect(10,130-h/2,220,h,0xcdd4);c.drawRect(10,130-h/2,220,h,0x8a65);center(11,"CARTA DE MAELIS",2,UI_GOLD);return true;}c.fillRect(10,41,220,177,0xcdd4);c.drawRect(10,41,220,177,0x8a65);
  center(11,"CARTA DE MAELIS",2,UI_GOLD);center(51,"Guilda dos Aventureiros",1,UI_GOLD);center(74,"Asas sobre os telhados",1,UI_GOLD);
  center(97,"Caro aventureiro,");center(114,"Um hipogrifo assustado ameaca");center(131,rpg::eventPlace(g.eventTier));center(148,"Afaste-o antes que alguem se fira.");center(165,"Ha uma luz estranha nas penas.");center(183,*menu.notice?menu.notice:"Passagem paga / 2-4 min",1,*menu.notice?UI_RED:UI_WHITE);snprintf(b,sizeof(b),"Recompensa: %u ouro / %u XP",rpg::eventGold(g.eventTier),rpg::eventXp(g.eventTier));center(206,b,1,UI_GOLD);button(14,222,212,"Fechar carta");
  button(14,272,102,"Recusar");button(124,272,102,"Aceitar");return true;}
 if(page==75){center(12,"RECUSAR CARTA?",2,UI_GOLD);center(116,"Esta oferta termina por hoje.");center(145,"Nao sera sorteada outra no lugar.");button(14,272,102,"Cancelar");button(124,272,102,"Recusar");return true;}
 if(page==76){center(18,"PASSAGEM DA GUILDA",2,UI_GOLD);center(100,"Maelis providenciou sua passagem.");center(132,rpg::eventPlace(g.eventTier));center(170,"Prepare sua arma...");return true;}
 bool won=g.phase==rpg::Phase::Won;center(14,won?"CIDADE PROTEGIDA":g.phase==rpg::Phase::Lost?"RESGATE DA GUILDA":"RETIRADA SEGURA",2,won?UI_GREEN:UI_GOLD);
 center(81,won?"O hipogrifo fugiu para o ceu.":"A missao terminou sem recompensa.");center(110,won?"A ave foi poupada.":g.phase==rpg::Phase::Lost?"Voce volta com 1 HP.":"Seu HP e MP foram preservados.");
 if(won){snprintf(b,sizeof(b),"%u ouro / %u XP",rpg::eventGold(g.eventTier),rpg::eventXp(g.eventTier));center(153,b,2,UI_GOLD);center(186,"Maelis agradece sua ajuda.");}else if(g.phase==rpg::Phase::Lost){snprintf(b,sizeof(b),"Perda de XP: %u",g.gainXp);center(153,b);}
 center(228,"Retorno ao seu local de origem.");button(14,272,212,won?"Receber e voltar":"Voltar");return true;
}
