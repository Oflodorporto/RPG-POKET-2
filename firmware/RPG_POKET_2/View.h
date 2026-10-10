#pragma once
#include "Rules.h"
#include "Camp.h"
#include "BackgroundArt.h"
#include "SpriteArt.h"
#include "RegionalArt.h"
#include "WorldEnemyArt.h"
#include "HeroArt.h"
#include "PersonalArt.h"
#include "ClubUi.h"
#include "GearArt.h"
#include "CombatFx.h"
#include "CardCheck.h"
#include <stdio.h>
#include <string.h>
enum class Page { Choose, Help, Home, Battle, Skills, Bag, Result, SaveError, Blocked, Village, Shop, Buy, TownBag, Character, Map, Market, Inventory, GearShop, GearBag, GearBuy, GearEquip, Forge, Upgrade, Tavern, Contract, QuestConfirm, Card, Menu, Slots, SlotConfirm, DeleteSlot, Settings, Wifi, Keyboard, Tests, Travel, Ruins, Guild, GuildJoin, GuildMissions, Race, Clothes, Club, ClubBattle, ClubResult, Updates, TravelRoll, CityGoods, GoodsBuy, Explore, ForgetWifi, Clock, NetworkTest, DungeonEntry, Dungeon, DungeonMenu, CrystalBuy, DungeonExit, DungeonVictory, BagGear, DungeonLoot, CampSetup, CampRoll, CampRest, CampKit, GearSell, Prologue, Journal, People, Dialogue, Continent, TimeSettings, TimeEdit, Letters, Letter, LetterRefuse, EventTravel, EventResult, Title, Campaign, Evolution, Powers, OathConfirm, TravelConfirm, Recovery, Guide, CampaignTask, CampaignResult, Discovery, Scrap, ChestLock, Lockpicks, Progression, AttributeInfo, EnemyInfo, Bestiary, ContributionResult, Epilogue, IslandEntry };
inline const Backdrop& backdropFor(Page page,const rpg::Game& g){
  switch(page){
  case Page::IslandEntry:return bg_explore1;
  case Page::Prologue:return bg_tavern;case Page::Journal:return bg_character;case Page::People:case Page::Dialogue:return bg_village;case Page::Continent:return bg_world;
  case Page::Guild:return bg_guild;case Page::GuildJoin:return bg_guildjoin;case Page::GuildMissions:return bg_missions;case Page::Race:return bg_race;case Page::Clothes:return bg_clothes;case Page::Club:return bg_club;case Page::ClubBattle:return bg_clubbattle;case Page::ClubResult:return bg_clubresult;
  case Page::EventTravel:return bg_world;case Page::Letters:case Page::Letter:case Page::LetterRefuse:case Page::EventResult:return bg_guild;
  case Page::TimeSettings:case Page::TimeEdit:case Page::NetworkTest:case Page::Clock:case Page::Menu:return bg_menu;case Page::Slots:return bg_slots;case Page::SlotConfirm:return bg_slotconfirm;case Page::DeleteSlot:return bg_delete;case Page::Settings:return bg_settings;case Page::Wifi:case Page::ForgetWifi:return bg_wifi;case Page::Keyboard:return bg_keyboard;case Page::Tests:return bg_tests;case Page::Travel:return bg_world;case Page::Ruins:return bg_map;
  case Page::TravelRoll:return bg_world;case Page::CityGoods:return bg_goods;case Page::GoodsBuy:return bg_goodsbuy;case Page::Explore:return g.city==0?bg_explore0:g.city==1?bg_explore1:g.city==2?bg_explore2:bg_explore3;
  case Page::CampSetup:case Page::CampRoll:case Page::CampRest:return g.city==0?bg_explore0:g.city==1?bg_explore1:g.city==2?bg_explore2:bg_explore3;case Page::CampKit:case Page::GearSell:case Page::DungeonLoot:case Page::DungeonVictory:case Page::BagGear:case Page::DungeonEntry:case Page::CrystalBuy:case Page::DungeonExit:case Page::DungeonMenu:return bg_slotconfirm;case Page::Dungeon:return bg_explore1;
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
  case Page::Battle:if(g.eventStage==2)return g.eventTier==0?bg_explore0:g.eventTier==1?bg_explore1:g.eventTier==2?bg_explore2:bg_explore3;if(g.enemyId>=14)return g.city==0?bg_explore0:g.city==1?bg_explore1:g.city==2?bg_explore2:bg_explore3;if(g.enemyId>=4)return g.enemyId==4?bg_explore0:g.enemyId==5?bg_explore1:g.enemyId==6?bg_explore2:bg_explore3;return g.enemyId==0?bg_battle0:g.enemyId==1?bg_battle1:g.enemyId==2?bg_battle2:bg_battle3;
  }return bg_camp;
}
struct ViewState {Page page=Page::Choose,picksReturn=Page::TownBag,bagReturn=Page::Inventory,objectiveReturn=Page::Menu,powersReturn=Page::Evolution;Page enemyReturn=Page::Battle;Page guideReturn=Page::Menu;bool paperAnimate=false;unsigned paperStartFrame=0;bool dialogueAnimate=false;unsigned dialogueStartFrame=0;uint8_t dialoguePage=0;uint8_t bestiaryIndex=0;uint8_t guideIndex=0,tripDestination=0,campaignChoice=0,campaignScene=0;uint8_t evolutionLevel=1,powerIndex=0,attributeIndex=0;uint8_t choice=0,gearIndex=0,itemId=0,forgeSlot=0,questChoice=1,questAction=0;const char* message="";bool recovered=false;Effect effect=Effect::None;unsigned effectFrame=0,heroFrame=0,campProgress=0;bool effectOnHero=false,touchFeedback=false;CardInfo card;};
constexpr uint16_t UI_INK=0x1083,UI_PANEL=0x1926,UI_GOLD=0xd5aa,UI_WHITE=0xef1b,UI_MUTED=0x9cf4,UI_GREEN=0x6e0c,UI_RED=0xe28b,UI_BLUE=0x549f;
#include "StoryPaper.h"
#include "MenuView.h"
#include "NpcArt.h"
#include "EventView.h"
#include "HippogriffArt.h"
#include "MagicView.h"
#include "DungeonView.h"
#include "BagView.h"
#include "NarrativeView.h"
#include "TitleView.h"
#include "ScenicView.h"
#include "PanelView.h"
#include "CampView.h"
#include "EvolutionView.h"
#include "PowersView.h"
#include "ProgressionView.h"
#include "EnemyInfoView.h"
#include "BestiaryView.h"
#include "LaunchView.h"
#include "ExplorationView.h"
#include "LocksView.h"
// Rendering shared by device and native screenshot test. No gameplay mutation.
template<class Canvas> struct WifiOverlay {
  Canvas& c;bool connected;uint8_t bars;
  ~WifiOverlay(){if(!connected)return;const uint16_t green=0x07e0;c.fillRect(221,1,19,19,UI_INK);
    for(unsigned i=0;i<4;++i)c.fillRect(223+i*4,17-(i+1)*4,3,(i+1)*4,i<bars?green:0x3186);}
};
template<class Canvas> void render(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame=0){
  WifiOverlay<Canvas> wifiOverlay{c,menu.connected&&v.page!=Page::Clock,menu.signalBars};
  if(v.page==Page::ChestLock||v.page==Page::Lockpicks){drawLocks(c,g,v);return;}
  if(v.page==Page::Discovery||v.page==Page::Scrap){drawDiscovery(c,g,v,frame);return;}
  if(v.page==Page::TravelConfirm||v.page==Page::Recovery||v.page==Page::Guide){drawLaunch(c,g,v);return;}
  auto text=[&](int x,int y,const char* s,int size=1,uint16_t color=UI_WHITE){c.setTextColor(storyPaperPage(v.page)&&y<262?storyInk(color):color);c.setTextSize(size);c.setCursor(x,y);c.print(s);};
  auto center=[&](int y,const char* s,int size=1,uint16_t color=UI_WHITE){text((240-int(strlen(s))*6*size)/2,y,s,size,color);};
  auto box=[&](int x,int y,int w,int h){c.fillRect(x,y,w,h,UI_PANEL);c.drawRect(x,y,w,h,UI_GOLD);c.drawRect(x+2,y+2,w-4,h-4,0x3186);};
  auto button=[&](int x,int y,int w,const char* s){box(x,y,w,40);c.setTextColor(UI_WHITE);int size=int(strlen(s))*12<=w-6?2:1;c.setTextSize(size);c.setCursor(x+(w-int(strlen(s))*6*size)/2,y+(40-8*size)/2);c.print(s);};
  auto sprite=[&](int x,int y,const uint16_t* pixels,int w,int h,bool outline=false){
    if(outline)for(int row=0;row<h;++row){int col=0;while(col<w){while(col<w&&pixels[row*w+col]==SPRITE_KEY)++col;int start=col;while(col<w&&pixels[row*w+col]!=SPRITE_KEY)++col;if(col>start)c.fillRect(x+start-1,y+row-1,col-start+2,3,UI_MUTED);}}
    for(int row=0;row<h;++row){int col=0;while(col<w){while(col<w&&pixels[row*w+col]==SPRITE_KEY)++col;int start=col;while(col<w&&pixels[row*w+col]!=SPRITE_KEY)++col;if(col>start)c.draw16bitRGBBitmap(x+start,y+row,const_cast<uint16_t*>(pixels+row*w+start),col-start,1);}}
  };
  auto portrait=[&](uint8_t cls,int x,int y,const rpg::Game* overrideGame=nullptr){box(x-1,y-1,82,68);auto who=overrideGame?*overrideGame:g;who.p.cls=cls;personalSprite(c,who,x,y,6);c.drawRect(x-1,y-1,82,68,UI_GOLD);};
  if(v.page==Page::Title){drawTitle(c,frame);return;}
  if(v.page==Page::Menu){drawScenicMenu(c,g,v,frame);return;}
  if(v.page==Page::Home){drawScenicHome(c,g,v,frame);return;}
  if(v.page==Page::Map){drawScenicMap(c,g,v,frame);return;}
  if(v.page==Page::Ruins){drawScenicRuins(c,g,v,frame);return;}
  if(v.page==Page::Skills){drawPanelSkills(c,g,v);return;}
  if(v.page==Page::Bestiary){drawBestiary(c,g,v);return;}
  if(v.page==Page::EnemyInfo){drawEnemyInfo(c,g,v);return;}
  if(v.page==Page::Powers||v.page==Page::OathConfirm){drawPowers(c,g,v);return;}
  if(v.page==Page::Progression||v.page==Page::AttributeInfo){drawProgression(c,g,v);return;}
  if(v.page==Page::Evolution){drawEvolution(c,g,v);return;}
  if(v.page==Page::Bag||v.page==Page::TownBag){drawPanelBag(c,g,v);return;}
  if(v.page==Page::Character){drawPanelCharacter(c,g);return;}
  if(v.page==Page::Village&&g.city==0){drawPanelVillage(c,g,v);return;}
  if(v.page==Page::Prologue||v.page==Page::Journal||v.page==Page::People||v.page==Page::Dialogue||v.page==Page::Continent||v.page==Page::Campaign||v.page==Page::CampaignTask||v.page==Page::CampaignResult||v.page==Page::ContributionResult||v.page==Page::Epilogue){drawNarrative(c,g,v,frame);return;}
  if(v.page==Page::CampSetup||v.page==Page::CampRoll||v.page==Page::CampRest){drawCamp(c,g,v,frame);return;}
  if(v.page==Page::Dungeon){drawDungeon(c,g,v);return;}
  if(v.page==Page::Bag||v.page==Page::TownBag||v.page==Page::BagGear){drawBag(c,g,v);return;}
  bool outside=v.page==Page::Map||v.page==Page::Travel||v.page==Page::EventTravel||v.page==Page::TravelRoll||v.page==Page::Home||v.page==Page::Village||v.page==Page::Ruins||v.page==Page::Explore||v.page==Page::CampSetup||v.page==Page::CampRoll||v.page==Page::CampRest||v.page==Page::Battle;
  backdropPeriod=outside&&menu.clockValid&&menu.dayCycle?menu.worldPeriod:worldClock::Period::Day;
  char b[64];if(v.page==Page::Travel||v.page==Page::TravelRoll)scenicWorld(c,false,backdropPeriod);else if(v.page==Page::Battle){PANEL_IMAGE(c,battle);if(g.city==1)panelBattleArena(c);if(g.city!=1){drawBackdrop(c,backdropFor(v.page,g));panelImage(c,panelArt::battlePalette,panelArt::battleAsset,0,66);panelImage(c,panelArt::battlePalette,panelArt::battleAsset,192,320);}}else if(v.page!=Page::Clock)drawBackdrop(c,backdropFor(v.page,g));c.setTextWrap(false);if(v.touchFeedback)c.fillRect(232,3,5,5,UI_GREEN);
  if(storyPaperPage(v.page)&&!drawStoryPaper(c,v,frame))return;
  if(renderEvent(c,g,int(v.page),text,center,box,button,frame))return;
  if(renderMenu(c,g,int(v.page),text,center,box,button,portrait,frame))return;
  if(v.page==Page::CampKit||v.page==Page::GearSell){bool selling=v.page==Page::GearSell;if(selling&&!rpg::gearId(v.itemId)){center(100,"Item indisponivel");button(14,272,102,"Voltar");return;}
    center(16,selling?"VENDER EQUIPAMENTO":"KIT DE ACAMPAMENTO",2,UI_GOLD);
    if(selling){box(86,55,68,68);sprite(92,61,gear_icons[rpg::gearFamily(v.itemId)],56,56);center(146,rpg::gearName(v.itemId));snprintf(b,sizeof(b),"Receber: %u ouro",rpg::sellPrice(g,v.itemId));center(174,b,2,UI_GOLD);center(207,"O item sera retirado da bolsa.");}
    else {magicSprite(c,campArt::kit,48,48,84,55,72,72);center(146,"Preco: 80 ouro",2,UI_GOLD);center(180,"Reutilizavel / +2 no teste");center(204,"Saco de dormir ao lado do heroi.");}
    center(241,v.message,1,UI_RED);button(14,272,102,"Voltar");button(124,272,102,selling?"Vender":"Comprar");return;
  }
  if(v.page==Page::IslandEntry){
    center(21,"DIANTE DO DESCONHECIDO",1,UI_GOLD);
    const char* speech="Voce encontra pedras tomadas pelo sal. Um frio atravessa sua roupa. Do interior vem um som que voce nao reconhece. A passagem espera.";
    story::wrapStory(speech,18,[&](unsigned row,const char* line){c.setTextSize(2);c.setTextColor(STORY_INK);c.setCursor(12,58+row*17);c.print(line);});
    if(*v.message)center(248,v.message,1,UI_RED);
    button(14,272,102,"Recuar");button(124,272,102,"Entrar");return;
  }
  if(v.page==Page::DungeonLoot){
    center(18,"BAU ABERTO",2,UI_GOLD);box(86,55,68,68);if(v.itemId)sprite(92,61,gear_icons[rpg::gearFamily(v.itemId)],56,56);else magicSprite(c,dungeonArt::props[1],32,32,92,61,56,56);
    center(143,v.message,1,UI_GREEN);center(170,v.itemId?"Guardado em Bolsa > Equipamentos":"O ouro foi somado a sua bolsa.");
    center(186,"Equipe fora de um combate.");
    button(14,218,212,"Ver na bolsa");button(14,272,212,"Explorar");return;
  }
  if(v.page==Page::DungeonVictory){
    center(18,rpg::islandDungeon(g)?"THALVOR VENCIDO!":"VAELOR DERROTADO!",2,UI_GOLD);
    center(76,"A cripta foi conquistada.");
    snprintf(b,sizeof(b),"+%u XP / +%u ouro",g.gainXp,g.gainGold);center(108,b,2,UI_GREEN);
    center(148,rpg::islandDungeon(g)?"O tesouro do santuario esta livre.":"O Livro das Vigilias foi revelado.");
    center(174,"Continue para buscar o saque");center(190,"ou saia com suas recompensas.");
    center(202,rpg::islandDungeon(g)?"Vitoria preservada no personagem.":"Descoberta registrada no Diario.",1,UI_GREEN);
    button(14,218,212,"Explorar");button(14,272,212,"Sair");return;
  }
  if(v.page==Page::DungeonEntry||v.page==Page::CrystalBuy||v.page==Page::DungeonExit){
    bool buy=v.page==Page::CrystalBuy,exit=v.page==Page::DungeonExit;
    center(16,buy?"CRISTAL DA CRIPTA":exit?"SAIR DA CRIPTA?":"CRIPTA DO ARCONTE",2,UI_GOLD);
    snprintf(b,sizeof(b),"Cristais: %u / ouro: %lu",g.crystals,(unsigned long)g.p.gold);center(64,b);
    center(98,buy?"Preco: 300 ouro":exit?"Seu saque sera preservado.":"Entrada: consome 1 cristal");
    center(124,buy?"Guardiao tambem deixa cristal.":exit?(rpg::islandDungeon(g)?"Voce voltara ao mapa.":"Nova entrada custa outro cristal."):"2 andares + sala do chefe");
    center(152,buy?"Compra limitada a 9 cristais.":exit?"O labirinto sera reiniciado.":"Nivel recomendado: 8+");
    center(182,exit?"Confirme somente para sair.":"Setas: mover / girar / andar de lado");
    center(206,exit?"": "Toque na cena: interagir / atacar");center(237,v.message,1,UI_RED);
    button(14,272,102,"Voltar");button(124,272,102,buy?"Comprar":exit?"Sair":"Entrar");return;
  }
  if(v.page==Page::DungeonMenu){center(14,g.phase==rpg::Phase::Hero?"FICHA DO INIMIGO >":"MENU DA CRIPTA",2,UI_GOLD);snprintf(b,sizeof(b),"Vida: %u / Mana: %u",g.p.life,g.p.mana);center(46,b);center(64,v.message,1,UI_RED);if(g.dndProgression){box(14,40,212,34);center(52,"Poderes de classe >",1,UI_GOLD);}
    button(14,86,102,"Tecnica");button(124,86,102,"Defesa");button(14,134,212,"Bolsa");button(14,182,212,"Fugir do inimigo");button(14,230,212,"Sair da dungeon");button(14,278,212,"Voltar");return;}
  if(v.page==Page::SaveError||v.page==Page::Blocked){
    center(30,"RPG POKET 2.0",2,UI_GOLD);center(88,"PROGRESSO PROTEGIDO",1,UI_RED);
    center(119,v.page==Page::SaveError?"Nao foi possivel salvar.":"Save indisponivel ou incompativel.");
    center(140,"Nenhum save foi apagado.");
    center(163,v.page==Page::SaveError?"Mantenha o aparelho ligado.":"Informe esta tela ao desenvolvedor.");
    if(v.page==Page::SaveError)button(14,250,212,"Tentar salvar");else button(14,250,212,"Menu");return;
  }
  if(v.page==Page::Club){center(12,"CLUBE DA LUTA",2,UI_GOLD);npcPortrait(c,story::Npc::Grum,14,29,24);text(44,37,"Grum Pedrafranca / 25 ouro");
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
  if(v.page==Page::Guild||v.page==Page::GuildJoin){center(12,"GUILDA",2,UI_GOLD);center(43,"DOS AVENTUREIROS",2,UI_GOLD);snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(68,b);
    npcPortrait(c,story::Npc::Maelis,14,84,48);text(76,88,"Maelis Voss",1,UI_GOLD);text(76,106,"Mestra da Guilda");center(141,g.guildMember?"Membro cadastrado":"Cadastro: 100 ouro / uma vez");center(156,v.message,1,UI_RED);
    if(v.page==Page::GuildJoin){center(182,"Pagar 100 ouro para se cadastrar?");button(14,272,102,"Cancelar");button(124,272,102,"Pagar");}
    else{button(14,170,212,g.guildMember?"Missoes":"Cadastrar");button(14,218,212,"Clube da luta");button(14,272,212,"Voltar");}return;}
  if(v.page==Page::Tavern){center(12,"TAVERNA",2,UI_GOLD);npcPortrait(c,story::Npc::Nara,14,45,48);text(76,55,"Nara Veld",1,UI_GOLD);text(76,75,"Dona do Refugio");center(112,"Entre. A fogueira ainda esta acesa.");center(140,"Maelis cuida dos contratos na Guilda.");center(167,v.message,1,UI_GREEN);button(14,220,212,"Ir a guilda");button(14,272,212,"Voltar");return;}
  if(v.page==Page::Choose){
    for(int y=8;y<26;y+=7)c.fillRect(8,y,20,3,UI_GOLD);center(12,"RPG POKET 2.0",2,UI_GOLD);center(38,"ESCOLHA SUA CLASSE");auto draft=g;draft.race=menu.draftRace;portrait(v.choice,80,57,&draft);
    center(136,rpg::className(v.choice),2);auto p=rpg::create(v.choice,1).p;
    snprintf(b,sizeof(b),"HP %u  MP %u  ATQ %u  DEF %u",p.hp,p.mp,p.atk,p.def);center(161,b);
    const char* motto[]={"Magia e estrategia","Protecao e coragem","Precisao em cada golpe","Forca e resistencia"};center(180,motto[v.choice],1,UI_MUTED);
    button(14,211,102,"< Voltar");button(124,211,102,"Proxima>");button(14,263,212,"Personalizar");return;
  }
  if(v.page==Page::Help){
    center(14,"NARA VELD / GUIA",2,UI_GOLD);center(43,"Refugio das Brasas / Carvalho");
    c.fillRect(10,64,220,82,UI_INK);text(19,75,"Um toque, uma acao.",1);
    text(19,92,"Ataque e aguarde o inimigo.");text(19,109,"Habilidades consomem mana.");text(19,126,"Pocoes tambem usam um turno.");
    center(162,"Carvalho: explore ate nivel 5.");center(184,"Depois: mapa > Ruinas > Guardiao.");center(205,"O progresso e salvo a cada turno.");
    scenicPanel(c,14,233,212,27);panelLabel(c,18,242,204,"Abrir guia completo >",UI_GOLD);
    center(216,v.recovered?"Checkpoint anterior recuperado.":"Reiniciar retoma sua aventura.",1,UI_MUTED);
    button(14,268,212,"Continuar");return;
  }
  if(v.page==Page::CityGoods||v.page==Page::GoodsBuy){bool buy=v.page==Page::GoodsBuy;uint8_t id=rpg::localGood(g.city);auto preview=g;
    center(12,buy?"CONFIRMAR COMPRA":"SUPRIMENTOS",2,UI_GOLD);center(48,story::supplier(g.city),1,UI_GOLD);center(88,rpg::goodName(id),2);center(124,rpg::goodBonus(id));
    snprintf(b,sizeof(b),"Preco %u ouro / tem %u de 9",rpg::goodPrice(g.city),rpg::goodCount(preview,id));center(156,b,1,UI_GOLD);
    if(buy){center(182,"Uma unidade usada por viagem.");center(200,"Cada tipo soma seu bonus ao dado.");}else {box(14,181,212,30);center(191,"Gazuas / ferramentas",1,UI_GOLD);}center(170,v.message,1,UI_RED);
    if(buy){button(14,270,102,"Cancelar");button(124,270,102,"Comprar");}else {if(g.city==1){button(14,218,102,"Comprar");button(124,218,102,"Cristal");}else button(14,218,212,"Comprar");button(14,270,102,"Voltar");button(124,270,102,"Kit");}return;}
  if(v.page==Page::Explore){center(12,placeName(g.city),2,UI_GOLD);center(56,rpg::citySpecialty(g.city));snprintf(b,sizeof(b),"Nivel recomendado: %u+",rpg::cityLevel(g.city));center(90,b,1,UI_GOLD);
    center(123,g.p.level<rpg::cityLevel(g.city)?"PERIGO: acima do seu nivel!":"Prepare equipamento e pocoes.",1,g.p.level<rpg::cityLevel(g.city)?UI_RED:UI_WHITE);center(160,v.message,1,UI_RED);
    center(195,"Combate, achados e surpresas.");button(14,220,212,"Explorar");button(14,270,102,"Acampar");button(124,270,102,"Voltar");return;}
  if(v.page==Page::Village){
    center(12,placeName(g.city),2,UI_GOLD);snprintf(b,sizeof(b),"%lu OURO",(unsigned long)g.p.gold);center(65,b,1,UI_GOLD);box(14,78,212,26);center(86,"Conversar / pessoas",1,UI_GOLD);snprintf(b,sizeof(b),"Exploracao: nivel %u+",rpg::cityLevel(g.city));center(49,b,1,UI_MUTED);center(109,v.message,1,UI_GREEN);
    button(14,126,102,"Loja");button(124,126,102,"Bolsa");button(14,170,102,"Heroi");button(124,170,102,"Explorar");
    button(14,214,102,"Guilda");button(124,214,102,"Acampar");button(14,258,102,"Mapa");button(124,258,102,"Menu");return;
  }
  if(v.page==Page::GuildMissions){
    center(12,"MISSOES",2,UI_GOLD);center(43,"Contratos de Maelis Voss",1,UI_GOLD);center(109,v.message,1,UI_GREEN);
    center(69,g.questId?"CONTRATO ATIVO":"ESCOLHA UM CONTRATO",1,UI_GOLD);
    if(g.questId){snprintf(b,sizeof(b),"%s: %u/%u",rpg::contract(g.questId).name,g.questProgress,rpg::contract(g.questId).count);center(91,b);}
    else center(91,placeName(g.city));
    for(unsigned slot=0;slot<3;++slot){uint8_t id=rpg::contractVisibleOffer(g,slot);int y=124+slot*44;box(14,y,212,40);center(y+6,rpg::contract(id).name,1,UI_GOLD);
      if(g.questId==id)snprintf(b,sizeof(b),"%u/%u / %s",g.questProgress,rpg::contract(id).count,rpg::questComplete(g)?"RECEBER":"EM ANDAMENTO");
      else if(g.p.level<rpg::contractLevel(id))snprintf(b,sizeof(b),"Disponivel no nivel %u",rpg::contractLevel(id));else snprintf(b,sizeof(b),"%u XP / %u ouro",rpg::contractXp(g,id,g.p.level),rpg::contractGold(id,g.p.level));center(y+24,b);}
    button(14,272,212,"Voltar");return;
  }
  if(v.page==Page::Contract){
    uint8_t id=v.questChoice;if(id<1||id>15){center(100,"Contrato indisponivel");return;}
    bool active=g.questId==id;uint8_t level=active?g.questLevel:g.p.level;
    center(12,"CONTRATO",2,UI_GOLD);center(36,rpg::contractAgent(id));center(57,rpg::contract(id).name,1,UI_GOLD);
    for(unsigned line=0;line<3;++line)center(80+line*13,rpg::contractStory(id,line));
    center(127,rpg::contract(id).objective);snprintf(b,sizeof(b),"Origem: %s / Nv %u+",placeName(rpg::contractCity(id)),rpg::contractLevel(id));center(145,b);
    snprintf(b,sizeof(b),"Progresso %u/%u",active?g.questProgress:0,rpg::contract(id).count);center(162,b);
    snprintf(b,sizeof(b),"%u XP / %u ouro",rpg::contractXp(g,id,level),rpg::contractGold(id,level));center(180,b,1,UI_GOLD);center(201,v.message,1,UI_RED);
    button(14,220,212,active?(rpg::questComplete(g)?"Receber":"Abandonar"):"Aceitar");button(14,272,212,"Voltar");return;
  }
  if(v.page==Page::QuestConfirm){
    uint8_t id=v.questChoice,action=v.questAction;if(id<1||id>15||action>2){center(100,"Contrato indisponivel");return;}
    uint8_t level=action==0?g.p.level:g.questLevel;
    center(12,action==0?"ACEITAR MISSAO":action==1?"RECEBER PREMIO":"ABANDONAR MISSAO",2,UI_GOLD);
    center(72,rpg::contract(id).name,1);center(105,rpg::contract(id).objective);
    if(action==2){center(146,"O progresso sera perdido.",1,UI_RED);center(176,"Voce nao recebera recompensa.");center(205,"Podera aceitar novamente do zero.");}
    else{snprintf(b,sizeof(b),"%u XP / %u ouro",rpg::contractXp(g,id,level),rpg::contractGold(id,level));center(148,b,2,UI_GOLD);
      if(action==0){center(181,"O objetivo vale a partir de agora.");center(207,"Volte aqui para receber depois.");}
      else{snprintf(b,sizeof(b),"Ouro: %lu -> %lu",(unsigned long)g.p.gold,(unsigned long)(g.p.gold+rpg::contractGold(id,level)));center(184,b);
        auto preview=g;rpg::claimQuest(preview);snprintf(b,sizeof(b),"Nivel: %u -> %u",g.p.level,preview.p.level);center(207,b);}}
    center(245,v.message,1,UI_RED);button(14,270,102,"Cancelar");button(124,270,102,action==0?"Aceitar":action==1?"Receber":"Desistir");return;
  }
  if(v.page==Page::Character){
    center(12,"PERSONAGEM",2,UI_GOLD);portrait(g.p.cls,80,40);center(117,rpg::className(g.p.cls),2);
    snprintf(b,sizeof(b),"Nivel %u   XP %lu/%u",g.p.level,(unsigned long)g.p.xp,rpg::xpNeeded(g));center(143,b);
    snprintf(b,sizeof(b),"HP %u/%u  MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);center(162,b);
    snprintf(b,sizeof(b),"ATAQUE %u  DEFESA %u",unsigned(rpg::effectiveAttack(g)),unsigned(rpg::effectiveDefense(g)));center(181,b);
    snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(200,b,1,UI_GOLD);
    snprintf(b,sizeof(b),"Pocoes: HP x%u / MP x%u",g.p.life,g.p.mana);center(219,b);
    snprintf(b,sizeof(b),"Sobrevivencia %u / Sorte %u",rpg::survival(g),rpg::luck(g));center(241,b,1,UI_GREEN);button(14,270,212,"Voltar");return;
  }
  if(v.page==Page::Market||v.page==Page::Inventory){
    bool shop=v.page==Page::Market;center(12,shop?"LOJA DA CIDADE":"SUA BOLSA",2,UI_GOLD);
    portrait(g.p.cls,80,shop?38:48);snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(shop?114:136,b,1,UI_GOLD);
    if(shop){button(14,138,212,"Pocoes");button(14,182,212,"Equipamentos");button(14,226,102,"Forja");button(124,226,102,"Viagem");button(14,270,212,"Voltar");}
    else{snprintf(b,sizeof(b),"Equipamentos: %u",rpg::gearOwnedCount(g.owned));center(155,b);snprintf(b,sizeof(b),"Racoes %u / Mapas %u / Sorte %u",g.rations,g.charts,g.charms);center(169,b,1,UI_MUTED);
      button(14,180,212,"Abrir bolsa");button(14,226,212,"Equipamentos");button(14,272,102,"Menu");button(124,272,102,"Voltar");}return;
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
    center(12,story::smith(g.city),1,UI_GOLD);snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(35,b,1,UI_GOLD);
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
    bool shop=v.page==Page::GearShop;uint8_t count=shop?rpg::cityGearCount(g):rpg::gearOwnedCount(g.owned);
    center(12,shop?"EQUIPAMENTOS":"EQUIPAR ITENS",2,UI_GOLD);
    if(!count){center(89,"Sua bolsa de equipamentos");center(109,"ainda esta vazia.");center(142,"Compre itens na loja da vila.");
      button(14,226,212,"Ver loja");button(14,272,212,"Voltar");return;}
    uint8_t id=shop?rpg::cityGearOffer(g,v.gearIndex%count):rpg::gearOwnedAt(g.owned,v.gearIndex%count);
    box(86,35,68,68);sprite(92,41,gear_icons[rpg::gearFamily(id)],56,56);
    center(107,rpg::gearName(id),1,UI_GOLD);
    snprintf(b,sizeof(b),"%s +%u / Nivel %u",rpg::gearStat(id),rpg::gearBonus(id),rpg::gearLevel(id));center(126,b);
    if(shop)snprintf(b,sizeof(b),"%u ouro / Voce tem %lu",rpg::cityGearPrice(g,id),(unsigned long)g.p.gold);
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
    if(buy){snprintf(b,sizeof(b),"Preco: %u ouro",rpg::cityGearPrice(g,id));center(142,b,2,UI_GOLD);
      snprintf(b,sizeof(b),"Ouro: %lu -> %lu",(unsigned long)g.p.gold,(unsigned long)(g.p.gold>=rpg::cityGearPrice(g,id)?g.p.gold-rpg::cityGearPrice(g,id):g.p.gold));center(177,b);
      auto preview=rpg::gearPreview(g,id);
      snprintf(b,sizeof(b),"ATQ: %d -> %d",rpg::effectiveAttack(g),rpg::effectiveAttack(preview));center(196,b);
      snprintf(b,sizeof(b),"DEF: %d -> %d",rpg::effectiveDefense(g),rpg::effectiveDefense(preview));center(211,b);
      snprintf(b,sizeof(b),"MP MAX: %u -> %u",g.p.maxmp,preview.p.maxmp);center(226,b);if(!*v.message)center(246,"Na bolsa: equipe para usar.",1,UI_MUTED);}
    else{auto preview=g;rpg::equipGear(preview,id);
      snprintf(b,sizeof(b),"ATAQUE: %d -> %d",rpg::effectiveAttack(g),rpg::effectiveAttack(preview));center(144,b);
      snprintf(b,sizeof(b),"DEFESA: %d -> %d",rpg::effectiveDefense(g),rpg::effectiveDefense(preview));center(163,b);
      snprintf(b,sizeof(b),"MP MAX: %u -> %u",g.p.maxmp,preview.p.maxmp);center(182,b);
      if(preview.p.mp<g.p.mp){snprintf(b,sizeof(b),"MP atual: %u -> %u",g.p.mp,preview.p.mp);center(207,b,1,UI_RED);center(223,"Mana acima do limite sera perdida.",1,UI_RED);}
      else center(212,remove?"O item continua na sua bolsa.":"Trocar item nao recupera mana.",1,UI_MUTED);}
    center(246,v.message,1,UI_RED);button(14,270,102,"Cancelar");button(124,270,102,buy?"Comprar":"Aplicar");return;
  }
  if(v.page==Page::Shop){
    center(12,"LOJA DE POCOES",2,UI_GOLD);center(41,story::supplier(g.city),1,UI_GOLD);box(14,62,212,70);
    snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);center(73,b,2,UI_GOLD);center(103,"Escolha uma pocao para comprar.");
    center(146,v.message,1,UI_GREEN);
    for(int i=0;i<2;++i){int y=172+i*46;box(14,y,212,40);snprintf(b,sizeof(b),"%s / %u ouro",i?"Mana":"Vida",rpg::potionPrice(g,i));center(y+5,b,2);
      snprintf(b,sizeof(b),"Na bolsa %u/99 / recupera %u%%",i?g.p.mana:g.p.life,i?50:30);center(y+26,b);}
    button(14,270,212,"Voltar");return;
  }
  if(v.page==Page::Buy){
    const bool mana=v.choice==1;uint8_t price=rpg::potionPrice(g,mana);center(18,"CONFIRMAR COMPRA",2,UI_GOLD);
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
      center(110,g.dropLife?"Encontrou uma pocao de HP.":"Encontro vencido.");center(125,g.dropMana?"Encontrou uma pocao de MP.":"");}
    else {center(89,g.phase==rpg::Phase::Lost?"Voce retorna com 1 HP.":"Voce escapou do encontro.");if(g.phase==rpg::Phase::Lost){snprintf(b,sizeof(b),"Penalidade: %u XP",g.gainXp);center(111,b);}}
    snprintf(b,sizeof(b),"Nv %u / XP %lu/%u",g.p.level,(unsigned long)g.p.xp,rpg::xpNeeded(g));center(151,b,1,UI_GOLD);panelBar(c,25,165,190,g.p.xp,rpg::xpNeeded(g),UI_GREEN);
    auto goal=story::objective(g);center(183,goal.title,1,UI_GOLD);center(198,"Progresso salvo.",1,UI_GREEN);if(g.questId){snprintf(b,sizeof(b),"Contrato: %u/%u%s",g.questProgress,rpg::contract(g.questId).count,rpg::questComplete(g)?" / pronto!":"");center(218,b,1,UI_GREEN);}else center(218,g.phase==rpg::Phase::Lost?"Prepare-se antes de tentar de novo.":"Retorne ao local de origem.",1,UI_MUTED);button(14,268,212,g.phase==rpg::Phase::Lost?"Recuperar forcas":g.campStage==2&&g.phase==rpg::Phase::Won?"Descansar":"Voltar a explorar");return;
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
  const auto& foe=rpg::enemySpec(g.enemyId);
  // A single framed panel replaces the concept's baked-in enemy name/level.
  scenicPanel(c,42,24,156,62);
  panelLabel(c,50,34,140,foe.name,UI_GOLD,2);char health[48];snprintf(health,sizeof(health),"HP %u/%u",g.enemyHp,rpg::encounterHp(g,g.enemyId));panelLabel(c,78,66,85,health);panelBar(c,56,59,128,g.enemyHp,rpg::encounterHp(g,g.enemyId),UI_RED);panelLabel(c,75,78,90,"Ficha >",UI_GOLD);
  personalSprite(c,g,8,77,v.heroFrame%6);
  const uint16_t* foeFrame=g.enemyId>=10?hippogriffArt::frames[(v.effectOnHero&&v.effect==Effect::Slash)?2+v.effectFrame%2:frame%2]:g.enemyId>=4?regionEnemyFrame(g.enemyId,(v.effectOnHero&&v.effect==Effect::Slash)?2+v.effectFrame%2:frame%2):g.enemyId==0?sprites_goblin[frame%goblin_frames]:g.enemyId==1?sprites_wolf[frame%wolf_frames]:g.enemyId==2?sprites_skeleton[frame%skeleton_frames]:sprites_guardian[frame%guardian_frames];
  if(g.enemyId>=25)magicSprite(c,worldEnemyFrame(g.enemyId,v.effectOnHero&&v.effect!=Effect::None?1:0),64,80,145,99,75,80);else if(g.enemyId>=20)magicSprite(c,islandArt::enemy(g.enemyId),48,64,145,99,75,80);else if(g.enemyId>=18)finaleFoe(c,g.enemyId,145,99,frame);else if(g.enemyId>=14)drawMimic(c,145,99,frame,true);else magicSprite(c,foeFrame,80,86,145,99,75,80);
  PanelEffectCanvas<Canvas> effectCanvas{c};
  if(v.effect==Effect::Rage){
    // Furia: broad axe-like sweep, red trails and expanding impact. No new art RAM.
    unsigned f=std::min(7u,v.effectFrame);int reach=90+int(f)*17;
    for(int trail=0;trail<3;++trail)for(int k=0;k<55;++k){int x=reach-k,y=52+k+trail*9;if(x>=45&&x<232)effectCanvas.fillRect(x,y,7,5,trail==0?0xffe0:trail==1?0xfd20:0xf800);}
    if(f>=3){int r=9+int(f-3)*7;int cx=184,cy=111;for(int k=-r;k<=r;++k){int y=r-abs(k);effectCanvas.fillRect(cx+k,cy-y,3,3,0xfd20);effectCanvas.fillRect(cx+k,cy+y,3,3,0xf800);}effectCanvas.fillRect(178,96,12,29,0xffe0);effectCanvas.fillRect(169,106,30,8,0xffe0);}
  }
  else if(v.effect==Effect::Projectile||v.effect==Effect::Lightning||v.effect==Effect::MagicDarts||v.effect==Effect::FlameVolley||v.effect==Effect::FireBurst||v.effect==Effect::Radiant||v.effect==Effect::DivineSlash){magicEffect(effectCanvas,v,false);}
  else if(v.effect==Effect::Thrust){unsigned t=std::min(7u,v.effectFrame);int x=v.effectOnHero?184-int(t)*20:52+int(t)*20;for(int k=0;k<6;++k)effectCanvas.fillRect(x-k*3,105-k,5,10,v.effect==Effect::Projectile?UI_BLUE:UI_GOLD);if(t>=5){effectCanvas.drawRect(v.effectOnHero?18:162,80,50,50,UI_WHITE);}}
  else if(v.effect!=Effect::None){const uint16_t* fx=v.effect==Effect::Lightning?sprites_lightning[v.effectFrame%lightning_frames]:v.effect==Effect::Shield?sprites_shield[v.effectFrame%shield_frames]:sprites_slash[v.effectFrame%slash_frames];if(v.effect==Effect::Lightning){for(int y=0;y<72;++y)for(int x=0;x<64;++x){uint16_t color=fx[y*64+x];if(color!=SPRITE_KEY)effectCanvas.fillRect(104+x*2,18+y*2,2,2,color);}}else magicSprite(effectCanvas,fx,64,72,v.effectOnHero?22:156,82,64,72);}
  panelLabel(c,4,181,232,rpg::intentName(g),UI_GOLD);
  // Panels and buttons retain the concept; all combat data remains live.
  c.fillRect(65,193,170,44,0x0843);snprintf(b,sizeof(b),"HP %u/%u",g.p.hp,g.p.maxhp);panelLabel(c,68,200,65,b);panelBar(c,70,210,62,g.p.hp,g.p.maxhp,UI_GREEN);
  snprintf(b,sizeof(b),"MP %u/%u",g.p.mp,g.p.maxmp);panelLabel(c,68,216,65,b);panelBar(c,70,225,62,g.p.mp,g.p.maxmp,UI_BLUE);
  panelHero(c,g,22,199,41,33);char turn[24];snprintf(turn,sizeof(turn),"Furia: %u",g.rageTurns);panelLabel(c,140,211,84,v.effect!=Effect::None?"Animando...":g.phase==rpg::Phase::Enemy?"Turno inimigo":g.surgePending?"Acao extra":g.rageTurns?turn:"Seu turno",UI_GOLD);
  snprintf(b,sizeof(b),"Risco ate %u HP",rpg::incomingCeiling(g,g.guard));panelLabel(c,132,227,101,b,UI_GOLD);
  if(*v.message)panelLabel(c,4,310,232,v.message,g.phase==rpg::Phase::Enemy?UI_GOLD:UI_WHITE);
  if(v.effect!=Effect::None||g.phase!=rpg::Phase::Hero){for(auto r:panelUi::battleButtons){c.drawRect(r.x,r.y,r.w,r.h,UI_MUTED);} }

}
// Inclusive lower and exclusive upper bounds match the visible controls.
inline bool hit(int x,int y,int left,int top,int width,int height=40){return x>=left&&x<left+width&&y>=top&&y<top+height;}



