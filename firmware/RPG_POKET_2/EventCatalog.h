#pragma once
#include "GuildEvents.h"
#include "NpcArt.h"
inline story::Npc eventSpeaker(const rpg::Game& g){return g.eventKind==3?story::Npc::Iria:g.eventKind==4?story::Npc::Nilsa:g.eventKind==5?story::Npc::Sabela:g.eventKind==6?story::Npc::Seraphine:g.eventKind==7?story::Npc::Grum:story::Npc::Maelis;}
inline const char* eventSender(const rpg::Game& g){return g.eventKind==3?"Iria Sorel":g.eventKind==4?"Nilsa":g.eventKind==5?"Sabela":g.eventKind==6?"Seraphine":g.eventKind==7?"Grum":"Maelis Voss";}
inline const char* eventSealClue(unsigned tier,unsigned i){const char* clues[3][3]={{"Raiz sustenta a Memoria.","Memoria orienta o Caminho.","Caminho encerra o relato."},{"Memoria abre o relato.","Caminho conduz a Raiz.","Raiz encerra o relato."},{"Caminho abre o relato.","Raiz antecede Memoria.","Memoria encerra o relato."}};return clues[tier%3][i%3];}
inline const char* eventOrder(unsigned i){const char* lines[]={"Raiz > Memoria > Caminho","Memoria > Caminho > Raiz","Caminho > Raiz > Memoria"};return lines[i%3];}
inline const char* eventActionName(const rpg::Game& g){
 if(g.eventKind==3)return "Entrar na expedicao";
 if(g.eventKind==4)return g.eventProgress==0?"Buscar a lente":g.eventProgress==1?"Ajustar a lente":"Defender o marco";
 if(g.eventKind==5)return g.eventProgress==0?"Seguir os rastros":g.eventProgress==1?"Libertar viajantes":g.eventProgress==2?"Tratar os feridos":"Guiar ao abrigo";
 if(g.eventKind==6)return g.eventProgress==0?"Examinar os selos":g.eventProgress==1?"Ordenar simbolos":"Enfrentar o eco";
 return g.eventProgress==0?"Preparar o abrigo":g.eventProgress==1?"Proteger: onda 1":"Proteger: onda 2";
}
inline bool eventNeedsChoice(const rpg::Game& g){return (g.eventKind==4&&g.eventProgress==1)||(g.eventKind==5&&g.eventProgress==2)||(g.eventKind==6&&g.eventProgress<2)||(g.eventKind==7&&!g.eventProgress);}
inline const char* eventCompletion(const rpg::Game& g){const char* lines[]={"A ave voltou para o ceu.","As duas cargas foram recuperadas.","Seu passageiro chegou a salvo.","O sino calou. Os ecos descansam.","A estrada tem luz outra vez.","Os viajantes alcancaram o abrigo.","Os nomes deixaram de se repetir.","Todos atravessaram a noite vivos."};return lines[g.eventKind<8?g.eventKind:0];}
inline const char* eventBrief(const rpg::Game& g,unsigned line){
 static const char* text[5][5]={
 {"Ouvi um sino sob a terra.","Ninguem mora ali ha muitos anos.","Os ecos impedem que a gente durma.","Entre, encontre a corda do sino","e devolva o silencio aos mortos."},
 {"Um marco da estrada apagou.","Quem volta para casa perde a rota.","Ache a lente, ajuste seu encaixe","e defenda a luz enquanto acende.","Nao e um dos quatro grandes farois."},
 {"Uma caravana nao chegou ao posto.","As familias esperam sem noticias.","Siga os rastros, liberte o grupo","e cuide de quem estiver ferido.","Traga todos ao abrigo da estrada."},
 {"Tres pedras repetem nomes antigos.","Sao pessoas, nao palavras vazias.","Leia os selos e entenda sua ordem.","Um eco corrompeu essas lembrancas.","Liberte os nomes sem apaga-los."},
 {"Viajantes pediram abrigo a Grum.","Ha feras rondando a fogueira.","Prepare uma defesa e proteja todos","por duas ondas. Em agradecimento,","dividiremos uma racao com voce."}};
 if(line>=5)return "";
 if(g.eventKind==3&&line==4)return g.eventTier?"Duas galerias / passagem custeada.":"Duas salas / entrada custeada.";
 return text[g.eventKind>=3&&g.eventKind<8?g.eventKind-3:0][line];
}
