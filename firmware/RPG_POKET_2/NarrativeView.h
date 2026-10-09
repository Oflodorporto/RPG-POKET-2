#pragma once
#include "DialogueStory.h"
template<class Canvas> void drawNarrative(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame=0){
 auto text=[&](int x,int y,const char* s,int size=1,uint16_t color=UI_WHITE){c.fillRect(x-2,y-2,int(strlen(s))*6*size+4,size*8+4,UI_INK);c.setTextSize(size);c.setTextColor(color);c.setCursor(x,y);c.print(s);};
 auto center=[&](int y,const char* s,int size=1,uint16_t color=UI_WHITE){text((240-int(strlen(s))*6*size)/2,y,s,size,color);};
 auto button=[&](int x,int y,int w,const char* s){c.fillRect(x,y,w,40,UI_PANEL);c.drawRect(x,y,w,40,UI_GOLD);text(x+(w-int(strlen(s))*6)/2,y+15,s);};
 char b[64];
 if(v.page==Page::ContributionResult||(v.page==Page::CampaignResult&&g.campaignStage>=4&&g.phase==rpg::Phase::Won)){
  unsigned m=v.page==Page::ContributionResult?v.campaignChoice:g.campaignStage,pages=story::contributionPages(m),page=std::min(unsigned(v.campaignScene),pages-1);drawBackdrop(c,g.city==0?bg_explore0:g.city==1?bg_explore1:g.city==2?bg_port:bg_castle);
  c.fillRect(8,8,224,253,0xf6d4);c.drawRect(8,8,224,253,0xa3c9);auto npc=story::cityNpc(rpg::campaignCity(m),rpg::campaignPerson(m));npcPortrait(c,npc,16,38,48);
  auto inkText=[&](int x,int y,const char* line,int size=1){c.setTextSize(size);c.setTextColor(0x30e4);c.setCursor(x,y);c.print(line);};auto inkCenter=[&](int y,const char* line){inkText((240-int(strlen(line))*6)/2,y,line);};
  inkCenter(19,story::contributionContact(m));inkText(74,47,"O LEGADO DE ANWEN");snprintf(b,sizeof(b),"Pagina %u / %u",page+1,pages);inkText(74,69,b);
  story::wrapStory(story::contributionSpeech(m),18,[&](unsigned row,const char* line){if(row/7==page)inkText(12,102+(row%7)*17,line,2);});
  inkCenter(236,v.page==Page::ContributionResult?"Entrega registrada no diario":"Ao concluir, registrar no diario");
  if(v.page==Page::ContributionResult){button(14,272,102,page?"Anterior":"Voltar");button(124,272,102,page+1<pages?"Proxima":"Fechar");}else button(14,272,212,page+1<pages?"Proxima":"Registrar no diario");return;
 }
 if(v.page==Page::CampaignResult&&g.campaignStage>=4){drawBackdrop(c,g.city==1?bg_explore1:bg_port);center(16,"A EQUIPE RECUOU",2,UI_GOLD);center(62,story::contributionContact(g.campaignStage),1,UI_GOLD);center(108,"Os aliados se abrigaram em seguranca.");center(139,"A contribuicao ainda esta pendente.");center(170,"Prepare-se e tente de novo.");center(208,"Nenhuma recompensa foi registrada.");button(14,272,212,g.phase==rpg::Phase::Lost?"Recuperar forcas":"Voltar ao aliado");return;}
 if(v.page==Page::CampaignTask&&v.campaignChoice>=4){
  unsigned m=v.campaignChoice;drawBackdrop(c,g.city==0?bg_explore0:g.city==1?bg_explore1:g.city==2?bg_port:bg_castle);center(12,"O LEGADO DE ANWEN",2,UI_GOLD);center(45,rpg::campaignTitle(m),1,UI_GOLD);npcPortrait(c,story::cityNpc(rpg::campaignCity(m),rpg::campaignPerson(m)),16,62,40);text(64,69,story::contributionContact(m),1,UI_GOLD);
  c.fillRect(8,111,224,132,UI_INK);c.drawRect(8,111,224,132,UI_GOLD);story::wrapStory(story::contributionBrief(m),18,[&](unsigned row,const char* line){c.setTextSize(2);c.setTextColor(UI_WHITE);c.setCursor(12,115+row*17);c.print(line);});
  center(93,m==4?"Entrega: 3 racoes / Nv18":m==7?"Componentes: 150 ouro / Nv18":"Proteja a equipe / Nv18");center(252,v.message,1,UI_GOLD);button(14,272,102,"Voltar");button(124,272,102,rpg::campaignDonation(m)?"Entregar":"Escoltar");return;
 }
 if(v.page==Page::CampaignTask){
  unsigned mission=v.campaignChoice;bool royal=mission==3;drawBackdrop(c,royal?bg_castle:bg_port);center(12,royal?"MISSAO DE LIORA":"MISSAO DE SABELA",2,UI_GOLD);center(48,rpg::campaignTitle(mission),1,UI_GOLD);
  const char* proof[]={"Cristais saem pelas cisternas.","Enfrente a escolta da Mao de Cinza.","Sabela recolhera as provas.","A pista nao depende do saque."};
  const char* rescue[]={"Uma tempestade apagou o farol.","Refugiados estao presos no cais.","Derrote o bloqueio da Mao de Cinza.","A guarda conduzira o resgate."};
  const char* archive[]={"Ordens antigas fecharam o arquivo.","Uma sentinela impede nossa passagem.","Abra caminho para Liora e a equipe.","Os documentos nao dependem do saque."};
  for(unsigned i=0;i<4;++i)center(82+i*24,royal?archive[i]:mission==1?proof[i]:rescue[i]);
  center(186,royal?"Aurora / requer nivel 18":"Mares / requer nivel 10",1,UI_GOLD);snprintf(b,sizeof(b),"Contrato: +%u ouro / +%u XP",rpg::campaignGold(mission),rpg::campaignXp(g,mission));center(208,b);
  center(231,"Use bolsa e poderes para se preparar.");center(252,v.message,1,UI_GOLD);button(14,272,102,"Voltar");button(124,272,102,"Comecar");return;
 }
 if(v.page==Page::CampaignResult){
  bool won=g.phase==rpg::Phase::Won;unsigned mission=g.campaignStage;bool royal=mission==3;drawBackdrop(c,royal?bg_castle:bg_port);center(12,won?(royal?"AURORA RESPONDE":"MARES RESPONDE"):"A MISSAO CONTINUA",2,UI_GOLD);
  center(48,rpg::campaignTitle(mission),1,UI_GOLD);
  if(won){const auto& scene=story::campaignScene(mission,v.campaignScene);auto npc=story::speakerNpc(scene.speaker);npcPortrait(c,npc,14,63,48);
   if(npc!=story::Npc::None){text(76,73,story::npcs[unsigned(npc)].name,1,UI_GOLD);text(76,95,strstr(scene.speaker,"CARTA")?"Carta da Guilda":strstr(scene.speaker,"REGISTRO")?"Registro do palacio":story::npcs[unsigned(npc)].role);}
   for(unsigned i=0;i<4;++i)center(131+i*22,scene.lines[i]);
   snprintf(b,sizeof(b),"Recompensa: +%u ouro / +%u XP",rpg::campaignGold(mission),rpg::campaignXp(g,mission));center(232,b);button(14,272,212,v.campaignScene+1<rpg::campaignScenes(mission)?"Proxima":"Registrar no diario");}
  else {center(106,g.phase==rpg::Phase::Lost?(royal?"Liora organiza seu resgate.":"Sabela organiza seu resgate."):(royal?"Voce recua; os aliados ficam a salvo.":"Voce recua; a guarda segura o cais."));center(140,"Sem contrato pago nem pista perdida.");center(204,"Prepare-se e tente outra vez.");center(220,royal?"Os registros aguardam nova tentativa.":"Nenhum refugiado e perdido por falhar.");button(14,272,212,g.phase==rpg::Phase::Lost?"Recuperar forcas":royal?"Voltar a Aurora":"Voltar ao porto");}return;
 }
 if(v.page==Page::Campaign){if(g.campaignFlags&4){drawBackdrop(c,bg_character);center(12,"O LEGADO DE ANWEN",2,UI_GOLD);center(48,rpg::anwenReady(g)?"As quatro cidades estao prontas":"Quatro ajudas / nenhum sacrificio",1,UI_GOLD);const char* city[]={"Carvalho / Elarin","Vespera / Iria","Mares / Nilsa","Aurora / Dargan"};for(unsigned i=0;i<4;++i){c.fillRect(14,74+i*36,212,30,UI_INK);c.drawRect(14,74+i*36,212,30,UI_GOLD);text(22,78+i*36,city[i]);text(22,92+i*36,rpg::contributionName(i),1,rpg::contribution(g,i)?UI_GREEN:UI_MUTED);text(184,78+i*36,rpg::contribution(g,i)?"PRONTA":"FALTA",1,rpg::contribution(g,i)?UI_GREEN:UI_MUTED);}button(14,224,212,rpg::anwenReady(g)?"Encontrar Liora":"Seguir proximo aliado");button(14,272,102,"Diario");button(124,272,102,"Voltar");return;}drawBackdrop(c,bg_character);auto goal=story::objective(g);center(12,"SUA JORNADA",2,UI_GOLD);center(48,goal.title,1,UI_GOLD);center(78,goal.place,1,UI_GREEN);for(unsigned i=0;i<4;++i)center(108+i*23,goal.lines[i]);c.fillRect(14,198,212,24,UI_PANEL);c.drawRect(14,198,212,24,UI_GOLD);center(205,"Relembrar o inicio",1,UI_GOLD);button(14,224,212,"Seguir objetivo");button(14,272,102,"Diario");button(124,272,102,"Voltar");return;}
 if(v.page==Page::Prologue){const auto& page=story::originPage(g,menu.storyIndex);drawBackdrop(c,page.npc==story::Npc::Maelis?bg_guild:page.npc==story::Npc::None?bg_explore0:bg_tavern);
  c.fillRect(8,8,224,253,UI_INK);c.drawRect(8,8,224,253,UI_GOLD);center(16,page.title,1,UI_GOLD);
  if(page.npc!=story::Npc::None){npcPortrait(c,page.npc,16,39,48);const auto& who=story::npcs[unsigned(page.npc)];text(74,45,who.name,1,UI_GOLD);text(74,66,who.role);}
  else {center(45,g.originStory&&menu.storyIndex<3?"VOCE / ANTES DA JORNADA":"NARRADOR",1,UI_GREEN);center(68,rpg::className(g.p.cls),1,UI_GOLD);}
  story::wrapStory(page.text,18,[&](unsigned row,const char* line){c.setTextSize(2);c.setTextColor(UI_WHITE);c.setCursor(12,92+row*17);c.print(line);});
  if(menu.storyIndex+1==story::originCount(g)&&g.dndProgression&&g.p.level==1)center(235,story::nextUnlock(g.p.cls),1,UI_GREEN);
  snprintf(b,sizeof(b),"Pagina %u / %u",menu.storyIndex+1,story::originCount(g));center(249,b,1,UI_MUTED);
  button(14,272,102,menu.storyIndex?"Voltar":menu.storyReplay?"Fechar":"Pular");button(124,272,102,menu.storyIndex+1==story::originCount(g)?"Continuar":"Avancar");return;
 }

 drawBackdrop(c,v.page==Page::Continent?bg_world:v.page==Page::People||v.page==Page::Dialogue?(g.city==0?bg_explore0:g.city==1?bg_explore1:g.city==2?bg_port:bg_castle):bg_character);
 if(v.page==Page::Continent){const auto& r=story::regions[menu.regionIndex%8];center(12,"VALDARIA",2,UI_GOLD);center(49,"Continente / atlas");center(86,r.name,2,UI_GOLD);center(114,r.subtitle);center(150,r.route);
  center(178,menu.regionIndex?"Regiao futura / mapa em preparo":"Aeldra e a regiao onde voce esta");center(202,menu.regionIndex?"Sem viagem disponivel ainda":"Carvalho, Ruinas, Mares e Aurora");
  button(14,224,102,"Anterior");button(124,224,102,"Proxima");button(14,272,102,"Voltar");button(124,272,102,menu.regionIndex?"Em preparo":"Mapa Aeldra");return;
 }
 if(v.page==Page::People){center(12,"PESSOAS",2,UI_GOLD);center(45,placeName(g.city));
  for(unsigned i=0;i<3;++i){const auto& p=story::person(g.city,i);c.fillRect(14,78+i*54,212,48,UI_PANEL);c.drawRect(14,78+i*54,212,48,UI_GOLD);bool hidden=g.city==1&&i==2&&!story::arconteKnown(g);
   if(!hidden)npcPortrait(c,story::cityNpc(g.city,i),19,83+i*54,36);
   text(64,87+i*54,hidden?"Voz desconhecida":p.name,1,UI_GOLD);text(64,107+i*54,hidden?"Explore a Cripta":p.role);}
  center(249,"Toque no nome para conversar");button(14,272,212,"Voltar");return;
 }
 if(v.page==Page::Dialogue){
  bool hidden=g.city==1&&menu.personIndex==2&&!story::arconteKnown(g);auto npc=story::cityNpc(g.city,menu.personIndex);const auto& who=story::npcs[unsigned(npc)];const char* speech=story::conversation(g,menu.personIndex);unsigned pages=story::conversationPages(speech),page=std::min(unsigned(v.dialoguePage),pages-1);
  // Opening is a short, nonblocking visual effect; the whole page stays readable afterwards.
  unsigned age=frame-v.dialogueStartFrame;unsigned opening=v.dialogueAnimate?std::min(age,6u):6u;
  const uint16_t paper=0xf6d4,ink=0x30e4,edge=0xa3c9;
  int height=24+int(opening)*37,top=134-height/2;
  c.fillRect(7,top+3,226,height,0x18c3);c.fillRect(10,top,220,height,paper);c.drawRect(10,top,220,height,edge);
  for(int yy=top+12;yy<top+height-10;yy+=9)c.fillRect(14,yy,212,1,0xee72);
  auto roll=[&](int yy){c.fillRect(5,yy,230,8,edge);c.fillRect(8,yy+1,224,3,0xff18);c.drawRect(5,yy,230,8,ink);};
  roll(top-3);roll(top+height-4);
  auto paperText=[&](int x,int y,const char* line,int size=1){c.setTextSize(size);c.setTextColor(ink);c.setCursor(x,y);c.print(line);};
  auto paperCenter=[&](int y,const char* line){paperText((240-int(strlen(line))*6)/2,y,line);};
  if(opening==6){
   paperCenter(20,hidden?"VOZ DESCONHECIDA":who.name);
   if(!hidden){npcPortrait(c,npc,16,38,48);paperText(74,46,who.role);paperText(74,67,"CONVERSA / ");paperText(74,78,placeName(g.city));}
   else paperCenter(61,"Uma voz entre as pedras...");
   snprintf(b,sizeof(b),"Pagina %u / %u",page+1,pages);paperCenter(89,b);c.fillRect(18,99,204,1,edge);
   story::wrapStory(speech,18,[&](unsigned row,const char* line){if(row/7==page)paperText(12,105+(row%7)*17,line,2);});
   if(menu.personIndex==rpg::campaignPerson(rpg::campaignMission(g))&&rpg::campaignContact(g)){c.fillRect(14,225,212,32,UI_PANEL);c.drawRect(14,225,212,32,UI_GOLD);center(236,rpg::campaignMission(g)>=4?"Ajudar na restauracao":rpg::campaignMission(g)==3?"Abrir arquivos da Coroa":g.campaignFlags&1?"Resgatar refugiados":"Investigar a carga",1,UI_GOLD);}
  }
  button(14,272,102,page?"Anterior":"Voltar");button(124,272,102,page+1<pages?"Proxima":"Fechar");return;
 }
 unsigned index=menu.chapterIndex%6;center(12,"DIARIO DE AELDRA",2,UI_GOLD);center(46,story::chapterNames[index],1,UI_GOLD);
 bool known=index<=story::knownChapter(g);const auto& chapter=story::chapters[index];center(73,known?chapter.speaker:"AINDA POR DESCOBRIR",1,known?UI_GREEN:UI_MUTED);
 if(known){if(index==4&&(g.campaignFlags&4)){for(unsigned i=0;i<4;++i){snprintf(b,sizeof(b),"%s: %s",rpg::contributionName(i),rpg::contribution(g,i)?"pronta":"pendente");center(100+i*23,b,1,rpg::contribution(g,i)?UI_GREEN:UI_MUTED);}}else for(unsigned i=0;i<4;++i)center(100+i*23,chapter.lines[i]);}
 else {center(113,"Continue a jornada em Aeldra.");center(143,"Esta pagina evita revelar");center(165,"o que voce ainda nao descobriu.");}
 if(index==3&&(g.campaignFlags&1))center(191,"Cargas: provas recuperadas",1,UI_GREEN);
 if(index==3&&(g.campaignFlags&2))center(209,"Refugiados: resgatados",1,UI_GREEN);
 if(index==4&&(g.campaignFlags&4)){center(191,"Vigilia Perpetua: ordens reveladas",1,UI_GREEN);center(209,"Projeto de Anwen: protegido",1,UI_GREEN);}
 if(index==2&&g.guardianDefeated)center(191,"Guardiao do Limiar: vencido",1,UI_GREEN);
 if(index==2&&story::arconteKnown(g))center(209,"Cripta de Vaelor: concluida",1,UI_GREEN);
 button(14,224,102,"Anterior");button(124,224,102,"Proxima");
 button(14,272,102,"Voltar");button(124,272,102,"Objetivo");
}
