#pragma once
#include "Narrative.h"
template<class Canvas> void drawNarrative(Canvas& c,const rpg::Game& g,const ViewState& v){
 auto text=[&](int x,int y,const char* s,int size=1,uint16_t color=UI_WHITE){c.fillRect(x-2,y-2,int(strlen(s))*6*size+4,size*8+4,UI_INK);c.setTextSize(size);c.setTextColor(color);c.setCursor(x,y);c.print(s);};
 auto center=[&](int y,const char* s,int size=1,uint16_t color=UI_WHITE){text((240-int(strlen(s))*6*size)/2,y,s,size,color);};
 auto button=[&](int x,int y,int w,const char* s){c.fillRect(x,y,w,40,UI_PANEL);c.drawRect(x,y,w,40,UI_GOLD);text(x+(w-int(strlen(s))*6)/2,y+15,s);};
 char b[64];
 if(v.page==Page::CampaignTask){
  unsigned mission=v.campaignChoice;drawBackdrop(c,bg_port);center(12,"MISSAO DE SABELA",2,UI_GOLD);center(48,rpg::campaignTitle(mission),1,UI_GOLD);
  const char* proof[]={"Cristais saem pelas cisternas.","Enfrente a escolta da Mao de Cinza.","Sabela recolhera as provas.","A pista nao depende do saque."};
  const char* rescue[]={"Uma tempestade apagou o farol.","Refugiados estao presos no cais.","Derrote o bloqueio da Mao de Cinza.","A guarda conduzira o resgate."};
  for(unsigned i=0;i<4;++i)center(82+i*24,mission==1?proof[i]:rescue[i]);
  center(186,"Mares / requer nivel 10",1,UI_GOLD);snprintf(b,sizeof(b),"Contrato: +%u ouro / +%u XP",rpg::campaignGold(mission),rpg::campaignXp(g,mission));center(208,b);
  center(231,"Use bolsa e poderes para se preparar.");center(252,v.message,1,UI_GOLD);button(14,272,102,"Voltar");button(124,272,102,"Comecar");return;
 }
 if(v.page==Page::CampaignResult){
  bool won=g.phase==rpg::Phase::Won;unsigned mission=g.campaignStage;drawBackdrop(c,bg_port);center(12,won?"MARES RESPONDE":"A MISSAO CONTINUA",2,UI_GOLD);
  center(48,rpg::campaignTitle(mission),1,UI_GOLD);
  if(won){const auto& scene=story::portScenes[mission==2?1:0][std::min(3u,unsigned(v.campaignScene))];center(81,scene.speaker,1,UI_GOLD);for(unsigned i=0;i<4;++i)center(112+i*25,scene.lines[i]);
   snprintf(b,sizeof(b),"Recompensa: +%u ouro / +%u XP",rpg::campaignGold(mission),rpg::campaignXp(g,mission));center(232,b);button(14,272,212,v.campaignScene<3?"Proxima":"Registrar no diario");}
  else {center(106,g.phase==rpg::Phase::Lost?"Sabela organiza seu resgate.":"Voce recua; a guarda segura o cais.");center(140,"Sem contrato pago nem pista perdida.");center(174,"Prepare-se e tente outra vez.");center(220,"Nenhum refugiado e perdido por falhar.");button(14,272,212,g.phase==rpg::Phase::Lost?"Recuperar forcas":"Voltar ao porto");}return;
 }
 if(v.page==Page::Campaign){drawBackdrop(c,bg_character);auto goal=story::objective(g);center(12,"SUA JORNADA",2,UI_GOLD);center(48,goal.title,1,UI_GOLD);center(78,goal.place,1,UI_GREEN);for(unsigned i=0;i<4;++i)center(108+i*23,goal.lines[i]);c.fillRect(14,198,212,24,UI_PANEL);c.drawRect(14,198,212,24,UI_GOLD);center(205,"Relembrar o inicio",1,UI_GOLD);button(14,224,212,"Seguir objetivo");button(14,272,102,"Diario");button(124,272,102,"Voltar");return;}
 if(v.page==Page::Prologue){const auto& scene=story::opening[menu.storyIndex%4];drawBackdrop(c,menu.storyIndex==0?bg_explore0:menu.storyIndex==3?bg_guild:bg_tavern);
  center(15,"AS CINZAS DA",2,UI_GOLD);center(36,"PRIMEIRA AURORA",2,UI_GOLD);center(75,scene.speaker,1,UI_GOLD);
  for(unsigned i=0;i<4;++i)center(116+i*25,scene.lines[i]);snprintf(b,sizeof(b),"Aeldra / cena %u de 4",menu.storyIndex+1);center(231,b);
  button(14,272,102,g.tutorial?"Voltar":"Pular");button(124,272,102,menu.storyIndex==3?"Continuar":"Proxima");return;
 }
 drawBackdrop(c,v.page==Page::Continent?bg_world:v.page==Page::People||v.page==Page::Dialogue?(g.city==0?bg_explore0:g.city==1?bg_explore1:g.city==2?bg_port:bg_castle):bg_character);
 if(v.page==Page::Continent){const auto& r=story::regions[menu.regionIndex%8];center(12,"VALDARIA",2,UI_GOLD);center(49,"Continente / atlas");center(86,r.name,2,UI_GOLD);center(114,r.subtitle);center(150,r.route);
  center(178,menu.regionIndex?"Regiao futura / mapa em preparo":"Aeldra e a regiao onde voce esta");center(202,menu.regionIndex?"Sem viagem disponivel ainda":"Carvalho, Ruinas, Mares e Aurora");
  button(14,224,102,"Anterior");button(124,224,102,"Proxima");button(14,272,102,"Voltar");button(124,272,102,menu.regionIndex?"Em preparo":"Mapa Aeldra");return;
 }
 if(v.page==Page::People){center(12,"PESSOAS",2,UI_GOLD);center(45,placeName(g.city));
  for(unsigned i=0;i<3;++i){const auto& p=story::person(g.city,i);c.fillRect(14,78+i*54,212,48,UI_PANEL);c.drawRect(14,78+i*54,212,48,UI_GOLD);center(87+i*54,p.name,1,UI_GOLD);center(107+i*54,p.role);}
  center(249,"Toque no nome para conversar");button(14,272,212,"Voltar");return;
 }
 if(v.page==Page::Dialogue){const auto& p=story::person(g.city,menu.personIndex);center(14,p.name,1,UI_GOLD);center(41,p.role);
  if(g.city==1&&menu.personIndex==2&&!story::arconteKnown(g)){center(105,"Um eco permanece na Cripta.");center(133,"Encontre-o antes de ouvir sua voz.");}
  else if(g.city==0&&menu.personIndex==0){center(87,story::classVoice(g.p.cls),1,UI_GOLD);center(116,"O mundo esquece. Nos resistimos.");center(145,g.guardianDefeated?"Voce abriu caminho em Vespera.":"Elarin e Borin precisam de voce.");center(174,story::arconteKnown(g)?"Leve as pistas para Mares.":"Nao deixe a estrada apagar seu nome.");}
  else if(g.city==1&&menu.personIndex==0&&g.guardianDefeated){center(87,"O Guardiao nao era o Arconte.");center(116,"Era a sentinela do Farol Memoria.");center(145,story::arconteKnown(g)?"O livro prova o custo do Pacto.":"Seu cristal abre a Cripta de Vaelor.");center(174,story::arconteKnown(g)?"Sabela precisa ouvir isso em Mares.":"Procure a verdade alem dos selos.");}
  else if(g.city==2&&menu.personIndex==0&&g.campaignFlags==3){center(87,"O porto esta aberto aos refugiados.");center(116,"Nilsa religou o Farol do Caminho.");center(145,"Leve o Livro e os registros a Liora.");center(174,"Aurora deve responder pelas cargas.");}
  else if(g.city==2&&menu.personIndex==0&&(g.campaignFlags&1)){center(87,"A prova liga as cargas a Coroa.");center(116,"Maelis abriu os arquivos da Guilda.");center(145,"Agora ha refugiados presos no cais.");center(174,"Ajude-nos a abrir a passagem.");}
  else if(g.city==3&&menu.personIndex==0&&g.campaignFlags==3){center(87,"Recebi os registros de Maelis.");center(116,"Meu pai sabia o preco do Pacto.");center(145,"Seraphine e Dargan vao nos ajudar.");center(174,"O ato de Aurora esta em preparo.");}
  else for(unsigned i=0;i<4;++i)center(87+i*29,p.lines[i]);
  if(g.city==2&&menu.personIndex==0&&story::arconteKnown(g)&&rpg::campaignMission(g)){c.fillRect(14,225,212,32,UI_PANEL);c.drawRect(14,225,212,32,UI_GOLD);center(236,g.campaignFlags&1?"Resgatar refugiados":"Investigar a carga",1,UI_GOLD);}
  else center(223,"As Cinzas da Primeira Aurora",1,UI_GOLD);button(14,272,212,"Voltar");return;
 }
 unsigned index=menu.chapterIndex%6;center(12,"DIARIO DE AELDRA",2,UI_GOLD);center(46,story::chapterNames[index],1,UI_GOLD);
 bool known=index<=story::knownChapter(g);const auto& chapter=story::chapters[index];center(73,known?chapter.speaker:"AINDA POR DESCOBRIR",1,known?UI_GREEN:UI_MUTED);
 if(known){for(unsigned i=0;i<4;++i)center(100+i*23,chapter.lines[i]);}
 else {center(113,"Continue a jornada em Aeldra.");center(143,"Esta pagina evita revelar");center(165,"o que voce ainda nao descobriu.");}
 if(index==3&&(g.campaignFlags&1))center(191,"Cargas: provas recuperadas",1,UI_GREEN);
 if(index==3&&(g.campaignFlags&2))center(209,"Refugiados: resgatados",1,UI_GREEN);
 if(index==2&&g.guardianDefeated)center(191,"Guardiao do Limiar: vencido",1,UI_GREEN);
 if(index==2&&story::arconteKnown(g))center(209,"Cripta de Vaelor: concluida",1,UI_GREEN);
 button(14,224,102,"Anterior");button(124,224,102,"Proxima");
 button(14,272,102,"Voltar");button(124,272,102,"Objetivo");
}
