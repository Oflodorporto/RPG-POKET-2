#pragma once
namespace launchUi {
constexpr scenicUi::Rect cancel={14,272,102,40},confirm={124,272,102,40},back={14,272,212,40},previous={14,222,102,40},next={124,222,102,40};
struct Guide {const char* title;const char* lines[7];};
constexpr Guide guides[]={
 {"PRIMEIROS PASSOS",{"Crie em um slot vazio; comece Nv1.","Carvalho: explore, compre e equipe.","Objetivo abre sua proxima tarefa.","Mapa conecta cidades e aventuras.","Ruinas: recomendado nivel 5+.","Tres vitorias liberam o Guardiao.","Um cristal abre a primeira dungeon."}},
 {"COMBATE E INTENCOES",{"Leia a intencao abaixo da cena.","Preparar: proximo golpe sera forte.","Defesa e escudo reduzem esse golpe.","Pocoes usam uma acao de combate.","Magia nao e dano fisico.","Bolsa e guia pausam o inimigo.","Reiniciar preserva o turno salvo."}},
 {"PODERES DAS CLASSES",{"Tecnicas > Poderes mostra seus usos.","Mago usa mana; ataque usa cajado.","Paladino cura; juramento no Nv3.","Guerreiro: Folego1 / Surto2.","Barbaro: Furia dura tres rodadas.","Descanso concluido repoe recursos.","Pocao ou reinicio nao repoe usos."}},
 {"VIAGEM E ACAMPAMENTO",{"Confira nivel, CD e chance segura.","Uma racao por dia; leve o total.","D20 natural1 falha; natural20 passa.","Cancelar nao gasta nem rola o dado.","Acampar sem racao recupera 50%.","Com racao, recupera 100% HP/MP.","Falha no teste gera um encontro."}},
 {"BOLSA E EQUIPAMENTO",{"Cada item possui quantidade e slot.","Cristais abrem dungeons nas Ruinas.","Loja compara o item com o equipado.","Comprar guarda; equipe pela bolsa.","Trocar amuleto nao recupera mana.","Armas de outras classes podem vender.","Venda e forja acontecem na cidade."}},
 {"DUNGEON E PROGRESSO",{"Setas ou gestos movem seu heroi.","Toque na cena para interagir/atacar.","Cajado atira; espada exige alcance.","Ache moedas, baus e o selo.","Venca o Arconte no segundo andar.","Chefe vencido: sair ou explorar.","O diario guarda suas descobertas."}},
 {"SAVES E ATUALIZACAO",{"Tres slots independentes para herois.","Exclusao exige sua confirmacao.","Saves automaticos a cada acao.","Falha de save pausa para tentar.","Atualizacao: Wi-Fi nas configuracoes.","Nao desligue durante instalacao.","Versoes antigas podem nao ler save."}}
};
constexpr unsigned guideCount=sizeof(guides)/sizeof(guides[0]);
}
template<class C>void drawTravelPreparation(C& c,const rpg::Game& g,const ViewState& v){
 const bool ready=menu.art==ArtStatus::Ready;if(ready)sceneArt::draw(c,sceneArt::Id::preparar_viagem);else c.fillScreen(0xef19);
 char b[64];const uint16_t ink=0x20c3,accent=0x6222;auto line=[&](int y,const char* text,uint16_t color=0x20c3){touchLabel(c,28,y,184,text,color);};
 carvalhoUi::frame(c,{52,12,176,28});touchLabel(c,56,22,168,"PREPARAR VIAGEM",UI_GOLD);
 unsigned dest=v.tripDestination;snprintf(b,sizeof(b),"%s -> %s",placeName(g.city),placeName(dest));line(59,b,accent);
 snprintf(b,sizeof(b),"Seu Nv %u / destino Nv %u+",g.p.level,rpg::cityLevel(dest));line(80,b);
 snprintf(b,sizeof(b),"%u dias / %u pernoites",rpg::dailyTripDays(g.city,dest),rpg::dailyTripDays(g.city,dest)-1);line(100,b);
 snprintf(b,sizeof(b),"Partida: %u%% segura / CD %u",rpg::tripSafety(g,dest),rpg::routeDifficulty(g.city,dest));line(119,b);
 line(140,"Uma racao por dia de viagem.",accent);
 snprintf(b,sizeof(b),"Precisa %u / na bolsa %u",rpg::dailyTripDays(g.city,dest),g.rations);line(157,b);
 snprintf(b,sizeof(b),"Partida: mapa %u / talisma %u",g.charts?1:0,g.charms?1:0);line(175,b);
 line(192,"Pernoite tem seu proprio D20.");
 line(209,"Kit ajuda; descanso sem taxa.",accent);
 line(226,g.p.level<rpg::cityLevel(dest)?"Destino acima do seu nivel!":"Toque Continuar apos descansar.",accent);
 if(*v.message)touchLines(c,28,242,184,v.message,0x7800);else line(247,"Cancelar nao gasta nem rola.",ink);
 for(auto r:{launchUi::cancel,launchUi::confirm})carvalhoUi::frame(c,r);
 touchLabel(c,18,288,94,"Cancelar",UI_GOLD);touchLabel(c,128,288,94,"Viajar",UI_GOLD);
}
template<class C>void drawLaunch(C& c,const rpg::Game& g,const ViewState& v){
 if(v.page==Page::TravelConfirm){drawTravelPreparation(c,g,v);return;}
 drawBackdrop(c,bg_guide);char b[64];
 auto button=[&](scenicUi::Rect r,const char* title){scenicPanel(c,r.x,r.y,r.w,r.h);panelLabel(c,r.x+4,r.y+15,r.w-8,title,UI_GOLD);};
 if(v.page==Page::Guide){const auto& topic=launchUi::guides[v.guideIndex%launchUi::guideCount];scenicHeading(c,"GUIA DO AVENTUREIRO");panelLabel(c,8,52,224,topic.title,UI_GOLD);for(unsigned i=0;i<7;++i)panelLabel(c,8,77+i*19,224,topic.lines[i]);snprintf(b,sizeof(b),"Pagina %u / %u",v.guideIndex+1,launchUi::guideCount);panelLabel(c,10,210,220,b,UI_BLUE);button(launchUi::previous,"< Anterior");button(launchUi::next,"Proxima >");button(launchUi::back,"Voltar");return;}
 if(v.page==Page::Recovery){scenicHeading(c,"RECUPERAR FORCAS");snprintf(b,sizeof(b),"%s / HP %u/%u / MP %u/%u",placeName(g.city),g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);panelLabel(c,8,53,224,b);panelLabel(c,8,76,224,"Prepare-se antes de tentar de novo.",UI_GOLD);button({14,98,212,40},"Bolsa / pocoes");button({14,148,212,40},"Acampar e recuperar");button({14,198,212,40},"Mapa / escolher caminho");panelLabel(c,10,249,220,"Nenhuma recuperacao automatica.",UI_MUTED);button(launchUi::back,rpg::escortCampAllowed(g)?"Voltar a missao":"Voltar ao local");return;}
}
