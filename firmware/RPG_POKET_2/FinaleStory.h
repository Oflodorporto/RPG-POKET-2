#pragma once
namespace story {
inline const char* finaleSpeech(const rpg::Game& g,unsigned mission){
 if(mission==8)return "Liora se ajoelha diante do pai, mas nao devolve a ele a coroa. 'Eu nao vou deixar que a sua dor escolha quem mais deve sofrer.' Odran tenta responder e nao encontra o nome da filha. A guarda baixa as armas. Ele vivera para responder por suas ordens. No centro do salao, o mecanismo continua girando: a Vigilia nao era apenas a vontade de um homem. Por tras dos cristais, o Coracao do Veu desperta. Voce pode voltar para recuperar suas forcas; Liora mantera o palacio aberto aos aliados.";
 bool full=g.campaignEnding==2||(g.campaignStage==9&&g.phase==rpg::Phase::Won&&rpg::anwenReady(g));
 return full?"O Coracao do Veu se rompe, mas voce nao sustenta o selo sozinho. Elarin planta as raizes; Iria liberta os nomes; Nilsa abre o caminho; Dargan monta a estrutura que Anwen deixou. Seraphine pede juramentos que possam ser renovados ou recusados. Os cristais deixam de cobrar vidas. Algumas memorias encontram suas familias; outras se despedem. Odran perde o poder e responde por suas escolhas. Em Carvalho, Nara sobe ao campanario. O sino toca, e desta vez todos sabem quem o fez soar. 'Parece que hoje o sol veio por vontade propria.' Aeldra esta livre. A Guilda continua trabalhando, e as estradas continuam perigosas. Uma carta de Sabela fala de um marco nas ilhas de Talassar: outra historia, sem apagar esta vitoria. RPG POKET / Rodolfo. Fim da campanha de Aeldra.":"O Coracao do Veu recua. Liora e Seraphine seguram o mecanismo enquanto voce fixa o selo antigo. Aeldra tera outro amanhecer, mas o Pacto ainda nao foi reparado. 'Isto nos compra tempo', diz Liora, 'nao o direito de esquecer quem continua pagando.' Odran permanece sob custodia. Ninguem exige que voce abandone a jornada ou sacrifique outra pessoa. Volte aos aliados: o diario mostra quais ajudas faltam. Quando as quatro cidades estiverem prontas, retorne ao palacio e enfrente o Coracao de novo. Seu personagem, suas descobertas e suas entregas continuam com voce. Hoje foi uma pausa no desastre. A aurora livre ainda pode ser construida.";
}
inline unsigned finalePages(const rpg::Game& g,unsigned mission){return (wrapStory(finaleSpeech(g,mission),18,[](unsigned,const char*){})+6)/7;}
}
template<class C>void finaleFoe(C& c,unsigned id,int x,int y,unsigned frame){
 if(id==18){npcPortrait(c,story::Npc::Odran,x,y+int(frame%2),64);return;}
 int cx=x+32,cy=y+33;unsigned pulse=frame%6;uint16_t outer=0x780f,inner=0xb3ff;
 for(int yy=-30;yy<=30;++yy){int half=30-abs(yy);c.fillRect(cx-half,cy+yy,half*2+1,1,outer);if(half>8)c.fillRect(cx-half+5,cy+yy,half-3,1,inner);}
 c.fillRect(cx-2,cy-22,4,44,0xffff);c.fillRect(cx-18,cy-2,36,4,0xffff);
 for(int i=0;i<6;++i){int dx=(i%2?1:-1)*(24+int(pulse)),dy=-23+i*9;c.fillRect(cx+dx,cy+dy,3,5,0x8fff);}
}
