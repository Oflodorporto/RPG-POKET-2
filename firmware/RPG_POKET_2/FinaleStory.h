#pragma once
namespace story {
inline bool finaleFull(const rpg::Game& g,unsigned mission){return mission!=8&&(g.campaignEnding==2||(g.campaignStage==9&&g.phase==rpg::Phase::Won&&rpg::anwenReady(g)));}
inline const char* finaleSpeech(const rpg::Game& g,unsigned mission){
 if(mission==8)return "Liora se ajoelha diante do pai. 'Eu nao vou deixar que a sua dor escolha quem mais deve sofrer.' Odran tenta responder e nao encontra o nome da filha. A guarda baixa as armas. Ele fica sob custodia e respondera por suas ordens. No centro do salao, o mecanismo continua girando. Por tras dos cristais, o Coracao do Veu desperta. Liora mantera o palacio aberto aos aliados enquanto voce recupera suas forcas.";
 if(finaleFull(g,mission))return "O Coracao do Veu se rompe. A fenda de Aeldra volta a ser selada pelo trabalho de todos. Elarin planta as raizes; Iria liberta os nomes; Nilsa abre o caminho; Dargan monta a estrutura de Anwen. Seraphine pede juramentos que possam ser renovados ou recusados. Os cristais deixam de consumir lembrancas. Algumas memorias encontram suas familias; outras se despedem. Odran permanece sob custodia e responde por suas escolhas. Em Carvalho, Nara sobe ao campanario. O sino toca, e todos sabem quem o fez soar. 'Parece que hoje o sol veio por vontade propria.' Aeldra esta livre. A Guilda continua trabalhando, e as estradas continuam perigosas.";
 if(g.campaignEnding==1&&rpg::anwenReady(g))return "Odran continua sob custodia. O selo temporario ainda depende das memorias aprisionadas; ninguem recolhera novas lembrancas para alimenta-lo. O Pacto ainda precisa ser reparado. Agora as quatro ajudas estao prontas. Volte a Liora, em Aurora, e enfrente o Coracao do Veu para concluir a restauracao.";
 return "O Coracao do Veu recua. Liora e Seraphine seguram o mecanismo enquanto voce fixa o selo antigo. Aeldra tera outro amanhecer, mas o Pacto ainda nao foi reparado. 'O selo ainda depende das memorias aprisionadas', diz Liora. 'Ninguem recolhera novas lembrancas para alimenta-lo. Precisamos libertar quem continua preso.' Odran permanece sob custodia. Volte aos aliados: faltam contribuicoes para a restauracao. Quando as quatro cidades estiverem prontas, Liora espera por voce para enfrentar o Coracao outra vez. A aurora livre ainda pode ser construida.";
}
inline const char* finaleOrigin(const rpg::Game& g){
 if(!g.originStory)return "";
 constexpr const char* closure[]={
  "Voce pode voltar a procurar seu mestre, levando respostas sobre a cinza e as lembrancas. Ainda ha cuidado a oferecer, mesmo onde a magia nao traz tudo de volta.",
  "Voce deixou um posto para proteger uma familia. Agora pode voltar a estrada com um juramento digno de seguir: cuidar de pessoas continua sendo sua escolha.",
  "A estrada volta a oferecer trabalho a quem guarda as caravanas. Voce pode procurar sua familia, levando respostas e a chance de retomar seu oficio.",
  "Voce pode voltar a sua comunidade com respostas sobre a cinza. Ha terra e gente de que cuidar; sua jornada abriu um caminho para ajudar os seus."
 };return closure[g.p.cls%4];
}
inline const char* finaleLetter(){return "Os farois de Aeldra continuam firmes. Mas chegou uma carta de Neris, de Talassar. O mapa que ela enviou mostra uma ilha onde os capitaes so encontraram mar. Tambem acharam por la um marco parecido com os nossos. Guardei os relatos no porto. Quando estiver pronto, venha ve-los.";}
inline unsigned finaleTextPages(const char* text){return (wrapStory(text,18,[](unsigned,const char*){})+6)/7;}
// Each block starts on its own page; render and navigation share this count.
inline unsigned finalePages(const rpg::Game& g,unsigned mission){
 unsigned pages=finaleTextPages(finaleSpeech(g,mission));
 if(finaleFull(g,mission))pages+=finaleTextPages(finaleOrigin(g))+finaleTextPages(finaleLetter());
 return pages;
}
struct FinalePage {const char* title;Npc npc;const char* text;unsigned page;};
inline FinalePage finalePage(const rpg::Game& g,unsigned mission,unsigned page){
 page=std::min(page,finalePages(g,mission)-1);
 const char* speech=finaleSpeech(g,mission);unsigned count=finaleTextPages(speech);
 if(page<count)return {mission==8?"A QUEDA DE ODRAN":finaleFull(g,mission)?"EPILOGO DE AELDRA":"O SELO TEMPORARIO",Npc::None,speech,page};
 page-=count;const char* origin=finaleOrigin(g);count=finaleTextPages(origin);
 if(page<count)return {"SUA JORNADA",Npc::None,origin,page};
 return {"CARTA DE SABELA",Npc::Sabela,finaleLetter(),page-count};
}
}
template<class C>void finaleFoe(C& c,unsigned id,int x,int y,unsigned frame){
 if(id==18){npcPortrait(c,story::Npc::Odran,x,y+int(frame%2),64);return;}
 int cx=x+32,cy=y+33;unsigned pulse=frame%6;uint16_t outer=0x780f,inner=0xb3ff;
 for(int yy=-30;yy<=30;++yy){int half=30-abs(yy);c.fillRect(cx-half,cy+yy,half*2+1,1,outer);if(half>8)c.fillRect(cx-half+5,cy+yy,half-3,1,inner);}
 c.fillRect(cx-2,cy-22,4,44,0xffff);c.fillRect(cx-18,cy-2,36,4,0xffff);
 for(int i=0;i<6;++i){int dx=(i%2?1:-1)*(24+int(pulse)),dy=-23+i*9;c.fillRect(cx+dx,cy+dy,3,5,0x8fff);}
}
