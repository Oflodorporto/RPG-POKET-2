#pragma once
namespace launchUi {
constexpr scenicUi::Rect cancel={14,272,102,40},confirm={124,272,102,40},back={14,272,212,40},previous={14,222,102,40},next={124,222,102,40};
struct Guide {const char* title;const char* lines[7];};
constexpr Guide guides[]={
 {"PRIMEIROS PASSOS",{"Crie em um slot vazio; comece Nv1.","Carvalho: explore, compre e equipe.","Objetivo abre sua proxima tarefa.","Mapa conecta cidades e aventuras.","Ruinas: recomendado nivel 5+.","Tres vitorias liberam o Guardiao.","Um cristal abre a primeira dungeon."}},
 {"COMBATE E INTENCOES",{"Leia a intencao abaixo da cena.","Preparar: proximo golpe sera forte.","Defesa e escudo reduzem esse golpe.","Pocoes usam uma acao de combate.","Magia nao e dano fisico.","Bolsa e guia pausam o inimigo.","Reiniciar preserva o turno salvo."}},
 {"PODERES DAS CLASSES",{"Tecnicas > Poderes mostra seus usos.","Mago usa mana; ataque usa cajado.","Paladino cura; juramento no Nv3.","Guerreiro: Folego1 / Surto2.","Barbaro: Furia dura tres rodadas.","Descanso concluido repoe recursos.","Pocao ou reinicio nao repoe usos."}},
 {"VIAGEM E ACAMPAMENTO",{"Confira nivel, CD e chance segura.","Confirmar viagem usa suprimentos.","D20 natural1 falha; natural20 passa.","Cancelar nao gasta nem rola o dado.","Acampar sem racao recupera 50%.","Com racao, recupera 100% HP/MP.","Falha no teste gera um encontro."}},
 {"BOLSA E EQUIPAMENTO",{"Cada item possui quantidade e slot.","Cristais abrem dungeons nas Ruinas.","Loja compara o item com o equipado.","Comprar guarda; equipe pela bolsa.","Trocar amuleto nao recupera mana.","Armas de outras classes podem vender.","Venda e forja acontecem na cidade."}},
 {"DUNGEON E PROGRESSO",{"Setas ou gestos movem seu heroi.","Toque na cena para interagir/atacar.","Cajado atira; espada exige alcance.","Ache moedas, baus e o selo.","Venca o Arconte no segundo andar.","Chefe vencido: sair ou explorar.","O diario guarda suas descobertas."}},
 {"SAVES E ATUALIZACAO",{"Tres slots independentes para herois.","Exclusao exige sua confirmacao.","Saves automaticos a cada acao.","Falha de save pausa para tentar.","Atualizacao: Wi-Fi nas configuracoes.","Nao desligue durante instalacao.","Versoes antigas podem nao ler save."}}
};
constexpr unsigned guideCount=sizeof(guides)/sizeof(guides[0]);
}
template<class C>void drawLaunch(C& c,const rpg::Game& g,const ViewState& v){
 drawBackdrop(c,v.page==Page::TravelConfirm?bg_world:bg_guide);char b[64];
 auto button=[&](scenicUi::Rect r,const char* title){scenicPanel(c,r.x,r.y,r.w,r.h);panelLabel(c,r.x+4,r.y+15,r.w-8,title,UI_GOLD);};
 if(v.page==Page::Guide){const auto& topic=launchUi::guides[v.guideIndex%launchUi::guideCount];scenicHeading(c,"GUIA DO AVENTUREIRO");panelLabel(c,8,52,224,topic.title,UI_GOLD);for(unsigned i=0;i<7;++i)panelLabel(c,8,77+i*19,224,topic.lines[i]);snprintf(b,sizeof(b),"Pagina %u / %u",v.guideIndex+1,launchUi::guideCount);panelLabel(c,10,210,220,b,UI_BLUE);button(launchUi::previous,"< Anterior");button(launchUi::next,"Proxima >");button(launchUi::back,"Voltar");return;}
 if(v.page==Page::Recovery){scenicHeading(c,"RECUPERAR FORCAS");snprintf(b,sizeof(b),"%s / HP %u/%u / MP %u/%u",placeName(g.city),g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);panelLabel(c,8,53,224,b);panelLabel(c,8,76,224,"Prepare-se antes de tentar de novo.",UI_GOLD);button({14,98,212,40},"Bolsa / pocoes");button({14,148,212,40},"Acampar e recuperar");button({14,198,212,40},"Mapa / escolher caminho");panelLabel(c,10,249,220,"Nenhuma recuperacao automatica.",UI_MUTED);button(launchUi::back,"Voltar ao local");return;}
 unsigned dest=v.tripDestination;scenicHeading(c,"PREPARAR VIAGEM");snprintf(b,sizeof(b),"%s -> %s",placeName(g.city),placeName(dest));panelLabel(c,8,56,224,b,UI_GOLD);
 snprintf(b,sizeof(b),"Seu Nv %u / destino Nv %u+",g.p.level,rpg::cityLevel(dest));panelLabel(c,8,81,224,b,g.p.level<rpg::cityLevel(dest)?UI_RED:UI_GREEN);
 snprintf(b,sizeof(b),"Chance segura: %u%% / CD %u",rpg::tripSafety(g,dest),rpg::routeDifficulty(g.city,dest));panelLabel(c,8,105,224,b,UI_BLUE);
 panelLabel(c,8,134,224,"Ao confirmar, usa da sua bolsa:",UI_GOLD);
 snprintf(b,sizeof(b),"Racao %u / Mapa %u / Sorte %u",g.rations?1:0,g.charts?1:0,g.charms?1:0);panelLabel(c,8,157,224,b);
 panelLabel(c,8,184,224,g.p.level<rpg::cityLevel(dest)?"Encontros podem ser muito perigosos.":"Prepare HP, mana e equipamento.",UI_GOLD);
 panelLabel(c,8,207,224,"Falha no D20: inimigo na estrada.");panelLabel(c,8,237,224,*v.message?v.message:"Cancelar nao gasta nem rola dados.",*v.message?UI_RED:UI_MUTED);
 button(launchUi::cancel,"Cancelar");button(launchUi::confirm,"Viajar");
}
