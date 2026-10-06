#pragma once
#include "Rules.h"
#include "BackgroundArt.h"
#include "SpriteArt.h"
#include "HeroArt.h"
#include "PersonalArt.h"
#include "ClubUi.h"
#include "GearArt.h"
#include "CombatFx.h"
#include "CardCheck.h"
#include <stdio.h>
#include <string.h>
enum class Page { Choose, Help, Home, Battle, Skills, Bag, Result, SaveError, Blocked, Village, Shop, Buy, TownBag, Character, Map, Market, Inventory, GearShop, GearBag, GearBuy, GearEquip, Forge, Upgrade, Tavern, Contract, QuestConfirm, Card, Menu, Slots, SlotConfirm, DeleteSlot, Settings, Wifi, Keyboard, Tests, Travel, Ruins, Guild, GuildJoin, GuildMissions, Race, Clothes, Club, ClubBattle, ClubResult, Updates };
inline const Backdrop& backdropFor(Page page,const rpg::Game& g){
  switch(page){
  case Page::Guild:return bg_guild;case Page::GuildJoin:return bg_guildjoin;case Page::GuildMissions:return bg_missions;case Page::Race:return bg_race;case Page::Clothes:return bg_clothes;case Page::Club:return bg_club;case Page::ClubBattle:return bg_clubbattle;case Page::ClubResult:return bg_clubresult;
  case Page::Menu:return bg_menu;case Page::Slots:return bg_slots;case Page::SlotConfirm:return bg_slotconfirm;case Page::DeleteSlot:return bg_delete;case Page::Settings:return bg_settings;case Page::Wifi:return bg_wifi;case Page::Keyboard:return bg_keyboard;case Page::Tests:return bg_tests;case Page::Travel:return bg_world;case Page::Ruins:return bg_map;
  case Page::Updates:return bg_updates;case Page::Card:return bg_card;
  case Page::Tavern:return bg_tavern;case Page::Contract:return bg_contract;case Page::QuestConfirm:return bg_questconfirm;
  case Page::Choose:return bg_choose;case Page::Help:return bg_guide;case Page::Home:return bg_camp;
  case Page::Village:return g.city==2?bg_port:g.city==3?bg_castle:bg_village;case Page::Map:return bg_world;case Page::Character:return bg_character;
  case Page::Market:return bg_market;case Page::Inventory:return bg_inventory;case Page::Shop:return bg_shop;
  case Page::Buy:return bg_buy;case Page::GearShop:return bg_gearshop;case Page::GearBag:return bg_gearbag;
  case Page::GearBuy:return bg_gearbuy;case Page::GearEquip:return bg_gearequip;case Page::Forge:return bg_forge;
  case Page::Upgrade:return bg_upgrade;case Page::Skills:return bg_skills;case Page::Bag:return bg_bag;
  case Page::TownBag:return bg_townbag;case Page::SaveError:return bg_saveerror;case Page::Blocked:return bg_blocked;
  case Page::Result:return g.phase==rpg::Phase::Won?bg_won:g.phase==rpg::Phase::Lost?bg_lost:bg_fled;
  case Page::Battle:return g.enemyId==0?bg_battle0:g.enemyId==1?bg_battle1:g.enemyId==2?bg_battle2:bg_battle3;
  }return bg_camp;
}
struct ViewState {Page page=Page::Choose;uint8_t choice=0,gearIndex=0,itemId=0,forgeSlot=0,questChoice=1,questAction=0;const char* message="";bool recovered=false;Effect effect=Effect::None;unsigned effectFrame=0,heroFrame=0;bool effectOnHero=false,touchFeedback=false;CardInfo card;};
constexpr uint16_t UI_INK=0x1083,UI_PANEL=0x1926,UI_GOLD=0xd5aa,UI_WHITE=0xef1b,UI_MUTED=0x9cf4,UI_GREEN=0x6e0c,UI_RED=0xe28b,UI_BLUE=0x549f;
#include "MenuView.h"
// Rendering shared by device and native screenshot test. No gameplay mutation.
template<class Canvas> struct WifiOverlay {
  Canvas& c;bool connected;
  ~WifiOverlay(){if(!connected)return;const uint16_t green=0x07e0;c.fillRect(221,1,19,19,UI_INK);
    c.fillRect(229,15,3,3,green);c.fillRect(227,10,7,2,green);c.fillRect(225,12,2,2,green);c.fillRect(234,12,2,2,green);
    c.fillRect(225,5,11,2,green);c.fillRect(223,7,2,2,green);c.fillRect(236,7,2,2,green);}
};
template<class Canvas> void render(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame=0){
  WifiOverlay<Canvas> wifiOverlay{c,menu.connected};
  auto text=[&](int x,int y,const char* s,int size=1,uint16_t color=UI_WHITE){if(*s)c.fillRect(x-3,y-2,int(strlen(s))*6*size+6,8*size+4,UI_INK);c.setTextColor(color);c.setTextSize(size);c.setCursor(x,y);c.print(s);};
  auto center=[&](int y,const char* s,int size=1,uint16_t color=UI_WHITE){text((240-int(strlen(s))*6*size)/2,y,s,size,color);};
  auto box=[&](int x,int y,int w,int h){c.fillRect(x,y,w,h,UI_PANEL);c.drawRect(x,y,w,h,UI_GOLD);c.drawRect(x+2,y+2,w-4,h-4,0x3186);};
  auto button=[&](int x,int y,int w,const char* s){box(x,y,w,40);c.setTextColor(UI_WHITE);c.setTextSize(2);c.setCursor(x+(w-int(strlen(s))*12)/2,y+12);c.print(s);};
  auto sprite=[&](int x,int y,const uint16_t* pixels,int w,int h,bool outline=false){
    if(outline)for(int row=0;row<h;++row){int col=0;while(col<w){while(col<w&&pixels[row*w+col]==SPRITE_KEY)++col;int start=col;while(col<w&&pixels[row*w+col]!=SPRITE_KEY)++col;if(col>start)c.fillRect(x+start-1,y+row-1,col-start+2,3,UI_MUTED);}}
    for(int row=0;row<h;++row){int col=0;while(col<w){while(col<w&&pixels[row*w+col]==SPRITE_KEY)++col;int start=col;while(col<w&&pixels[row*w+col]!=SPRITE_KEY)++col;if(col>start)c.draw16bitRGBBitmap(x+start,y+row,const_cast<uint16_t*>(pixels+row*w+start),col-start,1);}}
  };
  auto portrait=[&](uint8_t cls,int x,int y,const rpg::Game* overrideGame=nullptr){box(x-1,y-1,82,68);auto who=overrideGame?*overrideGame:g;who.p.cls=cls;personalSprite(c,who,x,y,6);c.drawRect(x-1,y-1,82,68,UI_GOLD);};
  char b[64];drawBackdrop(c,backdropFor(v.page,g));c.setTextWrap(false);if(v.touchFeedback)c.fillRect(232,3,5,5,UI_GREEN);
  if(renderMenu(c,g,int(v.page),text,center,box,button,portrait,frame))return;
  if(v.page==Page::SaveError||v.page==Page::Blocked){
    center(30,"RPG POKET 2.0",2,UI_GOLD);center(88,"PROGRESSO PROTEGIDO",1,UI_RED);
    center(119,v.page==Page::SaveError?"Nao foi possivel salvar.":"Save indisponivel ou incompativel.");
    center(140,"Nenhum save foi apagado.");
    center(163,v.page==Page::SaveError?"Mantenha o aparelho ligado.":"Informe esta tela ao desenvolvedor.");
    if(v.page==Page::SaveError)button(14,250,212,"Tentar salvar");else button(14,250,212,"Menu");return;
  }
  if(v.page==Page::Club){center(12,"CLUBE DA LUTA",2,UI_GOLD);center(37,"Duelo entre placas / entrada 25 ouro");
    auto state=arena.session.state;if(state==club::State::Listing||state==club::State::Notice){for(unsigned i=0;i<6;++i){int y=54+i*25;box(14,y,212,24);auto& peer=arena.session.peers[i];snprintf(b,sizeof(b),peer.used?"%s / Nv %u%s":"Procurando aventureiros...",peer.name,peer.level,peer.available?"":" / ocupado");text(20,y+8,b,1,int(i)==arena.choice?UI_GOLD:UI_WHITE);}button(14,218,212,"Convidar / 25g");}
    else {center(95,arena.session.opponent.name,2);center(135,state==club::State::Incoming?"Aceitar duelo por 25 ouro?":"Aguardando outra placa...");if(state==club::State::Incoming){button(14,218,102,"Recusar");button(124,218,102,"Aceitar");}}
    center(260,arena.notice,1,UI_RED);button(14,272,212,"Voltar");return;}
  if(v.page==Page::ClubBattle){auto& d=arena.duel;auto& me=d.fighters[d.local];auto& foe=d.fighters[1-d.local];center(10,"ARENA DA GUILDA",2,UI_GOLD);personalSprite(c,g,8,44,frame%6);auto opponent=rpg::create(foe.classId,1);personalSprite(c,opponent,124,44,frame%6,true);
    snprintf(b,sizeof(b),"HP %u/%u",me.hp,me.maxHP);text(14,170,b);snprintf(b,sizeof(b),"HP %u/%u",foe.hp,foe.maxHP);text(144,170,b);snprintf(b,sizeof(b),"MP %u / %s",me.mp,d.waiting?"Confirmando...":!d.ready?"Preparando...":d.turn==d.local?"Seu turno":"Turno adversario");center(190,b,1,UI_GOLD);
    button(14,220,102,"Ataque");button(124,220,102,"Ofensiva");button(14,270,102,"Defesa");button(124,270,102,"Desistir");return;}
  if(v.page==Page::ClubResult){center(12,"FIM DO DUELO",2,UI_GOLD);center(96,arena.result==2?"CANCELADO":arena.result>0?"VITORIA":arena.result==0?"EMPATE":"DERROTA",2);center(138,arena.result==2?"Entrada devolvida: 25 ouro":arena.result>0?"Premio: 45 ouro":arena.result==0?"Entrada devolvida: 25 ouro":"Entrada consumida: 25 ouro");center(177,"Antes do inicio: devolucao integral.");snprintf(b,sizeof(b),"Saldo: %lu ouro",(unsigned long)g.p.gold);center(206,b,1,UI_GOLD);center(245,arena.notice,1,UI_MUTED);button(14,272,212,"Voltar");return;}
  if(v.page==Page::Race){center(12,"ESCOLHA SUA RACA",2,UI_GOLD);
    for(unsigned i=0;i<4;++i){int x=14+(i%2)*110,y=46+(i/2)*104;box(x,y,102,98);auto draft=rpg::create(v.choice,1);draft.race=i;personalSprite(c,draft,x+11,y+5,6);text(x+12,y+78,raceName(i),1,i==menu.draftRace?UI_GOLD:UI_WHITE);if(i==menu.draftRace)c.drawRect(x+2,y+2,98,94,UI_GREEN);}
    button(14,263,102,"Voltar");button(124,263,102,"Classe >");return;}
  if(v.page==Page::Clothes){center(10,"CORES DA ROUPA",2,UI_GOLD);auto draft=rpg::create(v.choice,1);draft.race=menu.draftRace;draft.shirt=menu.draftShirt;draft.trousers=menu.draftPants;personalSprite(c,draft,64,36,0);center(158,raceName(draft.race));
    button(14,177,102,"Roupa <");button(124,177,102,"Roupa >");button(14,224,102,"Calca <");button(124,224,102,"Calca >");
    button(14,272,102,"Voltar");button(124,272,102,"Criar");return;}
  if(v.page==Page::Guild||v.page==Page::GuildJoin){center(12,"GUILDA",2,UI_GOLD);center(43,"DOS AVENTUREIROS",2,UI_GOLD);snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(88,b);
    center(118,g.guildMember?"Membro cadastrado":"Cadastro: 100 ouro / uma vez");center(145,v.message,1,UI_RED);
    if(v.page==Page::GuildJoin){center(182,"Pagar 100 ouro para se cadastrar?");button(14,272,102,"Cancelar");button(124,272,102,"Pagar");}
    else{button(14,170,212,g.guildMember?"Missoes":"Cadastrar");button(14,218,212,"Clube da luta");button(14,272,212,"Voltar");}return;}
  if(v.page==Page::Tavern){center(12,"TAVERNA",2,UI_GOLD);center(95,"O taberneiro aponta para a guilda.");center(119,"Os contratos ficam com os aventureiros.");center(167,v.message,1,UI_GREEN);button(14,220,212,"Ir a guilda");button(14,272,212,"Voltar");return;}
  if(v.page==Page::Choose){
    for(int y=8;y<26;y+=7)c.fillRect(8,y,20,3,UI_GOLD);center(12,"RPG POKET 2.0",2,UI_GOLD);center(38,"ESCOLHA SUA CLASSE");auto draft=g;draft.race=menu.draftRace;portrait(v.choice,80,57,&draft);
    center(136,rpg::className(v.choice),2);auto p=rpg::create(v.choice,1).p;
    snprintf(b,sizeof(b),"HP %u  MP %u  ATQ %u  DEF %u",p.hp,p.mp,p.atk,p.def);center(161,b);
    const char* motto[]={"Magia e estrategia","Protecao e coragem","Precisao em cada golpe","Forca e resistencia"};center(180,motto[v.choice],1,UI_MUTED);
    button(14,211,102,"< Voltar");button(124,211,102,"Proxima>");button(14,263,212,"Personalizar");return;
  }
  if(v.page==Page::Help){
    center(14,"GUIA DO VIAJANTE",2,UI_GOLD);
    c.fillRect(10,64,220,82,UI_INK);text(19,75,"Um toque, uma acao.",1);
    text(19,92,"Ataque e aguarde o inimigo.");text(19,109,"Habilidades consomem mana.");text(19,126,"Pocoes tambem usam um turno.");
    center(184,"Descanse para recuperar HP/MP.");center(205,"O progresso e salvo a cada turno.");
    center(229,v.recovered?"Checkpoint anterior recuperado.":"Reiniciar retoma sua aventura.",1,UI_MUTED);
    button(14,268,212,"Continuar");return;
  }
  if(v.page==Page::Home){
    center(10,"REFUGIO",2,UI_GOLD);
    portrait(g.p.cls,14,80);box(102,80,124,66);text(111,88,rpg::className(g.p.cls));snprintf(b,sizeof(b),"Nivel %u",g.p.level);text(111,106,b,2);
    snprintf(b,sizeof(b),"%lu ouro",(unsigned long)g.p.gold);text(111,130,b,1,UI_GOLD);
    snprintf(b,sizeof(b),"HP %u/%u MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);center(169,b);
    snprintf(b,sizeof(b),"XP %lu/%u",(unsigned long)g.p.xp,rpg::xpNeeded(g.p.level));center(187,b,1,UI_MUTED);
    center(203,v.message,1,UI_GREEN);button(14,220,212,"Mapa do mundo");button(14,270,102,"Descanso");button(124,270,102,"Menu");return;
  }
  if(v.page==Page::Ruins){
    center(10,"RUINAS ANTIGAS",2,UI_GOLD);
    box(14,98,212,54);snprintf(b,sizeof(b),"Vitorias: %u/3",g.ruinsWins);center(106,b,1,UI_GOLD);
    center(124,g.guardianDefeated?"Guardiao vencido!":g.ruinsWins==3?"Guardiao liberado!":"Venca 3 encontros para liberar.");
    center(140,v.message,1,UI_RED);if(g.questId){snprintf(b,sizeof(b),"Missao: %u/%u%s",g.questProgress,rpg::contract(g.questId).count,rpg::questComplete(g)?" / volte a guilda":"");center(156,b,1,UI_GREEN);}button(14,166,212,"Explorar");
    box(14,214,212,40);center(222,g.ruinsWins<3?"Guardiao bloqueado":"Desafiar Guardiao",1,g.ruinsWins<3?UI_MUTED:UI_GOLD);
    center(239,"HP 28 / ATQ 6 / DEF 6",1,UI_MUTED);button(14,262,212,"Voltar");return;
  }
  if(v.page==Page::Village){
    center(12,placeName(g.city),2,UI_GOLD);snprintf(b,sizeof(b),"%lu OURO",(unsigned long)g.p.gold);box(66,68,108,30);center(80,b,1,UI_GOLD);center(109,v.message,1,UI_GREEN);
    button(14,126,102,"Loja");button(124,126,102,"Bolsa");button(14,170,102,"Heroi");button(124,170,102,"Taverna");
    button(14,214,102,"Guilda");button(124,214,102,"Descanso");button(14,258,102,"Mapa");button(124,258,102,"Menu");return;
  }
  if(v.page==Page::GuildMissions){
    center(12,"MISSOES",2,UI_GOLD);center(43,v.message,1,UI_GREEN);
    center(69,g.questId?"CONTRATO ATIVO":"ESCOLHA UM CONTRATO",1,UI_GOLD);
    if(g.questId){snprintf(b,sizeof(b),"%s: %u/%u",rpg::contract(g.questId).name,g.questProgress,rpg::contract(g.questId).count);center(91,b);}
    else center(91,"Uma missao por vez / ruinas.");
    for(uint8_t id=1;id<=3;++id){int y=124+(id-1)*44;box(14,y,212,40);center(y+6,rpg::contract(id).name,1,UI_GOLD);
      if(g.questId==id)snprintf(b,sizeof(b),"%u/%u / %s",g.questProgress,rpg::contract(id).count,rpg::questComplete(g)?"RECEBER":"EM ANDAMENTO");
      else snprintf(b,sizeof(b),"%u XP / %u ouro",rpg::contractXp(id,g.p.level),rpg::contractGold(id,g.p.level));center(y+24,b);}
    button(14,272,212,"Voltar");return;
  }
  if(v.page==Page::Contract){
    uint8_t id=v.questChoice;if(id<1||id>3){center(100,"Contrato indisponivel");return;}
    bool active=g.questId==id;uint8_t level=active?g.questLevel:g.p.level;
    center(12,"CONTRATO",2,UI_GOLD);center(60,rpg::contract(id).name,2);center(89,rpg::contract(id).objective);center(108,"REGIAO: RUINAS",1,UI_GOLD);
    snprintf(b,sizeof(b),"Progresso: %u/%u",active?g.questProgress:0,rpg::contract(id).count);center(132,b,2);
    snprintf(b,sizeof(b),"Recompensa: %u XP / %u ouro",rpg::contractXp(id,level),rpg::contractGold(id,level));center(164,b,1,UI_GOLD);
    snprintf(b,sizeof(b),"Calculada no nivel %u",level);center(185,b);center(201,v.message,1,UI_RED);
    button(14,220,212,active?(rpg::questComplete(g)?"Receber":"Abandonar"):"Aceitar");button(14,272,212,"Voltar");return;
  }
  if(v.page==Page::QuestConfirm){
    uint8_t id=v.questChoice,action=v.questAction;if(id<1||id>3||action>2){center(100,"Contrato indisponivel");return;}
    uint8_t level=action==0?g.p.level:g.questLevel;
    center(12,action==0?"ACEITAR MISSAO":action==1?"RECEBER PREMIO":"ABANDONAR MISSAO",2,UI_GOLD);
    center(72,rpg::contract(id).name,2);center(105,rpg::contract(id).objective);
    if(action==2){center(146,"O progresso sera perdido.",1,UI_RED);center(176,"Voce nao recebera recompensa.");center(205,"Podera aceitar novamente do zero.");}
    else{snprintf(b,sizeof(b),"%u XP / %u ouro",rpg::contractXp(id,level),rpg::contractGold(id,level));center(148,b,2,UI_GOLD);
      if(action==0){center(181,"Contam apenas as novas vitorias.");center(207,"Volte aqui para receber depois.");}
      else{snprintf(b,sizeof(b),"Ouro: %lu -> %lu",(unsigned long)g.p.gold,(unsigned long)(g.p.gold+rpg::contractGold(id,level)));center(184,b);
        auto preview=g;rpg::claimQuest(preview);snprintf(b,sizeof(b),"Nivel: %u -> %u",g.p.level,preview.p.level);center(207,b);}}
    center(245,v.message,1,UI_RED);button(14,270,102,"Cancelar");button(124,270,102,action==0?"Aceitar":action==1?"Receber":"Desistir");return;
  }
  if(v.page==Page::Character){
    center(12,"PERSONAGEM",2,UI_GOLD);portrait(g.p.cls,80,40);center(117,rpg::className(g.p.cls),2);
    snprintf(b,sizeof(b),"Nivel %u   XP %lu/%u",g.p.level,(unsigned long)g.p.xp,rpg::xpNeeded(g.p.level));center(143,b);
    snprintf(b,sizeof(b),"HP %u/%u  MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);center(162,b);
    snprintf(b,sizeof(b),"ATAQUE %u  DEFESA %u",unsigned(rpg::effectiveAttack(g)),unsigned(rpg::effectiveDefense(g)));center(181,b);
    snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(200,b,1,UI_GOLD);
    snprintf(b,sizeof(b),"Pocoes: HP x%u / MP x%u",g.p.life,g.p.mana);center(219,b);
    button(14,270,212,"Voltar");return;
  }
  if(v.page==Page::Market||v.page==Page::Inventory){
    bool shop=v.page==Page::Market;center(12,shop?"LOJA DA CIDADE":"SUA BOLSA",2,UI_GOLD);
    portrait(g.p.cls,80,shop?38:48);snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(shop?114:136,b,1,UI_GOLD);
    if(shop){button(14,138,212,"Pocoes");button(14,182,212,"Equipamentos");button(14,226,212,"Ferreiro");button(14,270,212,"Voltar");}
    else{snprintf(b,sizeof(b),"Equipamentos: %u",rpg::gearOwnedCount(g.owned));center(155,b);
      button(14,180,212,"Pocoes");button(14,226,212,"Equipamentos");button(14,272,102,"Menu");button(124,272,102,"Voltar");}return;
  }
  if(v.page==Page::Card){
    center(12,"CARTAO MICROSD",2,UI_GOLD);center(43,v.message,1,UI_GOLD);
    const char* status[]={"Toque em Verificar","Cartao nao reconhecido","Cartao reconhecido","Arquivo de teste diferente","Falha durante a leitura","LEITURA CONFIRMADA"};
    center(78,status[unsigned(v.card.status)<=5?unsigned(v.card.status):0],1,v.card.status==CardStatus::Verified?UI_GREEN:UI_WHITE);
    if(v.card.capacityMiB){snprintf(b,sizeof(b),"Capacidade: %lu MiB",(unsigned long)v.card.capacityMiB);center(103,b);}
    center(131,v.card.status==CardStatus::Missing?"Falta RPGPOKET/teste.bin":v.card.status==CardStatus::Verified?"Arquivo de teste: OK":v.card.status==CardStatus::WrongFile?"Copie o teste.bin novamente.":v.card.status==CardStatus::ReadError?"Desligue e confira o encaixe.":"Use o cartao em FAT32.");
    center(160,"Saves continuam no aparelho.");center(182,"O jogo funciona sem cartao.");center(204,"Troque o cartao com ele desligado.",1,UI_MUTED);
    button(14,224,212,"Verificar");button(14,272,212,"Voltar");return;
  }
  if(v.page==Page::Forge){
    center(12,"FERREIRO",2,UI_GOLD);snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(35,b,1,UI_GOLD);
    center(153,v.message,1,UI_RED);
    for(uint8_t slot=0;slot<3;++slot){int x=slot==1?124:14,y=slot==2?224:174;button(x,y,102,rpg::forgeName(slot));
      snprintf(b,sizeof(b),"+%u: +%u %s",g.forge[slot],g.forge[slot]*(slot==0?2:3),slot==0?"ATQ":slot==1?"DEF":"MP");text(x,y+43,b,1,UI_GOLD);}
    button(124,224,102,"Voltar");center(297,"Bonus permanentes por categoria.",1,UI_MUTED);return;
  }
  if(v.page==Page::Upgrade){
    uint8_t slot=v.forgeSlot;if(slot>2){center(100,"Melhoria indisponivel");return;}
    bool max=g.forge[slot]>=3;center(12,"MELHORIA DA FORJA",2,UI_GOLD);box(86,39,68,68);sprite(92,45,gear_icons[slot==0?g.p.cls:slot+3],56,56);
    snprintf(b,sizeof(b),"%s +%u%s",rpg::forgeName(slot),g.forge[slot],max?" / MAXIMO":"");center(113,b,2,UI_GOLD);
    int total=slot==0?rpg::effectiveAttack(g):slot==1?rpg::effectiveDefense(g):g.p.maxmp;
    snprintf(b,sizeof(b),"%s: %d -> %d",slot==0?"ATAQUE":slot==1?"DEFESA":"MP MAX",total,total+(max?0:slot==0?2:3));center(143,b);
    if(!max){snprintf(b,sizeof(b),"Preco: %u ouro / Nivel %u",rpg::forgePrice(g,slot),rpg::forgeRequirement(g.forge[slot]));center(166,b,1,UI_GOLD);
      snprintf(b,sizeof(b),"Ouro: %lu -> %lu",(unsigned long)g.p.gold,(unsigned long)(g.p.gold>=rpg::forgePrice(g,slot)?g.p.gold-rpg::forgePrice(g,slot):g.p.gold));center(188,b);}
    center(213,"Bonus permanente nesta categoria.");center(230,slot==2?"A melhoria nao recupera mana.":"Soma ao bonus do item equipado.",1,UI_MUTED);
    center(246,v.message,1,UI_RED);button(14,270,102,"Cancelar");if(!max)button(124,270,102,"Melhorar");return;
  }
  if(v.page==Page::GearShop||v.page==Page::GearBag){
    bool shop=v.page==Page::GearShop;uint8_t count=shop?9:rpg::gearOwnedCount(g.owned);
    center(12,shop?"EQUIPAMENTOS":"EQUIPAR ITENS",2,UI_GOLD);
    if(!count){center(89,"Sua bolsa de equipamentos");center(109,"ainda esta vazia.");center(142,"Compre itens na loja da vila.");
      button(14,226,212,"Ver loja");button(14,272,212,"Voltar");return;}
    uint8_t id=shop?rpg::gearOffer(g.p.cls,v.gearIndex%count):rpg::gearOwnedAt(g.owned,v.gearIndex%count);
    box(86,35,68,68);sprite(92,41,gear_icons[rpg::gearFamily(id)],56,56);
    center(107,rpg::gearName(id),1,UI_GOLD);
    snprintf(b,sizeof(b),"%s +%u / Nivel %u",rpg::gearStat(id),rpg::gearBonus(id),rpg::gearLevel(id));center(126,b);
    if(shop)snprintf(b,sizeof(b),"%u ouro / Voce tem %lu",rpg::gearPrice(id),(unsigned long)g.p.gold);
    else snprintf(b,sizeof(b),"%u/%u itens / %s",unsigned(v.gearIndex%count+1),count,g.equipped[rpg::gearSlot(id)]==id?"EQUIPADO":"NA BOLSA");
    center(145,b);center(162,v.message,1,UI_RED);
    button(14,180,212,shop?(rpg::gearOwns(g.owned,id)?"Ja adquirido":"Comprar"):(g.equipped[rpg::gearSlot(id)]==id?"Retirar":"Equipar"));
    button(14,226,102,"Anterior");button(124,226,102,"Proximo");button(14,272,212,"Voltar");return;
  }
  if(v.page==Page::GearBuy||v.page==Page::GearEquip){
    uint8_t id=v.itemId;if(!rpg::gearId(id)){center(100,"Item indisponivel");return;}
    bool buy=v.page==Page::GearBuy;bool remove=g.equipped[rpg::gearSlot(id)]==id;
    center(12,buy?"COMPRAR ITEM":remove?"RETIRAR ITEM":"EQUIPAR ITEM",2,UI_GOLD);
    box(86,39,68,68);sprite(92,45,gear_icons[rpg::gearFamily(id)],56,56);center(113,rpg::gearName(id),1,UI_GOLD);
    if(buy){snprintf(b,sizeof(b),"Preco: %u ouro",rpg::gearPrice(id));center(142,b,2,UI_GOLD);
      snprintf(b,sizeof(b),"Ouro: %lu -> %lu",(unsigned long)g.p.gold,(unsigned long)(g.p.gold>=rpg::gearPrice(id)?g.p.gold-rpg::gearPrice(id):g.p.gold));center(177,b);
      center(200,"O item vai para sua bolsa.");center(218,"Equipe depois para usar o bonus.");}
    else{auto preview=g;rpg::equipGear(preview,id);
      snprintf(b,sizeof(b),"ATAQUE: %d -> %d",rpg::effectiveAttack(g),rpg::effectiveAttack(preview));center(144,b);
      snprintf(b,sizeof(b),"DEFESA: %d -> %d",rpg::effectiveDefense(g),rpg::effectiveDefense(preview));center(163,b);
      snprintf(b,sizeof(b),"MP MAX: %u -> %u",g.p.maxmp,preview.p.maxmp);center(182,b);
      if(preview.p.mp<g.p.mp){snprintf(b,sizeof(b),"MP atual: %u -> %u",g.p.mp,preview.p.mp);center(207,b,1,UI_RED);center(223,"Mana acima do limite sera perdida.",1,UI_RED);}
      else center(212,remove?"O item continua na sua bolsa.":"Trocar item nao recupera mana.",1,UI_MUTED);}
    center(246,v.message,1,UI_RED);button(14,270,102,"Cancelar");button(124,270,102,buy?"Comprar":"Aplicar");return;
  }
  if(v.page==Page::Shop){
    center(12,"LOJA DE POCOES",2,UI_GOLD);box(14,62,212,70);
    snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(73,b,2,UI_GOLD);center(103,"Escolha uma pocao para comprar.");
    center(146,v.message,1,UI_GREEN);
    for(int i=0;i<2;++i){int y=172+i*46;box(14,y,212,40);snprintf(b,sizeof(b),"%s / %u ouro",i?"Mana":"Vida",rpg::potionPrice(i));center(y+5,b,2);
      snprintf(b,sizeof(b),"Na bolsa %u/99 / recupera %u%%",i?g.p.mana:g.p.life,i?50:30);center(y+26,b);}
    button(14,270,212,"Voltar");return;
  }
  if(v.page==Page::Buy){
    const bool mana=v.choice==1;uint8_t price=rpg::potionPrice(mana);center(18,"CONFIRMAR COMPRA",2,UI_GOLD);
    center(76,mana?"1 POCAO DE MANA":"1 POCAO DE VIDA",2);
    snprintf(b,sizeof(b),"Preco: %u ouro",price);center(111,b,2,UI_GOLD);
    snprintf(b,sizeof(b),"Voce tem: %lu ouro",(unsigned long)g.p.gold);center(150,b);
    snprintf(b,sizeof(b),"Depois: %lu ouro",(unsigned long)(g.p.gold>=price?g.p.gold-price:g.p.gold));center(172,b);
    center(195,"A pocao vai para sua bolsa.");center(220,v.message,1,UI_RED);
    button(14,270,102,"Cancelar");button(124,270,102,"Comprar");return;
  }
  if(v.page==Page::Result){
    c.fillRect(12,65,216,77,UI_INK);
    center(15,g.phase==rpg::Phase::Won?"VITORIA":g.phase==rpg::Phase::Lost?"DERROTA":"FUGA",2,UI_GOLD);
    if(g.phase==rpg::Phase::Won){snprintf(b,sizeof(b),"+%u XP  +%u ouro",g.gainXp,g.gainGold);center(79,b,2);
      center(110,g.dropLife?"Encontrou uma pocao de HP.":"Ruinas exploradas.");center(125,g.dropMana?"Encontrou uma pocao de MP.":"");}
    else {center(89,g.phase==rpg::Phase::Lost?"Voce retorna com 1 HP.":"Voce escapou das ruinas.");if(g.phase==rpg::Phase::Lost){snprintf(b,sizeof(b),"Penalidade: %u XP",g.gainXp);center(111,b);}}
    snprintf(b,sizeof(b),"Ruinas: %u/3 / Guardiao %s",g.ruinsWins,g.guardianDefeated?"vencido":"pendente");center(170,b);center(190,"Progresso salvo.",1,UI_GREEN);if(g.questId){snprintf(b,sizeof(b),"Contrato: %u/%u%s",g.questProgress,rpg::contract(g.questId).count,rpg::questComplete(g)?" / pronto!":"");center(218,b,1,UI_GREEN);}else center(218,"Descanse no refugio.",1,UI_MUTED);button(14,268,212,"Voltar ao refugio");return;
  }
  if(v.page==Page::Skills||v.page==Page::Bag||v.page==Page::TownBag){
    bool bag=v.page!=Page::Skills;center(14,bag?"POCOES":"HABILIDADES",2,UI_GOLD);portrait(g.p.cls,80,44);
    snprintf(b,sizeof(b),"HP %u/%u  MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);center(128,b);
    center(149,v.message,1,UI_RED);
    for(int i=0;i<2;++i){int y=172+i*46;box(14,y,212,40);
      if(bag){snprintf(b,sizeof(b),"Pocao %s x%u",i?"MP":"HP",i?g.p.mana:g.p.life);center(y+5,b,2);
        snprintf(b,sizeof(b),"Recupera %u %s",rpg::recovery(i?g.p.mp:g.p.hp,i?g.p.maxmp:g.p.maxhp,i),i?"MP":"HP");center(y+25,b);}
      else{snprintf(b,sizeof(b),"%s / %u MP",rpg::skillName(g.p.cls,i),rpg::skillCost(g.p.cls,i));center(y+5,b);
        const char* desc=i?(g.p.cls<2?"Bloqueia 75% do proximo golpe":g.p.cls==3?"Cura 25% HP / bloqueia 50%":"Bloqueia 50% do proximo golpe"):(g.p.cls==0?"Dano +80% / ignora defesa":g.p.cls==3?"Dano dobrado":g.p.cls==2?"Dano +50% / nao erra":"Dano +50%");center(y+24,desc);}}
    button(14,270,212,"Voltar");return;
  }
  const auto& foe=rpg::enemySpec(g.enemyId);center(7,foe.name,2,UI_GOLD);snprintf(b,sizeof(b),"HP %u/%u",g.enemyHp,foe.hp);center(29,b);
  c.fillRect(14,40,212,4,UI_PANEL);c.fillRect(14,40,212*g.enemyHp/foe.hp,4,UI_RED);
  personalSprite(c,g,12,46,v.heroFrame%6);
  const uint16_t* foeFrame=g.enemyId==0?sprites_goblin[frame%goblin_frames]:g.enemyId==1?sprites_wolf[frame%wolf_frames]:g.enemyId==2?sprites_skeleton[frame%skeleton_frames]:sprites_guardian[frame%guardian_frames];
  sprite(148,76,foeFrame,80,86,true);
  if(v.effect==Effect::Rage){
    // Furia: broad axe-like sweep, red trails and expanding impact. No new art RAM.
    unsigned f=std::min(7u,v.effectFrame);int reach=90+int(f)*17;
    for(int trail=0;trail<3;++trail)for(int k=0;k<55;++k){int x=reach-k,y=52+k+trail*9;if(x>=45&&x<232)c.fillRect(x,y,7,5,trail==0?0xffe0:trail==1?0xfd20:0xf800);}
    if(f>=3){int r=9+int(f-3)*7;int cx=184,cy=111;for(int k=-r;k<=r;++k){int y=r-abs(k);c.fillRect(cx+k,cy-y,3,3,0xfd20);c.fillRect(cx+k,cy+y,3,3,0xf800);}c.fillRect(178,96,12,29,0xffe0);c.fillRect(169,106,30,8,0xffe0);}
  }
  else if(v.effect!=Effect::None){const uint16_t* fx=v.effect==Effect::Lightning?sprites_lightning[v.effectFrame%lightning_frames]:v.effect==Effect::Shield?sprites_shield[v.effectFrame%shield_frames]:sprites_slash[v.effectFrame%slash_frames];if(v.effect==Effect::Lightning){for(int y=0;y<72;++y)for(int x=0;x<64;++x){uint16_t color=fx[y*64+x];if(color!=SPRITE_KEY)c.fillRect(104+x*2,18+y*2,2,2,color);}}else sprite(v.effectOnHero?22:156,82,fx,64,72);}
  c.fillRect(0,168,240,45,UI_INK);snprintf(b,sizeof(b),"HP %u/%u",g.p.hp,g.p.maxhp);text(14,171,b,2);
  snprintf(b,sizeof(b),"MP %u/%u",g.p.mp,g.p.maxmp);text(156,176,b);
  center(195,v.message,1,g.phase==rpg::Phase::Enemy?UI_GOLD:UI_WHITE);
  if(v.effect!=Effect::None){center(248,"Acao em andamento...",1,UI_GOLD);}
  else if(g.phase==rpg::Phase::Enemy){center(248,"Turno do inimigo...",1,UI_GOLD);center(277,"Aguarde",2,UI_MUTED);}
  else{button(14,220,102,"Atacar");button(124,220,102,"Tecnicas");button(14,270,102,"Pocoes");button(124,270,102,"Fugir");}
}
// Inclusive lower and exclusive upper bounds match the visible controls.
inline bool hit(int x,int y,int left,int top,int width,int height=40){return x>=left&&x<left+width&&y>=top&&y<top+height;}


