#pragma once
#include "Campaign.h"
#include "OriginStory.h"
#include "AnwenStory.h"
#include "FinaleStory.h"
namespace story {
struct Scene {const char* speaker;const char* lines[4];};
constexpr Scene portScenes[2][4]={
 {{"SABELA MAREBRAVA",{"A guarda recolhe os cristais.","Cada caixa traz um nome riscado.","O porto virou um mercado", "de lembrancas de gente com fome."}},
  {"TOMAS VALEVENTO",{"Reconheco o selo da Coroa.","Aceitei ouro sem fazer perguntas.","Conheco cada curva das cisternas.","Vou abrir meus registros a Sabela."}},
  {"MAELIS VOSS / CARTA",{"Autorizei as cargas da Guilda.","Aceitei ouro e ordens da Coroa.","Vou responder junto com a Guilda.","Envio meus registros como prova."}},
  {"SABELA / NOVA URGENCIA",{"O Farol do Caminho se apagou.","Um barco de refugiados bateu aqui.","A Mao de Cinza bloqueou o cais.","Vamos tirar aquelas pessoas dali."}}},
 {{"SABELA MAREBRAVA",{"Voce abriu passagem para a guarda.","Os refugiados chegam as oficinas.","Nilsa traz mantas e agua quente.","Hoje salvamos as pessoas do cais."}},
  {"NILSA BRONZAMAR",{"Havia pecas de farol nos cascos.","A forja real alterou o farol.","Com elas, reconectamos a rota.","O Caminho responde ao reparo."}},
  {"SABELA / A QUINTA TESTEMUNHA",{"A placa tem o nome apagado: Anwen.","Ela recusou sacrificar memorias.","Os registros seguem para Aurora.","Liora precisa ver estas provas."}},
  {"MAELIS VOSS / CARTA",{"Carvalho abrigara os refugiados.","Abrirei as contas da Guilda.","Leve Livro e registros a Liora.","Minha assinatura esta nas cargas."}}}
};
constexpr Scene auroraScenes[]={
 {"LIORA VALCER",{"A sentinela recua. Arquivo aberto.","As ordens tem o selo do meu pai.","Nao sao erros de seus soldados.","Ele mandou recolher memorias."}},
 {"SERAPHINE ALVOR",{"A ordem e a Vigilia Perpetua.","Todos pagariam com lembrancas.","Um juramento que exige isso", "deixou de proteger o reino."}},
 {"DARGAN SETEMARTELOS",{"Guardei este desenho de Anwen.","Raizes vivas alimentarao o selo.","Os vinculos serao voluntarios.","Minha oficina fara a estrutura."}},
 {"ODRAN VALCER / REGISTRO",{"Esqueci o rosto de minha filha.","Ofereci minhas memorias ao selo.","Ordeno que recolham as dos outros.","Nao perdi tudo isso em vao."}},
 {"LIORA VALCER",{"Ele ordenou que me protegessem.","Quer sacrificar outras familias.","A dor dele nao justifica isso.","Vamos impedir a Vigilia, juntos."}},
 {"SERAPHINE / PROXIMO PASSO",{"O Livro e as provas estao aqui.","Dargan guarda o projeto de Anwen.","Precisamos das quatro cidades.","Juramentos poderao ser renovados."}}
};
inline const Scene& campaignScene(unsigned mission,unsigned index){return mission==3?auroraScenes[std::min(5u,index)]:portScenes[mission==2?1:0][std::min(3u,index)];}
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
inline bool arconteKnown(const rpg::Game& g){return rpg::campaignMemory(g);}
inline const char* classVoice(unsigned cls){const char* lines[]={"Voce sente ecos nos cristais.","Um juramento pode salvar vidas.","Uma estrada segura e uma vitoria.","A terra pede que voce a escute."};return lines[cls%4];}
struct Objective {const char* title;const char* place;const char* lines[4];};
inline Objective objective(const rpg::Game& g){
 if(!g.tutorial)return {"UMA CAMA E UM NOME","Carvalho / Nara Veld",{"Termine o guia de Nara.","Aprenda a lutar e preparar a bolsa.","Elarin e Borin podem orientar voce.","Siga a cinza das raizes e dos farois."}};
 if(g.campaignEnding==2)return {"AURORA LIVRE","Aeldra / campanha concluida",{"Os farois nao exigem novas vitimas.","As cidades reconhecem sua ajuda.","Guilda e exploracao continuam.","Releia o epilogo no objetivo."}};
 if(g.campaignFlags&128){
  if(rpg::anwenReady(g))return {"RESTAURAR OS FAROIS","Aurora / Liora Valcer",{"Odran esta sob custodia.","As quatro ajudas estao prontas.","Volte a Liora e enfrente o Coracao.","A restauracao libertara as memorias."}};
  unsigned city=rpg::contributionNextCity(g);const char* places[]={"Carvalho / Elarin Folhacinza","Vespera / Iria Sorel","Mares / Nilsa Bronzamar","Aurora / Dargan Setemartelos"};
  return {g.campaignEnding==1?"O SELO TEMPORARIO":"O CORACAO DO VEU",places[city],{"Odran esta sob custodia.","O selo ainda usa memorias presas.","Ninguem recolhera novas lembrancas.","Reuna as ajudas ainda pendentes."}};
 }
 if(rpg::anwenReady(g))return {"O LEGADO DE ANWEN","Aurora / Liora Valcer",{"As quatro ajudas estao prontas.","O projeto sem vitimas esta completo.","Liora recebe a equipe de restauracao.","Converse com ela no palacio."}};
 if(g.campaignFlags&4){unsigned city=rpg::contributionNextCity(g);const char* places[]={"Carvalho / Elarin Folhacinza","Vespera / Iria Sorel","Mares / Nilsa Bronzamar","Aurora / Dargan Setemartelos"};return {"O LEGADO DE ANWEN",places[city],{"Reuna as quatro ajudas para os farois.","Cada cidade oferece uma contribuicao.","O diario registra as entregas reais.","Nenhuma delas exige outra vitima."}};}

 if((g.campaignFlags&3)==3)return {"JURAMENTOS DE AURORA","Aurora / Liora Valcer",{"Voce salvou os refugiados de Mares.","Maelis abriu os registros da Guilda.","Leve a verdade a Liora em Aurora.","Liora pede acesso aos arquivos reais."}};
 if(g.campaignFlags&1)return {"O FAROL APAGADO","Mares / Sabela Marebrava",{"A carga prova o roubo de memorias.","A tempestade apagou o Farol Caminho.","Refugiados estao presos no cais.","Sabela precisa de voce no resgate."}};
 if(arconteKnown(g))return {"O LIVRO DAS VIGILIAS","Mares / Sabela Marebrava",{"O livro liga cristais a memorias.","Sabela procura provas no porto.","Converse com ela em Mares.","Sabela oferece uma missao no porto."}};
 if(g.crystals)return {"A CONTA DOS ANTIGOS","Ruinas / entrada da Cripta",{"Seu cristal pode abrir a Cripta.","Iria investiga os selos em Vespera.","Explore os andares alem da entrada.","Busque a origem das vozes nas pedras."}};
 if(g.guardianDefeated)return {"A CONTA DOS ANTIGOS","Ruinas / Iria Sorel",{"A defesa do Guardiao foi vencida.","Um cristal abre a entrada da Cripta.","Ha cristais a venda nas Ruinas.","Iria pode explicar os selos antigos."}};
 if(g.ruinsWins)return {"OS MORTOS TEM ORDENS","Ruinas de Vespera",{"Um cristal abre a entrada da Cripta.","O Guardiao carrega um; ha a venda.","Tres vitorias revelam o Guardiao.","Iria pode explicar a antiga defesa."}};
 return {"A FLORESTA FERIDA","Vespera / Iria Sorel",{"Raizes e farois tem a mesma cinza.","Iria estuda os farois em Vespera.","Elarin e Borin cuidam de Carvalho.","Prepare a bolsa e siga esse rastro."}};
}
inline const Person& person(unsigned city,unsigned index){return people[city%4][index%3];}
inline const char* supplier(unsigned city){const char* n[]={"Elarin Folhacinza","Caelen Vesper","Tomas Valevento","Seraphine Alvor"};return n[city%4];}
inline const char* smith(unsigned city){const char* n[]={"Borin Caldaferrea","Caelen Vesper","Nilsa Bronzamar","Dargan Setemartelos"};return n[city%4];}
inline unsigned knownChapter(const rpg::Game& g){return g.campaignEnding?5:(g.campaignFlags&3)==3?4:arconteKnown(g)?3:g.guardianDefeated||g.ruinsWins?2:g.tutorial?1:0;}
constexpr const char* chapterNames[]={"O sino em Carvalho","A floresta ferida","Os mortos tem ordens","O porto e as cinzas","Juramentos de Aurora","A primeira luz"};
constexpr Scene chapters[]={
 {"PROLOGO",{"Nara oferece abrigo nas Brasas.","Uma carta pede ajuda nas estradas.","Cinza branca tomou as raizes.","Elarin e Borin investigam a origem."}},
 {"ATO I / CARVALHO",{"Raizes e farois tem a mesma cinza.","Uma placa tem um nome raspado.","Elarin e Borin cuidam de Carvalho.","Iria estuda os farois em Vespera."}},
 {"ATO II / VESPERA",{"Iria investiga as antigas ordens.","O Guardiao protege o limiar.","Um cristal abre a Cripta.","Ele tambem e vendido nas Ruinas."}},
 {"ATO III / MARES",{"O Livro revela o custo dos farois.","Cristais consomem memorias presas.","Uma testemunha recusou o sacrificio.","Sabela investiga as cargas em Mares."}},
 {"ATO IV / AURORA",{"O porto foi salvo; o farol respondeu.","Maelis assumiu o erro da Guilda.","Anwen e a testemunha apagada.","Liora recebera os registros em Aurora."}},
 {"DESFECHO DE AELDRA",{"Odran responde por suas ordens.","Os aliados sustentam os farois.","Aeldra pode escolher seu amanha.","A jornada continua nas estradas."}}
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
