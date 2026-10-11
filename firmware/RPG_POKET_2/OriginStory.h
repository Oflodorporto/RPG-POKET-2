#pragma once
#include "Rules.h"
#include "NpcArt.h"
namespace story {
struct OriginPage {const char* title;Npc npc;const char* text;};
constexpr OriginPage origins[4][3]={
 {{"SUA ORIGEM",Npc::None,"Voce cresceu num povoado de Aeldra. Consertava encantamentos para pagar as aulas de magia."},
  {"SEU APRENDIZADO",Npc::None,"Seu mestre lhe deu um cajado e ensinou a controlar o fogo. Agora ele esquece os nomes dos alunos."},
  {"POR QUE PARTIR",Npc::None,"As falhas comecaram com a cinza branca. Voce vai a Carvalho buscar Elarin. Quer salvar seu mestre."}},
 {{"SUA ORIGEM",Npc::None,"Voce cresceu junto a uma estrada de Aeldra. Aprendeu espada, escudo e cura para proteger viajantes."},
  {"SUA PRIMEIRA ESCOLTA",Npc::None,"Uma ordem mandou abandonar uma familia na chuva. Voce ficou. Perdeu o posto, mas todos chegaram vivos."},
  {"POR QUE PARTIR",Npc::None,"Voce busca trabalho em Carvalho e um juramento digno de seguir. Proteger pessoas vem antes das ordens."}},
 {{"SUA ORIGEM",Npc::None,"Voce carregava sacos nas caravanas de Aeldra. Um veterano lhe ensinou espada e como guardar a estrada."},
  {"SEU OFICIO",Npc::None,"Virou guarda de caravana. Quando as feras fecharam a rota, perdeu o trabalho. Sua familia depende dele."},
  {"POR QUE PARTIR",Npc::None,"Voce vai a Carvalho buscar contratos. Quer reabrir a estrada, sustentar os seus e voltar com orgulho."}},
 {{"SUA ORIGEM",Npc::None,"Voce cresceu num povoado de Aeldra. Aprendeu a cacar e lutar para defender os seus, nao para um rei."},
  {"A TERRA FERIDA",Npc::None,"A cinza branca secou a horta. Os animais fugiram. Sua comunidade dividiu as ultimas racoes com voce."},
  {"POR QUE PARTIR",Npc::None,"Voce leva sua arma a Carvalho em busca de ajuda. Quer comida, respostas e forca para proteger sua casa."}}
};
constexpr OriginPage arrival[]={
 {"CHEGADA A CARVALHO",Npc::None,"Apos tres dias na estrada, voce ajuda a empurrar uma carroca ate Carvalho. Esta sem trabalho e quase sem ouro."},
 {"UMA PORTA ABERTA",Npc::Nara,"Entre, tire a capa molhada. Tenho sopa e uma cama. Amanha Elarin e Borin podem lhe indicar trabalho."},
 {"O MENSAGEIRO",Npc::Messenger,"Feras tomaram a estrada! A carta e da Guilda. Vi cinza branca nas raizes e ouvi o sino da torre vazia."},
 {"UMA CARTA DE MAELIS",Npc::Maelis,"Precisamos investigar a cinza e manter a estrada aberta. Fale com Elarin e Borin. Carvalho precisa de ajuda."},
 {"SEU PRIMEIRO OBJETIVO",Npc::Nara,"Abra Conversar em Carvalho. Procure Elarin e Borin: eles conhecem a estrada. Seu primeiro trabalho comeca aqui."}
};
inline const char* nextUnlock(unsigned cls){constexpr const char* label[]={"Nv 3: Raios abrasadores","Nv 3: Arma sagrada","Nv 2: Surto de acao","Nv 5: Ataque extra"};return label[cls%4];}
inline unsigned originCount(const rpg::Game& g){return g.originStory?8:5;}
inline const OriginPage& originPage(const rpg::Game& g,unsigned index){index=std::min(index,originCount(g)-1);if(g.originStory&&index<3)return origins[g.p.cls%4][index];return arrival[index-(g.originStory?3:0)];}
// Word wrapping uses whole words and reports overflow for native layout tests.
template<class Line> unsigned wrapStory(const char* text,unsigned columns,Line line){
 unsigned rows=0;while(*text){while(*text==' ')++text;if(!*text)break;unsigned n=0;while(text[n]&&n<columns)++n;
  if(text[n]&&text[n]!=' '){unsigned split=n;while(split&&text[split]!=' ')--split;if(split)n=split;}
  char buffer[41];n=std::min(n,40u);memcpy(buffer,text,n);buffer[n]=0;line(rows++,buffer);text+=n;
 }return rows;
}
}
