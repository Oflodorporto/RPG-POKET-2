#pragma once
#include "Rules.h"
namespace story {
struct Scene {const char* speaker;const char* lines[4];};
constexpr Scene opening[]={
 {"CARVALHO / AELDRA",{"A chuva cobre a estrada.","Voce empurra uma carroca quebrada.","Uma torre morta acende ao longe.","O sino toca. Ninguem esta la."}},
 {"NARA VELD",{"Entre. Ha uma cama e uma fogueira.","Quem fica na estrada depois", "desse sino acaba virando historia.","Seu nome ainda pode mudar isso."}},
 {"O MENSAGEIRO",{"Uma carta traz cinzas brancas.","O mensageiro cai junto ao fogo.","As Ruinas... estao lembrando.","O sino toca pela segunda vez."}},
 {"MAELIS VOSS / CARTA",{"Precisamos de olhos nas estradas.","Procure Elarin e Borin em Carvalho.","Algo esta expulsando os animais.","A Guilda precisa de sua ajuda."}}
};
struct Person {const char* name;const char* role;const char* lines[4];};
constexpr Person people[4][3]={
 {{"Nara Veld","Taverna e Refugio",{"Uma receita ficou sem assinatura.","Nao lembro quem me ensinou.","Descanse nas Brasas; depois", "converse com Elarin e Borin."}},
  {"Elarin Folhacinza","Boticario do bosque",{"Lobos e javalis fogem das raizes.","Veja estes veios de cinza branca.","Prepare racoes antes de partir.","Borin reconheceu esse material."}},
  {"Borin Caldaferrea","Ferreiro de Carvalho",{"Encontrei cinza em uma peca antiga.","Meus antepassados fizeram farois.","Iria pesquisa as Ruinas de Vespera.","Leve equipamento e provisoes."}}},
 {{"Iria Sorel","Arqueologa de Vespera",{"A defesa antiga ainda tem ordens.","O Guardiao protege o limiar.","Um cristal abre a Cripta.","Vaelor guarda algo alem de ouro."}},
  {"Caelen Vesper","Intendente das expedicoes",{"Toda carga chegou sob escolta.","Por isso a comida custa mais aqui.","Vendo suprimentos e avalio armas.","Nao confunda reliquia com pista."}},
  {"Arconte Vaelor","Eco do antigo mecanismo",{"Nao guardavamos um tesouro.","Guardavamos a conta.","O Livro das Vigilias revela", "por que a paz perdeu seu brilho."}}},
 {{"Sabela Marebrava","Capita da guarda do porto",{"Navios voltam sem tripulacao.","A Mao de Cinza vende cristais.","Procuro provas, nao boatos.","A costa precisa de aventureiros."}},
  {"Tomas Valevento","Mercador e cartografo",{"Mapas precisam de estradas vivas.","Ofereco suprimentos de viagem", "e armas trazidas de outros portos.","Talassar fica alem destas aguas."}},
  {"Nilsa Bronzamar","Construtora e ferreira",{"Ha pecas de farol dentro de cascos.","Alguem quer esconde-las no mar.","Traga suas armas para a forja.","Reparar tambem e resistir."}}},
 {{"Liora Valcer","Aliada em Aurora",{"Meu pai esqueceu muita coisa.","A guarda segue ordens vazias.","Precisamos entender os farois", "antes de desafiar o palacio."}},
  {"Seraphine Alvor","Guardia dos juramentos",{"Obediencia nao e virtude.","Um juramento pode ser reparado.","Conserve as pistas das outras", "cidades. Todas fazem parte disso."}},
  {"Dargan Setemartelos","Mestre da forja real",{"Minha lealdade e com o povo.","Aurora tem boas oficinas.","Prepare sua armadura; sentinelas", "nao perguntam por que voce veio."}}}
};
inline bool arconteKnown(const rpg::Game& g){return g.dungeonClears||((g.dungeonFlags&1)&&(g.dungeonEnemies&64))||(g.enemyId==8&&g.phase==rpg::Phase::Won);}
inline const char* classVoice(unsigned cls){const char* lines[]={"Voce sente ecos nos cristais.","Um juramento pode salvar vidas.","Uma estrada segura e uma vitoria.","A terra pede que voce a escute."};return lines[cls%4];}
struct Objective {const char* title;const char* place;const char* lines[4];};
inline Objective objective(const rpg::Game& g){
 if(!g.tutorial)return {"UMA CAMA E UM NOME","Carvalho / Nara Veld",{"Termine o guia de Nara.","Aprenda a lutar e preparar a bolsa.","Seu primeiro abrigo e nas Brasas.","O sino anuncia uma nova jornada."}};
 if(arconteKnown(g))return {"O LIVRO DAS VIGILIAS","Mares / Sabela Marebrava",{"O livro liga cristais a memorias.","Sabela procura provas no porto.","Converse com ela em Mares.","O proximo ato ainda esta em preparo."}};
 if(g.guardianDefeated)return {"A CONTA DOS ANTIGOS","Ruinas / Iria Sorel",{"O Guardiao deixou uma chave.","Leve um cristal a Cripta de Vaelor.","Busque selos, sobreviva aos andares.","O Arconte guarda mais que ouro."}};
 if(g.ruinsWins)return {"OS MORTOS TEM ORDENS","Ruinas de Vespera",{"Venca tres encontros nas Ruinas.","Enfrente o Guardiao do Limiar.","Iria pode explicar a antiga defesa.","Leve racoes e equipamento."}};
 return {"A FLORESTA FERIDA","Carvalho / Elarin e Borin",{"Os animais fogem de cinza branca.","Converse com Elarin e Borin.","Prepare-se e investigue Vespera.","Os farois ligam essas estradas."}};
}
inline const Person& person(unsigned city,unsigned index){return people[city%4][index%3];}
inline const char* supplier(unsigned city){const char* n[]={"Elarin Folhacinza","Caelen Vesper","Tomas Valevento","Seraphine Alvor"};return n[city%4];}
inline const char* smith(unsigned city){const char* n[]={"Borin Caldaferrea","Caelen Vesper","Nilsa Bronzamar","Dargan Setemartelos"};return n[city%4];}
inline unsigned knownChapter(const rpg::Game& g){return arconteKnown(g)?3:g.guardianDefeated||g.ruinsWins?2:g.tutorial?1:0;}
constexpr const char* chapterNames[]={"O sino em Carvalho","A floresta ferida","Os mortos tem ordens","O porto e as cinzas","Juramentos de Aurora","A primeira luz"};
constexpr Scene chapters[]={
 {"PROLOGO",{"Nara oferece abrigo nas Brasas.","Uma carta pede ajuda nas estradas.","Sua origem nao define seu destino.","Aeldra e sua primeira aventura."}},
 {"ATO I / CARVALHO",{"Elarin viu cinza nas raizes.","Borin reconhece pecas dos farois.","Os animais fogem de algo maior.","Converse com ambos e investigue."}},
 {"ATO II / VESPERA",{"Iria investiga as antigas ordens.","Venca encontros nas Ruinas.","O Guardiao protege o limiar.","Um cristal abre a Cripta."}},
 {"ATO III / MARES",{"Voce encontrou o Livro das Vigilias.","Os cristais guardam memorias.","Sabela investiga cargas no porto.","Procure-a em Mares."}},
 {"CAPITULO FUTURO",{"Aurora guarda juramentos antigos.","As pistas de Mares levam ate la.","Esta parte da campanha ainda", "aguarda sua proxima expedicao."}},
 {"CAPITULO FUTURO",{"Os quatro farois estao ligados.","A historia continuara com aliados", "e novas escolhas em Aeldra.","As outras regioes ficam alem."}}
};
struct Region {const char* name;const char* subtitle;const char* route;};
constexpr Region regions[]={
 {"Aeldra","Terras da Primeira Aurora","Mapa atual: quatro destinos."},
 {"Nivaria","Coroa do Inverno","Ao norte, alem de Kharum."},
 {"Kharum","Montanhas que Cantam","Passo de montanha em Aurora."},
 {"Silvaran","Floresta sem Fim","A leste da cordilheira."},
 {"Veldrum","Pantanos do Veu","Canais ao sul de Silvaran."},
 {"Sahram","Mar de Vidro","Caravanas ao sul de Kharum."},
 {"Ignaria","Terras da Brasa","Estrada de basalto de Sahram."},
 {"Talassar","Ilhas das Mil Mares","Navios partem de Mares."}
};
}
