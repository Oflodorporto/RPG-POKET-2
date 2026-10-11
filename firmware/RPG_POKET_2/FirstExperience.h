#pragma once
// Optional hints only. No Game fields, RNG, rewards or Save28 bytes are changed.
namespace firstExperience {
constexpr unsigned count=10;
constexpr uint16_t all=(1u<<count)-1,active=0x8000;
inline uint32_t pack(uint16_t bits){return uint32_t(bits)|(uint32_t(uint16_t(~bits))<<16);}
inline uint16_t unpack(uint32_t value){uint16_t bits=value;return uint16_t(value>>16)==uint16_t(~bits)&&!(bits&~(active|all))?bits:0;}
template<class Store>struct Progress {
 Store& store; uint16_t bits=0; uint8_t slot=0;
 explicit Progress(Store& s):store(s){}
 void load(unsigned s){slot=s;bits=s<3?unpack(store.readGuide(s)):0;}
 // Called only for an explicitly requested creation in an empty slot.
 void start(unsigned s){slot=s;bits=active;if(s>=3||!store.writeGuide(s,pack(bits)))bits=0;}
 bool pending(unsigned i)const{return i<count&&(bits&active)&&!(bits&(1u<<i));}
 bool acknowledge(unsigned i,bool skip){if(i>=count)return false;uint16_t next=skip?all:uint16_t(bits|(1u<<i));
  if(slot>=3||!store.writeGuide(slot,pack(next)))return false;bits=next;return true;}
};
inline int topic(Page page,const rpg::Game& g){switch(page){
 case Page::Home:case Page::Village:return 0;
 case Page::Menu:return 1;
 case Page::Explore:case Page::Ruins:return 2;
 case Page::Map:case Page::TravelConfirm:return 3;
 case Page::Battle:return g.phase==rpg::Phase::Hero?4:-1;
 case Page::Bag:case Page::TownBag:return 5;
 case Page::CampSetup:return 6;
 case Page::Campaign:case Page::Contract:case Page::GuildMissions:case Page::Result:return 7;
 case Page::Character:case Page::Evolution:return 8;
 case Page::Dungeon:return g.phase==rpg::Phase::Home||g.phase==rpg::Phase::Hero?9:-1;
 default:return -1;}}
struct Tip {const char* title;const char* text;};
constexpr Tip tips[]={
 {"UMA PORTA ABERTA","Nara: Carvalho precisa de voce. Toque em Conversar e procure Elarin ou Borin. Objetivo lembra seu proximo passo."},
 {"SEU CAMINHO","Toque nos botoes para abrir uma tela. Voltar retorna sem escolher. O Guia pode ser relido. Continuar na tela de titulo retoma seu ultimo passo salvo."},
 {"ALEM DOS MUROS","Explorar busca encontros nos arredores. Pode haver inimigos ou achados. Confira sua vida e sua bolsa antes de sair. Voce escolhe quando voltar."},
 {"PEGANDO A ESTRADA","Toque num destino para ver o caminho. Entrar prepara a viagem: confira nivel e risco antes de confirmar. O dado decide se ha perigo. Cancelar nao gasta itens."},
 {"SEU PRIMEIRO TURNO","HP e sua vida; MP alimenta poderes. Atacar usa sua arma. Tecnicas mostra habilidades e custos. Escolha uma acao e aguarde o inimigo. Bolsa oferece pocoes."},
 {"DENTRO DA BOLSA","Toque num slot para ler o item e a quantidade. Usar bebe a pocao escolhida. Em combate, isso usa uma acao. Equipamentos tem sua propria lista na bolsa."},
 {"RECUPERAR FORCAS","Acampar recupera metade da vida e mana. Com racao, recupera tudo. Confira suas reservas antes de escolher. Se houver perigo, voce luta antes de descansar."},
 {"TRABALHO E RECOMPENSA","Leia o pedido antes de aceitar. Objetivo e Diario ajudam a acompanhar a jornada. Conclua a tarefa e confirme a entrega para receber a recompensa. Contratos pedem retorno a Guilda."},
 {"CRESCER EM AELDRA","XP e experiencia: tarefas e vitorias ajudam a subir de nivel. Evolucao mostra os proximos poderes. Confira seus recursos e equipamentos. Cada classe aprende de um jeito."},
 {"PASSOS NO ESCURO","Na dungeon, as setas movem e giram seu heroi. Voce tambem pode deslizar o dedo. Toque curto na cena interage ou ataca. Cajado alcanca longe; espada exige chegar perto."}
};
}
template<class C>void drawFirstExperience(C& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 if(!drawStoryBackdrop(c,backdropFor(v.hintReturn,g),v,frame))return;
 const auto& tip=firstExperience::tips[v.hintIndex%firstExperience::count];
 auto text=[&](int x,int y,const char* s,int size=1){c.setTextColor(STORY_INK);c.setTextSize(size);c.setCursor(x,y);c.print(s);};
 text((240-int(strlen(tip.title))*6)/2,22,tip.title);
 npcPortrait(c,story::Npc::Nara,16,40,40);text(68,48,"NARA VELD");text(68,65,"Uma dica para sua jornada");
 story::wrapStory(tip.text,33,[&](unsigned row,const char* line){text(20,96+row*15,line);});
 text(32,235,"Nada foi escolhido por voce.");
 if(v.hintError)text(20,250,"Falha ao guardar; tente de novo.");
 auto button=[&](int x,const char* label){c.fillRect(x,272,102,40,UI_PANEL);c.drawRect(x,272,102,40,UI_GOLD);c.setTextColor(UI_WHITE);c.setTextSize(1);c.setCursor(x+(102-int(strlen(label))*6)/2,287);c.print(label);};
 button(14,"Pular dicas");button(124,"Entendi");
}
