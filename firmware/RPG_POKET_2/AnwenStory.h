#pragma once
namespace story {
inline const char* contributionBrief(unsigned mission){
 switch(mission){
 case 4:return "Entregue 3 racoes para os voluntarios. Elarin cuidara das mudas do Farol da Raiz.";
 case 5:return "Proteja Iria de um espectro. Ela libertara as memorias presas aos registros.";
 case 6:return "Escolte a equipe de Nilsa. Derrote o saqueador que bloqueia o caminho dos farois.";
 default:return "Entregue 150 ouro a Dargan. Ele montara a estrutura de Anwen, sem celas.";
 }
}
inline const char* contributionSpeech(unsigned mission){
 switch(mission){
 case 4:return "Com essas provisoes, os voluntarios podem ficar cuidando das mudas. Ainda ha raizes vivas sob a cinza; vamos prepara-las para os farois. Nara oferece o abrigo, e Borin leva as ferramentas. O projeto de Anwen conta com terra viva. Agora Carvalho pode oferecer sua parte.";
 case 5:return "Consegui separar os nomes das ordens enquanto voce mantinha o espectro longe. Vou conferir os registros com as familias. Algumas memorias poderao voltar; outras, se despedir. Levarei as copias e a chave do mecanismo. A ajuda de Vespera esta pronta.";
 case 6:return "Todos chegaram. Conferi as ferramentas e os nomes da equipe. Alguns fugiram de Mares uma vez e escolheram voltar para ajudar. Vou trabalhar nos farois; Sabela e Tomas cuidam da rota. Temos gente para a obra e um caminho seguro ate ela.";
 default:return "Os componentes estao pagos. Minha oficina ja montou a estrutura de Anwen: pode conferir, nenhuma cela. Os aprendizes cuidam dos encaixes, e Seraphine reunira juramentos que possam ser renovados ou recusados. Guardei este desenho por medo durante anos. Agora vamos usa-lo.";
 }
}
inline unsigned contributionPages(unsigned mission){return std::max(1u,(wrapStory(contributionSpeech(mission),18,[](unsigned,const char*){})+6)/7);}
inline const char* contributionContact(unsigned mission){return mission==4?"Elarin Folhacinza":mission==5?"Iria Sorel":mission==6?"Nilsa Bronzamar":"Dargan Setemartelos";}
}
