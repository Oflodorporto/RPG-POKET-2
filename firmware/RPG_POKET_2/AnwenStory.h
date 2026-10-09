#pragma once
namespace story {
inline const char* contributionBrief(unsigned mission){
 switch(mission){
 case 4:return "Entregue 3 racoes. Elarin cuidara das mudas e dos voluntarios do Farol da Raiz.";
 case 5:return "Proteja Iria de um espectro. Ela libertara as memorias presas aos registros.";
 case 6:return "Escolte a equipe de Nilsa. Derrote o saqueador que bloqueia o caminho dos farois.";
 default:return "Entregue 150 ouro a Dargan. Ele montara o novo farol, sem celas para prisioneiros.";
 }
}
inline const char* contributionSpeech(unsigned mission){
 switch(mission){
 case 4:return "Estas provisoes vao alimentar quem fica para cuidar das mudas, nao comprar o silencio de ninguem. A cinza ainda esta aqui, mas ha raizes vivas por baixo dela. Vou leva-las com Nara e Borin quando chegar a hora. Anwen entendeu uma coisa que os reis esqueceram: cuidar de algo todos os dias tambem e uma forma de poder. Carvalho esta com voce.";
 case 5:return "Enquanto voce mantinha o espectro longe, consegui separar os nomes das ordens. Estas memorias nao nos pertencem. Vamos devolve-las as familias ou permitir que se despecam. O Livro nos mostrou o crime; estes registros mostram quem sofreu com ele. Eu levarei as copias e a chave do mecanismo. Vespera nao precisa continuar sendo apenas um lugar onde as pessoas perderam tudo.";
 case 6:return "Todos chegaram. Alguns ja tinham fugido uma vez e mesmo assim escolheram voltar para ajudar. Ninguem foi obrigado a assinar um juramento. Vou manter a rota aberta com Sabela e Tomas, e levar minha equipe ate os farois. O Caminho nao deve servir para esconder cargas de cristal. Deve levar pessoas em seguranca para casa. Mares esta pronta.";
 default:return "Aqui esta a estrutura desenhada por Anwen. Veja: nao ha celas, nem lugar para prender uma lembranca. Seus componentes ja estao pagos; minha oficina e os meus aprendizes completam o trabalho. Seraphine reunira juramentos que possam ser renovados ou recusados. Guardei este projeto por medo durante anos. Agora ele pode sair da oficina e servir as pessoas que deveriam ter sido protegidas desde o inicio.";
 }
}
inline unsigned contributionPages(unsigned mission){return std::max(1u,(wrapStory(contributionSpeech(mission),18,[](unsigned,const char*){})+6)/7);}
inline const char* contributionContact(unsigned mission){return mission==4?"Elarin Folhacinza":mission==5?"Iria Sorel":mission==6?"Nilsa Bronzamar":"Dargan Setemartelos";}
}
