#pragma once
#include "Narrative.h"
namespace story {
// Continuous speech, selected only from facts already recorded for this hero.
inline const char* conversation(const rpg::Game& g,unsigned person){
 person%=3;
 if(g.campaignEnding==2){
  constexpr const char* replies[4][3]={
   {"Desta vez fui eu que toquei o sino. A sopa continua no fogo. Sente um pouco.",
    "As raizes voltaram a crescer. Ainda ha muito de que cuidar, mas ja nao precisamos arrancar lembrancas para manter esta terra viva.",
    "Dargan fez um bom trabalho. O mecanismo sustenta o selo sem prender ninguem la dentro. Era assim que devia ter sido construido."},
   {"Os nomes chegaram as familias. Algumas memorias voltaram; outras puderam se despedir. Quero que Vespera seja lembrada tambem por isso.",
    "A expedicao vai precisar de provisoes. As ruinas continuam perigosas. Ao menos agora sabemos por que entramos la.",
    "O selo permanece. Os nomes estao livres. Voces encontraram o caminho que nos nao tivemos coragem de seguir."},
   {"Os voluntarios voltaram e o porto recebeu suas familias. O Caminho esta aberto. Agora podemos cuidar de quem chega.",
    "As rotas dos farois continuam abertas. Ao menos os caminhos de Aeldra voltaram a responder.",
    "Minha equipe voltou inteira. Os farois estao de pe. Vou cuidar para que o trabalho dure."},
   {"Meu pai vai responder pelas ordens que deu. Eu vou ajudar a cuidar do que reconstruimos. Ainda ha muito a fazer.",
    "Os juramentos agora podem ser renovados ou recusados. Precisamos continuar ouvindo quem os faz.",
    "Pode conferir a estrutura. Nenhuma cela. Nenhum lugar para aprisionar uma lembranca. Agora comeca o trabalho de mante-la."}
  };
  return replies[g.city%4][person];
 }
 const bool archives=(g.campaignFlags&7)==7,odranDown=(g.campaignFlags&128)!=0;
 if(g.city==3&&person==0&&odranDown){
  if(g.campaignEnding==1)return rpg::anwenReady(g)?
   "Odran continua sob custodia. O selo ainda gasta as lembrancas aprisionadas, e nao recolheremos outras. Agora as quatro ajudas estao prontas. Volte comigo ao Coracao do Veu; os aliados podem sustentar os farois enquanto terminamos a restauracao.":
   "Odran esta sob custodia. Fechamos a fenda por enquanto, mas o mecanismo ainda gasta as lembrancas aprisionadas. Nao recolheremos outras. Precisamos terminar a restauracao. O diario mostra quais ajudas faltam. Quando as quatro estiverem prontas, volte a mim para enfrentarmos o Coracao do Veu.";
  return rpg::anwenReady(g)?
   "Meu pai perdeu o poder e esta sob custodia. O Coracao do Veu ainda mantem a fenda aberta. E a manifestacao dela aqui; vence-lo nao destroi todo o Veu Oco. As quatro ajudas estao prontas. Recupere suas forcas. Vou com voce, e os aliados sustentarao os farois enquanto enfrentamos o Coracao.":
   "Meu pai perdeu o poder e esta sob custodia. O Coracao do Veu ainda mantem a fenda aberta. E a manifestacao dela aqui; vence-lo nao destroi todo o Veu Oco. Recupere suas forcas. Com as quatro ajudas, poderemos restaurar os farois. Sem elas, so conseguiremos fechar a fenda por enquanto.";
 }
 // Deposition never hides an ally's outstanding request or a proven contribution.
 if(archives&&person==rpg::campaignPerson(4+g.city)){
  if(rpg::contribution(g,g.city))return contributionSpeech(4+g.city);
  if(g.city==0)return "O projeto de Anwen precisa de raizes vivas. Ainda ha algumas sob a cinza. Traga tres racoes para alimentar os voluntarios enquanto cuidam das mudas. Eu preparo as raizes; Nara oferece abrigo, e Borin leva as ferramentas. Assim Carvalho pode ajudar na restauracao.";
  if(g.city==1)return "Os nomes das vitimas ainda estao presos aos registros. Preciso abrir uma sala antiga; um espectro guarda a porta. Venha comigo e mantenha-o longe enquanto liberto as memorias. Algumas poderao voltar as familias. Outras precisam de uma despedida. Vou conferir cada nome.";
  if(g.city==2)return "A equipe esta pronta: construtores e alguns dos refugiados que voce salvou. Todos escolheram ajudar nos farois. A Mao de Cinza bloqueou o cais. Escolte-nos ate a rota segura; Sabela fica cuidando do porto. Minhas ferramentas ja estao na carroca.";
  return "Guardei o desenho de Anwen entre minhas ferramentas. Agora podemos construir a estrutura que ele mostra, sem celas para prisioneiros. Seraphine reunira os juramentos voluntarios. Sao cento e cinquenta moedas para os componentes. O trabalho fica por conta da minha oficina.";
 }
 if(rpg::anwenReady(g)&&g.city==3&&person==0)return "As quatro ajudas estao prontas. Anwen encontrou um modo de alimentar os farois com terra viva e juramentos livres. Voce reuniu quem pode reconstrui-los. Primeiro impediremos Odran de iniciar a Vigilia. Depois enfrentaremos o Coracao do Veu, a manifestacao da fenda. Os aliados sustentarao o selo enquanto voce luta.";

 if(g.city==0){
  if(person==0){
   if(odranDown)return rpg::anwenReady(g)?"Sente e coma um pouco. Odran ja esta sob custodia, e os aliados estao prontos. Liora espera por voce para terminar a restauracao. Quero que chegue la com forcas. Sua cama continua aqui.":"Tire essa capa e sente perto do fogo. Odran esta sob custodia, mas voce ainda tem trabalho com os farois. Descanse antes de buscar as ajudas que faltam. Elarin cuida das mudas; eu cuido de quem vem ajudar.";
   if(archives)return "Sente um pouco. Voce trouxe a verdade de Aurora, mas ainda precisa comer. Nao vou deixar que enfrente Odran de barriga vazia. Elarin prepara a ajuda de Carvalho. Quando tudo acabar, quero ouvir sua historia aqui, ao lado do fogo.";
   if((g.campaignFlags&3)==3)return "Soube que os refugiados de Mares estao abrigados. Voce pode descansar aqui tambem. Os registros seguem para Liora, em Aurora; leve uma refeicao quente antes de pegar a estrada. Quero ouvir o que ela descobrir.";
   if(g.campaignFlags&1)return "Entao havia lembrancas naquelas cargas... Agora entendo as receitas que ficaram sem dono. Sabela ainda precisa de ajuda no resgate. Coma antes de voltar ao porto; eu mantenho uma cama pronta para quando voce retornar.";
   if(arconteKnown(g))return "Achei que estava ficando velha quando esqueci quem me ensinou esta receita. Agora voce diz que os cristais levavam nossas lembrancas... Procure Sabela em Mares. E volte para me contar o que descobrir; sua cama continua aqui.";
   return "Entre, a sopa ainda esta quente. Elarin achou cinza nas raizes, e Borin encontrou a mesma coisa numa peca de farol. Se quiser ouvir os dois, estao por perto. Iria estuda essas pecas em Vespera. Prepare sua bolsa para seguir a pista. Nos ficamos cuidando da vila; quero ver voce voltar inteiro.";
  }
  if(person==1){
   if((g.campaignFlags&3)==3)return "As mudas ainda resistem. Os refugiados de Mares ja tem abrigo, e os registros seguem para Liora. Vou cuidar do bosque enquanto voce busca respostas em Aurora. Uma raiz precisa de tempo; as pessoas daqui, de comida todos os dias.";
   if(g.campaignFlags&1)return "A carga confirmou o que os cristais levavam. Ainda ha cinza nas raizes, mas vou manter estas mudas vivas. Sabela precisa de voce no resgate de Mares. Nos cuidamos de quem ficou em Carvalho.";
   if(arconteKnown(g))return "O Livro explica a cinza. Isso nao devolve a primavera de uma vez, mas temos por onde comecar. Vou cuidar das mudas enquanto voce segue a pista dos cristais em Mares. Leve racoes; ate quem sabe lutar precisa comer.";
   return "Veja esta raiz: por dentro esta fria e coberta de cinza. Os animais fogem do bosque procurando comida. Borin achou a mesma cinza numa peca de farol. No marco do bosque, ha um nome raspado. Iria pode comparar esses vestigios em Vespera. Eu fico cuidando das plantas e das familias daqui.";
  }
  if(odranDown)return "Odran caiu; o mecanismo ainda precisa de reparo. Dargan cuida da estrutura, e Elarin prepara as raizes. Minha forja continua acesa para manter as ferramentas prontas. Desta vez, cada peca vai servir a quem sustenta os farois por vontade propria.";
  if(archives)return "Agora sabemos para que serviam estas pecas. Dargan guarda o projeto de Anwen; Elarin prepara as raizes de Carvalho. Vou cuidar das ferramentas da restauracao. Ja me orgulhei dos farois antigos. Prefiro ajudar a corrigir o que construiram.";
  if((g.campaignFlags&3)==3)return "Voce abriu o caminho para os refugiados, e Nilsa fez o farol responder. Bom trabalho de reparo. Os registros levam a Aurora; Liora precisa le-los. Eu mantenho sua arma em ordem enquanto voce busca quem deu aquelas ordens.";
  if(g.campaignFlags&1)return "A carga de Mares confirmou o roubo. Que trabalho vergonhoso fizeram com estas pecas. Sabela precisa abrir caminho para o resgate; leve sua arma ate a forja antes de voltar. As ferramentas de Carvalho ficam por minha conta.";
  if(arconteKnown(g))return "Olhe o que fizeram com estas pecas. Passei a vida chamando isso de uma grande obra. O Livro mostra o preco que esconderam. Leve as provas a Sabela, em Mares. Eu mantenho a forja acesa para ajudar a consertar o que deixaram.";
  return "Esta cinza tambem estava numa peca de farol. Meus antepassados fizeram o mecanismo, mas os desenhos chegaram incompletos. Ha uma figura sem nome entre os construtores. Iria pesquisa as Ruinas de Vespera; pode reconhecer a peca e a marca. Eu cuido das ferramentas da vila. Prepare sua arma antes de seguir o rastro.";
 }
 if(g.city==1){
  if(person==0){
   if((g.campaignFlags&3)==3)return "Os registros de Mares seguem com o Livro para Aurora. Liora precisa conferir as ordens nos arquivos. A fenda sob Vespera tem cento e vinte anos; Odran nao a criou. Agora precisamos saber o que ele escolheu fazer com o mecanismo que herdou.";
   if(g.campaignFlags&1)return "A carga confirmou o roubo de memorias e o envolvimento da Coroa. Guarde esses registros. Sabela precisa abrir passagem para os refugiados antes de continuarmos a investigacao. Eu sigo conferindo os nomes que restaram em Vespera.";
   if(arconteKnown(g))return "O Livro conta que a fenda se abriu sob Vespera ha cento e vinte anos. Dela veio o Veu Oco: mortos retornam incompletos, vivos perdem nomes e vinculos. Os quatro farois fecharam a fenda, mas seus cristais consomem memorias para manter o selo. Sabela investiga cargas desses cristais em Mares. Leve a verdade a ela.";
   if(g.crystals)return g.guardianDefeated?"O Guardiao era a defesa externa, nao o Arconte. Voce ja tem um cristal para abrir a cripta. La dentro, procure os selos para avancar pelos dois andares. Precisamos descobrir por que essas defesas continuam lutando.":"Voce ja tem um cristal para abrir a cripta. O Guardiao protege o limiar, mas nao e preciso derrota-lo para usar essa chave. O Arconte esta alem dos selos. Investigue os dois andares; precisamos saber por que as antigas defesas continuam lutando.";
   if(g.guardianDefeated)return "O Guardiao que voce venceu era a defesa externa, nao o Arconte. Para entrar na cripta, leve um cristal. Ha outros a venda nas Ruinas por 300 moedas. La dentro, procure os selos dos dois andares. Ainda precisamos entender o que essa defesa protege.";
   return g.ruinsWins>=3?"Voce ja pode enfrentar o Guardiao do Limiar. Ele carrega um cristal que abre a cripta. Tambem ha cristais a venda nas Ruinas por 300 moedas. O Guardiao e a defesa externa; o Arconte esta la dentro. Prepare suas provisoes antes de entrar.":"Cada pedra de Vespera pertenceu a uma casa. Os mortos ainda repetem ordens antigas. Tres vitorias nas Ruinas permitem enfrentar o Guardiao do Limiar. Ele carrega um cristal; tambem ha cristais a venda aqui por 300 moedas. Um cristal abre a cripta. Leve provisoes.";
  }
  if(person==1)return "A escolta encareceu esta carga. Aqui, ate a comida precisa chegar protegida. Vendo provisoes e avalio armas recuperadas. Confira o que vai levar: la dentro, nenhuma caravana pode reabastecer sua bolsa.";
  if(arconteKnown(g))return "Eu ajudei a selar a fenda. E escondi o preco. Os cristais gastavam um pouco de cada lembranca, noite apos noite. A Raiz ligava a terra; a Memoria guardava os nomes; o Caminho unia as rotas; o Juramento sustentava o conjunto. Chamamos isso de Pacto da Primeira Aurora. Houve quem recusasse o sacrificio. Seu nome foi apagado. Quebrar os farois libertaria o Veu Oco. Precisamos mudar o que alimenta o selo.";
  return g.crystals?"Uma voz chega de alem dos selos. As palavras se perdem entre as pedras. Iria talvez saiba como alcancar quem esta la. Voce ja tem um cristal para abrir a cripta.":"Uma voz chega de alem dos selos. As palavras se perdem entre as pedras. Iria talvez saiba como alcancar quem esta la. Voce precisa de um cristal para entrar. O Guardiao carrega um; tambem ha cristais a venda nas Ruinas por 300 moedas.";
 }
 if(g.city==2){
  if(person==0){
   if(odranDown)return "Odran esta sob custodia. Bom. Agora precisamos terminar o trabalho nos farois. O porto continua recebendo refugiados, e minha guarda protege as oficinas. Nilsa cuida da equipe de restauracao. Quem chega com fome nao vai esperar uma cerimonia para comer.";
   if(archives)return "Liora encontrou as ordens. Agora temos provas de quem mandava as cargas. O porto continua aberto aos refugiados, e Nilsa prepara ajuda para os farois. Leve isso a Aurora: as pessoas que voce salvou querem participar do que vem depois.";
   if((g.campaignFlags&3)==3)return "Os refugiados estao nas oficinas, aquecidos e alimentados. Nilsa religou o Farol do Caminho. A placa trouxe um nome que faltava aos registros: Anwen. Maelis abriu os livros da Guilda. Leve essas pistas, junto com o Livro, a Liora em Aurora. Ela precisa conferir as ordens da Coroa.";
   if(g.campaignFlags&1)return "As caixas traziam nomes riscados, e os registros ligam a carga a Coroa. Maelis admitiu que financiou aquelas rotas sem perguntar o que levavam. Agora o farol apagou. Um barco de refugiados bateu no cais, e a Mao de Cinza bloqueia a passagem. Abra caminho; minha guarda fara o resgate.";
   return arconteKnown(g)?"Iria avisou que voce traria o Livro. Aqui os navios voltam sem tripulacao, e a Mao de Cinza leva cristais pelas cisternas. Preciso de provas contra quem paga por isso. Abra passagem. Minha guarda recolhe as caixas e procura os registros.":"Ha navios voltando sem tripulacao e familias esperando no cais. A Mao de Cinza negocia cristais; ainda procuro provas do que fazem. Iria investiga as Ruinas. Se descobrir o que esses cristais guardam, traga a verdade ao porto. Minha guarda fica com quem esta esperando.";
  }
  if(person==1)return g.campaignFlags&1?"Eu mesmo levei cargas da Coroa. Aceitei dinheiro sem perguntar o que havia dentro. Maelis admitiu a parte da Guilda; eu entreguei minhas rotas a Sabela. Agora os registros podem ser conferidos. Ainda vendo mapas e provisoes. Desta vez quero saber quem vai chegar ao outro lado.":"Tenho mapas enrolados ate debaixo do balcao, mas as mares andam mudando as rotas. Alguns mercadores aceitam cargas suspeitas para pagar a viagem. Sabela investiga os desaparecimentos. Se precisar ir ate ela, posso preparar suas provisoes.";
  return (g.campaignFlags&2)?"Aquelas pecas escondidas nos cascos eram do farol. Enquanto voce abria passagem, consegui reconectar a rota segura. Os refugiados estao nas minhas oficinas. Agora os registros seguem para Aurora. Vou conferir os encaixes outra vez antes de deixar alguem sair para o mar.":"Conheco um casco por dentro. Estas pecas nao pertencem a um navio: sao do farol, escondidas em madeira nova. Sabela esta seguindo a carga. Traga sua arma se precisar de reparo; nao vou deixar voce enfrentar essa gente com o cabo rachado.";
 }
 if(person==0){
  if(archives)return "Meu pai escreveu uma ordem para me proteger, porque ja nao lembrava meu rosto. Eu queria acreditar que isso explicava tudo. Mas foi ele quem decidiu cobrar as lembrancas de outras familias. Voce abriu os arquivos. Vou ficar ao seu lado para impedir a Vigilia. Seraphine e Dargan conhecem o projeto de Anwen; precisamos reunir as ajudas das quatro cidades.";
  if((g.campaignFlags&3)==3)return "Sabela enviou noticias do resgate, e recebi os registros de Maelis. Obrigada por trazer o Livro. Os registros apontam para meu pai. Preciso ver as ordens que ele assinou. Uma sentinela guarda os arquivos. Abra caminho; Seraphine, Dargan e eu recolheremos os documentos.";
  return g.campaignFlags&1?"Os registros da carga chegaram ao porto, mas Sabela ainda precisa salvar quem esta preso no cais. Ajude-a no resgate. Depois, traga as provas a Aurora. Preciso saber o que meu pai autorizou antes de confronta-lo.":"Meu pai perdeu muitas lembrancas. A guarda segue ordens que ninguem explica, e eu preciso saber quais decisoes ele ainda toma por vontade propria. Sabela investiga cargas da Coroa em Mares. Ajude-a e traga os registros. Ainda tenho perguntas demais sobre o homem que me criou.";
 }
 if(person==1){
  if(odranDown)return rpg::anwenReady(g)?"Odran esta sob custodia, mas o mecanismo ainda ameaca nossas lembrancas. As quatro ajudas estao prontas. Liora espera por voce para enfrentarmos o Coracao e terminarmos a restauracao. Vou reunir quem escolheu renovar os juramentos.":"Odran esta sob custodia, mas o mecanismo ainda ameaca nossas lembrancas. Precisamos terminar a restauracao. Cada cidade prepara sua ajuda; o diario mostra quais faltam. Nao recolheremos novas lembrancas. Vou cuidar dos juramentos de quem escolheu ajudar.";
  if(archives)return "Li a ordem da Vigilia. Odran escolheu um amanhecer que consumiria as lembrancas de todos. Anwen tentou outro caminho: terra viva, vinculos voluntarios e juramentos renovados. Dargan protege seu desenho. Precisamos das quatro cidades para realiza-lo, e cada pessoa deve poder escolher se participa.";
  if((g.campaignFlags&3)==3)return "Liora recebeu os registros de Mares. Agora precisamos ler as ordens guardadas nos arquivos. Vou com ela. Um juramento de protecao exige que eu olhe para as pessoas atingidas, mesmo quando foi a Coroa que assinou a ordem.";
  return "Obedecer a uma ordem cruel nao torna um juramento mais sagrado. Liora quer saber o que aconteceu com o pai e com os farois. Quando as provas de Mares chegarem, vou ajuda-la a ler os registros. O templo tambem precisa responder por aquilo que aceitou.";
 }
 // After the archives, Dargan's active speech is the request/receipt above.
 return (g.campaignFlags&3)==3?"As provas de Mares chegaram. Falta abrir os arquivos e conferir quem assinou as ordens. Liora vai precisar de voce contra a sentinela. Traga sua armadura; quero ver as fivelas antes que entre no palacio.":"A Coroa compra cristais a qualquer custo. Nao gosto de perguntas que precisam ser feitas em voz baixa. Liora procura provas; os registros de Mares podem ajuda-la a entrar nos arquivos. Enquanto isso, posso preparar seu equipamento. Sentinelas nao esperam a gente terminar um reparo.";
}
inline unsigned conversationPages(const char* text){return std::max(1u,(wrapStory(text,18,[](unsigned,const char*){})+6)/7);}
}
