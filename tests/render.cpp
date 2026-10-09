#include "TestHero.h"
#include "../firmware/RPG_POKET_2/View.h"
#include "../firmware/RPG_POKET_2/src/GFX/font/glcdfont.h"
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <cstdlib>
struct Canvas {
  uint16_t pixels[240*320]{};int tx=0,ty=0,size=1;uint16_t color=UI_WHITE;
  void pixel(int x,int y,uint16_t c){if(x>=0&&x<240&&y>=0&&y<320)pixels[y*240+x]=c;}
  void fillRect(int x,int y,int w,int h,uint16_t c){for(int j=0;j<h;++j)for(int i=0;i<w;++i)pixel(x+i,y+j,c);}
  void drawRect(int x,int y,int w,int h,uint16_t c){fillRect(x,y,w,1,c);fillRect(x,y+h-1,w,1,c);fillRect(x,y,1,h,c);fillRect(x+w-1,y,1,h,c);}
  void fillScreen(uint16_t c){fillRect(0,0,240,320,c);}
  void setTextWrap(bool){}
  void setTextColor(uint16_t c){color=c;}
  void setTextSize(int s){size=s;}
  void setCursor(int x,int y){tx=x;ty=y;}
  void print(const char* s){
    if(!(tx>=0&&ty>=0&&tx+int(strlen(s))*6*size<=240&&ty+8*size<=320))fprintf(stderr,"Text overflow: x=%d y=%d size=%d text=%s\n",tx,ty,size,s);
    assert(tx>=0&&ty>=0&&tx+int(strlen(s))*6*size<=240&&ty+8*size<=320);
    while(*s){unsigned ch=(unsigned char)*s++;for(int i=0;i<5;++i)for(int j=0;j<8;++j)if(font[ch*5+i]&(1<<j))fillRect(tx+i*size,ty+j*size,size,size,color);tx+=6*size;}
  }
  void draw16bitRGBBitmap(int x,int y,uint16_t* b,int w,int h){for(int j=0;j<h;++j)for(int i=0;i<w;++i)pixel(x+i,y+j,b[j*w+i]);}
  void save(const std::string& path){std::ofstream f(path,std::ios::binary);f<<"P6\n240 320\n255\n";for(auto p:pixels){char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};f.write(rgb,3);}}
};

// Legacy equipment regressions select a city that actually sells the item.
const char* buyStockGear(rpg::Game& g,uint8_t id){uint8_t old=g.city;if(!rpg::cityStock(g,id))g.city=3;auto err=rpg::buyGear(g,id);g.city=old;return err;}
int main(int argc,char** argv){
  assert(argc==2);artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);makeFallback();
  std::ifstream pack("outputs/RPG_POKET_2_0_Waveshare/cartao_preparado/RPGPOKET/artes.pak",std::ios::binary);if(!pack)pack.open("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);Canvas c;rpg::Game g=rpg::testHero(0,32);ViewState v;std::string root=argv[1];
  for(unsigned cls=0;cls<4;++cls)for(unsigned level=1;level<=20;++level){g=rpg::create(cls,42);g.p.xp=rpg::dndXp[level-1];rpg::levelUp(g);v=ViewState{};v.page=Page::Progression;v.evolutionLevel=level;uint8_t a[rpg::SAVE_SIZE],b[rpg::SAVE_SIZE];rpg::encode(g,1,a);render(c,g,v);rpg::encode(g,1,b);assert(!memcmp(a,b,sizeof(a)));if(level==1||level==3||level==4||level==5||level==20)c.save(root+"/progression-"+std::to_string(cls)+"-"+std::to_string(level)+".ppm");for(unsigned i=0;i<7;++i){v.page=Page::AttributeInfo;v.attributeIndex=i;render(c,g,v);if(level==4)c.save(root+"/attribute-"+std::to_string(cls)+"-"+std::to_string(i)+".ppm");}v.page=Page::Character;render(c,g,v);if(level==1)c.save(root+"/progression-hero-"+std::to_string(cls)+".ppm");}
  g=rpg::testHero(0,32);v=ViewState{};v.page=Page::Progression;render(c,g,v);c.save(root+"/progression-legacy.ppm");g=rpg::testHero(0,32);v=ViewState{};
  for(unsigned city=0;city<4;++city)for(unsigned kind=1;kind<=5;++kind){if(kind==4)continue;g=rpg::create(0,42);g.city=city;g.discovery=kind;g.discoverLoot=kind==2?7:kind==3?3:0;g.discoverAmount=7;v.page=Page::Discovery;v.message="";uint8_t a[rpg::SAVE_SIZE],b[rpg::SAVE_SIZE];rpg::encode(g,1,a);render(c,g,v,4);rpg::encode(g,1,b);assert(!memcmp(a,b,sizeof(a)));c.save(root+"/exploration-"+std::to_string(city)+"-"+std::to_string(kind)+".ppm");}
  g=rpg::create(0,42);g.scrap=9;v.page=Page::Scrap;render(c,g,v,0);c.save(root+"/exploration-scrap.ppm");g.discovery=2;g.discoverLoot=7;g.discoverAmount=1;rpg::collectDiscovery(g);v.page=Page::Battle;for(unsigned frame:{0,1,2,3}){render(c,g,v,frame);c.save(root+"/exploration-mimic-"+std::to_string(frame)+".ppm");}
  g=rpg::testHero(0,32);v=ViewState{};
  for(unsigned city=0;city<4;++city)for(unsigned state=0;state<4;++state){g=rpg::create(0,state==2?1:3);g.city=city;g.p.gold=120;g.gazuas=3;g.discovery=2;g.discoverLoot=0;g.discoverAmount=9;g.chestLock=1;g.chestTrap=true;if(state==1)rpg::attemptLock(g,true);if(state==2){g.chestRoll=20;g.chestTries=1;g.chestPick=true;g.chestLock=2;}if(state==3)rpg::attemptLock(g,false);v.page=Page::ChestLock;v.message="";uint8_t a[rpg::SAVE_SIZE],b[rpg::SAVE_SIZE];rpg::encode(g,1,a);render(c,g,v);rpg::encode(g,1,b);assert(!memcmp(a,b,sizeof(a)));c.save(root+"/locks-"+std::to_string(city)+"-"+std::to_string(state)+".ppm");}
  g=rpg::create(0,1);g.gazuas=9;g.p.gold=999999;v.page=Page::Lockpicks;render(c,g,v);c.save(root+"/locks-shop.ppm");v.page=Page::TownBag;render(c,g,v);c.save(root+"/locks-bag.ppm");g=rpg::testHero(0,32);v=ViewState{};
  for(int city=0;city<4;++city){g.city=city;v.page=Page::Home;for(auto period:{worldClock::Period::Day,worldClock::Period::Night}){menu.clockValid=true;menu.dayCycle=true;menu.worldPeriod=period;render(c,g,v,0);c.save(root+"/scenic-home-"+std::to_string(city)+"-"+std::to_string(int(period))+".ppm");}}
  menu.clockValid=false;menu.dayCycle=false;g.city=1;v.page=Page::Menu;render(c,g,v,0);c.save(root+"/scenic-menu.ppm");
  for(int i=0;i<240*320;++i)if(!(i/240>=281&&i/240<309&&((i%240>=14&&i%240<116)||(i%240>=124&&i%240<226))))assert(c.pixels[i]==scenicArt::menuPalette[scenicArt::menuPixels[i]]);
  uint8_t scenicBefore[rpg::SAVE_SIZE],scenicAfter[rpg::SAVE_SIZE];rpg::encode(g,11,scenicBefore);
  for(unsigned frame:{12,36,96}){render(c,g,v,frame);c.save(root+"/scenic-menu-"+std::to_string(frame)+".ppm");v.page=Page::Home;render(c,g,v,frame);c.save(root+"/scenic-home-"+std::to_string(frame)+".ppm");v.page=Page::Menu;}
  rpg::encode(g,11,scenicAfter);assert(!memcmp(scenicBefore,scenicAfter,sizeof(scenicBefore)));
  for(int wins=0;wins<=3;++wins){g.ruinsWins=wins;g.guardianDefeated=wins==3;v.page=Page::Ruins;render(c,g,v,0);c.save(root+"/scenic-ruins-"+std::to_string(wins)+".ppm");}
  for(int city=0;city<4;++city){g.city=city;menu.destination=city;v.page=Page::Map;render(c,g,v);c.save(root+"/scenic-map-"+std::to_string(city)+".ppm");}
  g.city=0;menu.destination=3;assert(!rpg::prepareTrip(g,3));menu.rollReady=false;v.page=Page::TravelRoll;
  for(unsigned frame:{0,5,13,24}){render(c,g,v,frame);c.save(root+"/scenic-d20-"+std::to_string(frame)+".ppm");}
  menu.rollReady=true;render(c,g,v,24);c.save(root+"/scenic-d20-result.ppm");menu.journey.start(0,3,0);v.page=Page::Travel;
  for(unsigned now:{0,750,1500,2250,3000}){menu.journey.tick(now);render(c,g,v,now/120);c.save(root+"/scenic-travel-"+std::to_string(now)+".ppm");}
  g=rpg::testHero(0,1);g.city=1;g.p.hp=1;v.message="";
  for(auto page:{Page::CampSetup,Page::CampRoll,Page::CampRest}){v.page=page;menu.rollReady=true;g.campRoll=13;render(c,g,v,4);c.save(root+"/panels-camp-"+std::to_string(int(page))+".ppm");}
  v.page=Page::TownBag;for(unsigned i=0;i<7;++i){v.choice=i;render(c,g,v);c.save(root+"/panels-bag-"+std::to_string(i)+".ppm");}v.choice=0;
  // Every new concept uses the real hero and read-only live state.
  for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race){g=rpg::testHero(cls,32);g.race=race;g.shirt=2;g.trousers=3;g.city=1;g.p.gold=999999;v.message="";
   uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,12,before);
   for(auto page:{Page::Battle,Page::Skills,Page::TownBag,Page::Character,Page::CampSetup,Page::CampRoll,Page::CampRest}){v.page=page;render(c,g,v,4);c.save(root+"/panels-"+std::to_string(int(page))+"-"+std::to_string(cls)+"-"+std::to_string(race)+".ppm");}
   rpg::encode(g,12,after);assert(!memcmp(before,after,sizeof(before)));
  }
  for(unsigned effect=1;effect<=6;++effect){g=rpg::testHero(0,4);g.city=1;g.enemyId=5;g.enemyHp=42;v.page=Page::Battle;v.effect=Effect(effect);for(unsigned f=0;f<8;++f){v.effectFrame=f;render(c,g,v,f);c.save(root+"/panels-fx-"+std::to_string(effect)+"-"+std::to_string(f)+".ppm");}}v.effect=Effect::None;
  g=rpg::testHero(0,32);v.message="";
   const Page pages[]={Page::Choose,Page::Help,Page::Home,Page::Battle,Page::Skills,Page::Bag,Page::Result,Page::SaveError,Page::Blocked};
  const char* names[]={"classe","guia","refugio","batalha","habilidades","pocoes","vitoria","erro-save","save-bloqueado"};
  for(unsigned i=0;i<9;++i){v.page=pages[i];g.phase=i==6?rpg::Phase::Won:rpg::Phase::Hero;g.gainXp=12;g.gainGold=6;v.message=i==3?"Voce causou 8 de dano":"";render(c,g,v);c.save(root+"/"+names[i]+".ppm");}
  for(int cls=0;cls<4;++cls){g=rpg::testHero(cls,1);v.page=Page::Skills;render(c,g,v);c.save(root+"/habilidade-"+std::to_string(cls)+".ppm");}
  g.phase=rpg::Phase::Lost;v.page=Page::Result;render(c,g,v);c.save(root+"/derrota.ppm");
  g.phase=rpg::Phase::Enemy;v.page=Page::Battle;v.message="Critico inimigo! -8 HP";render(c,g,v);c.save(root+"/turno-inimigo.ppm");
  g=rpg::testHero(0,1);g.p.gold=30;g.p.life=2;
  const Page townPages[]={Page::Village,Page::Shop,Page::Buy,Page::TownBag,Page::Character};
  const char* townNames[]={"vila","loja","compra","bolsa-vila","personagem"};
  for(int i=0;i<5;++i){v.page=townPages[i];v.message="";v.choice=0;render(c,g,v);c.save(root+"/"+townNames[i]+".ppm");}
  g=rpg::testHero(0,1);v.message="";
  for(int wins=0;wins<=3;++wins){g.ruinsWins=wins;v.page=Page::Ruins;render(c,g,v);c.save(root+"/mapa-"+std::to_string(wins)+".ppm");}
  g.guardianDefeated=true;render(c,g,v);c.save(root+"/mapa-vencido.ppm");
  for(int id=0;id<4;++id){rpg::begin(g,id);v.page=Page::Battle;render(c,g,v);c.save(root+"/inimigo-"+std::to_string(id)+".ppm");g.phase=rpg::Phase::Home;}
  g=rpg::testHero(0,4);g.ruinsWins=3;rpg::begin(g,3);v.page=Page::Battle;
  for(int id=0;id<4;++id){g.enemyId=id;g.enemyHp=rpg::enemySpec(id).hp;for(unsigned frame=0;frame<8;++frame){render(c,g,v,frame);c.save(root+"/sprite-"+std::to_string(id)+"-"+std::to_string(frame)+".ppm");}}
  for(int effect=1;effect<=4;++effect){v.effect=Effect(effect);v.effectOnHero=effect==3;for(unsigned frame=0;frame<8;++frame){v.effectFrame=frame;render(c,g,v,frame);c.save(root+"/efeito-"+std::to_string(effect)+"-"+std::to_string(frame)+".ppm");}}
  v.effect=Effect::None;
  for(int cls=0;cls<4;++cls){g=rpg::testHero(cls,8);rpg::begin(g);v.page=Page::Battle;v.message="";
    for(unsigned frame=0;frame<6;++frame){v.heroFrame=frame;render(c,g,v,0);c.save(root+"/heroi-"+std::to_string(cls)+"-"+std::to_string(frame)+".ppm");}}
  v.heroFrame=0;
  // Equipment pages: every catalog name/tier/class and full equip/retire previews.
  g=rpg::testHero(0,8);g.p.gold=999999;v.message="";v.gearIndex=0;v.itemId=1;
  const Page gearPages[]={Page::Market,Page::Inventory,Page::GearShop,Page::GearBag,Page::GearBuy,Page::GearEquip};
  const char* gearNames[]={"mercado","inventario","equip-loja","equip-vazio","equip-compra","equip-confirmar"};
  for(int i=0;i<5;++i){v.page=gearPages[i];render(c,g,v);c.save(root+"/"+gearNames[i]+".ppm");}
  for(int id=1;id<=18;++id){g=rpg::testHero(id<=12?(id-1)/3:0,8);g.p.level=8;g.p.mp=g.p.maxmp=rpg::maxMana(g.p.cls,8);g.p.gold=999999;
    v.page=Page::GearShop;v.gearIndex=id<=12?(id-1)%3:id-10;render(c,g,v);c.save(root+"/equip-item-"+std::to_string(id)+".ppm");
    v.page=Page::GearBuy;v.itemId=id;render(c,g,v);
    assert(!buyStockGear(g,id));v.page=Page::GearEquip;render(c,g,v);
    assert(!rpg::equipGear(g,id));rpg::rest(g);v.page=Page::GearEquip;render(c,g,v);
  }
  g=rpg::testHero(0,8);g.p.level=8;g.p.mp=g.p.maxmp=rpg::maxMana(0,8);g.p.gold=999999;
  for(uint8_t id:{uint8_t(1),uint8_t(3),uint8_t(13),uint8_t(16),uint8_t(18)})assert(!buyStockGear(g,id));
  assert(!rpg::equipGear(g,1));v.page=Page::GearBag;v.gearIndex=0;render(c,g,v);c.save(root+"/equip-bolsa.ppm");
  v.page=Page::GearEquip;v.itemId=3;render(c,g,v);c.save(root+"/equip-confirmar.ppm");
  assert(!rpg::equipGear(g,18));rpg::rest(g);v.itemId=18;render(c,g,v);c.save(root+"/equip-retirar.ppm");
  v.itemId=16;render(c,g,v);c.save(root+"/equip-trocar.ppm");
  v.page=Page::GearShop;v.gearIndex=0;v.message="Classe ou nivel insuficiente";render(c,g,v);c.save(root+"/equip-erro.ppm");v.message="";
  g.p.level=99;g.p.atk=g.p.def=255;g.p.hp=g.p.maxhp=65535;g.p.mp=g.p.maxmp=119;g.p.gold=999999;g.p.xp=UINT32_MAX;
  for(Page page:gearPages){v.page=page;render(c,g,v);}
  g=rpg::testHero(0,73);g.p.level=8;g.p.mp=g.p.maxmp=rpg::totalMana(g);g.p.gold=999999;v.message="";
  v.page=Page::Forge;render(c,g,v);c.save(root+"/ferreiro.ppm");
  for(uint8_t slot=0;slot<3;++slot){v.page=Page::Upgrade;v.forgeSlot=slot;
    for(int tier=0;tier<4;++tier){g.forge[slot]=tier;g.p.mp=g.p.maxmp=rpg::totalMana(g);render(c,g,v);c.save(root+"/forja-"+std::to_string(slot)+"-"+std::to_string(tier)+".ppm");}
    g.forge[slot]=0;}
  v.forgeSlot=2;v.message="Ouro insuficiente";render(c,g,v);c.save(root+"/forja-sem-ouro.ppm");
  v.message="Nivel insuficiente";render(c,g,v);c.save(root+"/forja-sem-nivel.ppm");v.message="";
  g.p.level=99;g.p.atk=g.p.def=255;g.forge[0]=g.forge[1]=g.forge[2]=3;g.owned=(1u<<18)-1;g.equipped[0]=3;g.equipped[1]=15;g.equipped[2]=18;g.p.mp=g.p.maxmp=rpg::totalMana(g);
  v.forgeSlot=2;render(c,g,v);c.save(root+"/forja-mana-max.ppm");v.page=Page::Forge;render(c,g,v);v.page=Page::Character;render(c,g,v);
  g=rpg::testHero(0,73);v.message="";v.page=Page::Tavern;render(c,g,v);c.save(root+"/taverna.ppm");
  for(uint8_t id=1;id<=3;++id){v.questChoice=id;v.page=Page::Contract;render(c,g,v);c.save(root+"/contrato-"+std::to_string(id)+".ppm");v.questAction=0;v.page=Page::QuestConfirm;render(c,g,v);c.save(root+"/aceitar-"+std::to_string(id)+".ppm");}
  assert(!rpg::acceptQuest(g,2));g.questProgress=1;v.page=Page::Tavern;render(c,g,v);c.save(root+"/missao-ativa.ppm");
  v.questChoice=2;v.questAction=2;v.page=Page::QuestConfirm;render(c,g,v);c.save(root+"/abandonar.ppm");
  g.questProgress=2;v.questAction=1;render(c,g,v);c.save(root+"/receber.ppm");v.page=Page::Ruins;render(c,g,v);c.save(root+"/mapa-missao.ppm");
  g.phase=rpg::Phase::Won;g.enemyHp=0;v.page=Page::Result;render(c,g,v);c.save(root+"/vitoria-missao.ppm");
  g.phase=rpg::Phase::Home;g.p.level=g.questLevel=99;g.p.maxmp=rpg::totalMana(g);g.p.xp=UINT32_MAX;g.p.gold=999999;g.questId=3;g.questProgress=1;v.questChoice=3;v.questAction=1;
  for(Page page:{Page::Tavern,Page::Contract,Page::QuestConfirm}){v.page=page;render(c,g,v);}
  // Every navigation page has a distinct background, including modal/error pages.
  v.page=Page::Card;v.message="";
  for(int status=0;status<=5;++status){v.card.status=CardStatus(status);v.card.capacityMiB=status>1?3815:0;render(c,g,v);c.save(root+"/cartao-"+std::to_string(status)+".ppm");}
  v.card.capacityMiB=UINT32_MAX;render(c,g,v);
  g=rpg::testHero(0,1);const Backdrop* seen[64]={};int count=0;
  for(int page=0;page<=int(Page::Explore);++page){const auto* bg=&backdropFor(Page(page),g);if(Page(page)==Page::Travel||Page(page)==Page::TravelRoll)continue;for(int i=0;i<count;++i)assert(seen[i]!=bg);seen[count++]=bg;}
  for(auto phase:{rpg::Phase::Won,rpg::Phase::Lost}){g.phase=phase;const auto* bg=&backdropFor(Page::Result,g);for(int i=0;i<count;++i)assert(seen[i]!=bg);seen[count++]=bg;}
  for(int id:{0,1,3}){g.enemyId=id;const auto* bg=&backdropFor(Page::Battle,g);for(int i=0;i<count;++i)assert(seen[i]!=bg);seen[count++]=bg;}
  for(unsigned city=1;city<4;++city){g.city=city;const auto* bg=&backdropFor(Page::Explore,g);for(int i=0;i<count;++i)assert(seen[i]!=bg);seen[count++]=bg;}g.city=0;
  assert(count==56);
  for(int i=0;i<count;++i){drawBackdrop(c,*seen[i]);for(int y=0;y<160;++y)for(int x=0;x<120;++x){uint16_t expected=seen[i]->palette()[seen[i]->pixels()[y*120+x]];assert(c.pixels[y*2*240+x*2]==expected&&c.pixels[(y*2+1)*240+x*2+1]==expected);}}
  // Largest legal display values must also fit on screen.
  g.p.level=99;g.p.hp=g.p.maxhp=65535;g.p.mp=g.p.maxmp=110;g.p.gold=999999;g.p.xp=UINT32_MAX;g.p.life=g.p.mana=99;
  for(Page page:townPages){v.page=page;render(c,g,v);}
  assert(hit(14,220,14,220,102)&&!hit(116,220,14,220,102)&&!hit(13,220,14,220,102));

  g=rpg::testHero(0,1);v.message="";menu.previews[0]=g;menu.slots[0]=rpg::Load::Ok;menu.slots[1]=rpg::Load::Blocked;
  for(Page page:{Page::Menu,Page::Slots,Page::SlotConfirm,Page::DeleteSlot,Page::Settings,Page::Wifi,Page::Keyboard,Page::Tests,Page::Map}){v.page=page;render(c,g,v);c.save(root+"/menu-"+std::to_string(int(page))+".ppm");}
  for(unsigned cls=0;cls<4;++cls){g=rpg::testHero(cls,1);for(unsigned a=0;a<4;++a)for(unsigned dest=0;dest<4;++dest){if(a==dest)continue;menu.journey=Journey{};assert(menu.journey.start(a,dest,0));v.page=Page::Travel;for(unsigned tick=0;tick<=3000;tick+=150){menu.journey.tick(tick);render(c,g,v,tick/120);} }}
  menu.journey=Journey{};menu.journey.start(0,3,0);v.page=Page::Travel;for(unsigned tick:{0u,750u,1500u,2250u,3000u}){menu.journey.tick(tick);render(c,g,v,tick/120);c.save(root+"/viagem-"+std::to_string(tick)+".ppm");}
  v.page=Page::Keyboard;for(unsigned mode=0;mode<3;++mode){menu.keyboard=mode;render(c,g,v);c.save(root+"/teclado-"+std::to_string(mode)+".ppm");}
  for(unsigned race=0;race<4;++race)for(unsigned cls=0;cls<4;++cls){menu.draftRace=race;v.choice=cls;g=rpg::testHero(cls,1);g.race=race;for(unsigned color=0;color<8;++color){menu.draftShirt=g.shirt=color;menu.draftPants=g.trousers=7-color;v.page=Page::Clothes;render(c,g,v,3);c.save(root+"/raca-"+std::to_string(race)+"-classe-"+std::to_string(cls)+"-cor-"+std::to_string(color)+".ppm");}}
  for(Page page:{Page::Race,Page::Guild,Page::GuildJoin,Page::GuildMissions,Page::Club,Page::ClubBattle,Page::ClubResult}){v.page=page;v.message="";render(c,g,v);c.save(root+"/nova-"+std::to_string(int(page))+".ppm");}
  v.page=Page::Keyboard;for(unsigned mode=0;mode<3;++mode)for(unsigned pg=0;pg<3;++pg){menu.keyboard=mode;menu.keyPage=pg;render(c,g,v);c.save(root+"/keyboard-"+std::to_string(mode)+"-"+std::to_string(pg)+".ppm");}
  // Stable clothing preview, even during former attack-frame ticks.
  v.page=Page::Clothes;render(c,g,v,0);std::vector<uint16_t> stable(c.pixels,c.pixels+240*320);render(c,g,v,5);assert(std::equal(stable.begin(),stable.end(),c.pixels));
  v.page=Page::Updates;menu.connected=true;menu.signalBars=2;render(c,g,v);assert(c.pixels[15*240+229]==0x07e0);c.save(root+"/atualizacao.ppm");
  for(unsigned state=0;state<=unsigned(updater::State::Error);++state){updateInfo.state=updater::State(state);updateInfo.busy=state==1||(state>=4&&state<=8);updateInfo.progress=57;strcpy(updateInfo.version,"2026.10.06-ota1");strcpy(updateInfo.message,"Mantenha a alimentacao");render(c,g,v);c.save(root+"/update-"+std::to_string(state)+".ppm");}menu.connected=false;updateInfo=updater::Info{};
  for(unsigned city=0;city<4;++city){g=rpg::testHero(0,10);g.city=city;for(Page page:{Page::Village,Page::Explore,Page::CityGoods,Page::GoodsBuy,Page::GearShop,Page::Shop}){v.page=page;render(c,g,v);c.save(root+"/cidade-"+std::to_string(city)+"-"+std::to_string(int(page))+".ppm");}}
  for(unsigned id=4;id<8;++id){g=rpg::testHero(1,1);rpg::begin(g,id);v.page=Page::Battle;for(unsigned frame=0;frame<4;++frame){v.effectOnHero=frame>=2;v.effect=frame>=2?Effect::Slash:Effect::None;v.effectFrame=frame;render(c,g,v,frame);c.save(root+"/novo-inimigo-"+std::to_string(id)+"-"+std::to_string(frame)+".ppm");}}v.effect=Effect::None;
  g=rpg::testHero(0,1);rpg::prepareTrip(g,3);v.page=Page::TravelRoll;for(unsigned frame=0;frame<20;++frame){menu.rollReady=frame>=15;render(c,g,v,frame);}menu.rollReady=true;render(c,g,v);c.save(root+"/dado-viagem.ppm");
  for(unsigned bars=1;bars<=4;++bars){menu.connected=true;menu.signalBars=bars;menu.signalDbm=bars==4?-48:bars==3?-60:bars==2?-72:-82;v.page=Page::Wifi;menu.savedNetworks=false;menu.networkCount=2;strcpy(menu.network,"RedeCasa2.4GHz");menu.networkDbm=-55;menu.networkChannel=6;render(c,g,v);for(unsigned i=0;i<4;++i)assert(c.pixels[15*240+223+i*4]==(i<bars?0x07e0:0x3186));c.save(root+"/wifi-sinal-"+std::to_string(bars)+".ppm");}
  menu.savedNetworks=true;menu.savedCount=5;menu.savedIndex=4;v.page=Page::Wifi;render(c,g,v);c.save(root+"/wifi-salvas.ppm");v.page=Page::ForgetWifi;render(c,g,v);c.save(root+"/wifi-esquecer.ppm");v.page=Page::Clock;strcpy(menu.clockTime,"17:05:26");strcpy(menu.clockDate,"06/10/2026");menu.clockValid=true;render(c,g,v);assert(c.pixels[0]==0&&c.pixels[319*240+239]==0);c.save(root+"/relogio.ppm");menu.connected=false;
  v.page=Page::NetworkTest;updateInfo.tested=true;updateInfo.netTrials=3;updateInfo.netSuccess=2;updateInfo.minDbm=-71;updateInfo.maxDbm=-65;updateInfo.avgMs=450;render(c,g,v);c.save(root+"/wifi-teste.png.ppm");
  makeFallback();v.page=Page::Battle;render(c,g,v);c.save(root+"/sem-cartao.ppm");
  for(unsigned cls=0;cls<4;++cls){g=rpg::testHero(cls,42);g.city=1;g.crystals=1;assert(!rpg::enterDungeon(g));v.message="";v.effect=Effect::None;
    for(unsigned floor=0;floor<2;++floor)for(unsigned direction=0;direction<4;++direction){g.dungeonFlags=1|(floor<<1)|(direction<<2);v.page=Page::Dungeon;render(c,g,v);c.save(root+"/dungeon-"+std::to_string(cls)+"-"+std::to_string(floor)+"-"+std::to_string(direction)+".ppm");}
    for(Page page:{Page::DungeonEntry,Page::DungeonMenu,Page::CrystalBuy,Page::DungeonExit}){v.page=page;render(c,g,v);c.save(root+"/dungeon-page-"+std::to_string(int(page))+".ppm");}
    for(unsigned enemy=0;enemy<4;++enemy)for(unsigned pose=0;pose<4;++pose){unsigned i=enemy==0?0:enemy==1?2:enemy==2?5:6;auto spawn=rpg::dungeonSpawns[i];g.dungeonFlags=1|(spawn.floor<<1);g.dungeonXY=spawn.x|((spawn.y+1)<<4);g.enemyId=spawn.id;g.enemyHp=rpg::enemySpec(spawn.id).hp;g.phase=rpg::Phase::Hero;v.page=Page::Dungeon;v.effect=pose?Effect::Slash:Effect::None;v.effectOnHero=pose==1||pose==2;v.effectFrame=pose==2?5:0;render(c,g,v);c.save(root+"/dungeon-enemy-"+std::to_string(enemy)+"-"+std::to_string(pose)+".ppm");}
  }
  // All bag slots, maximum quantities and gear pagination; boss decision, each attack phase.
  g=rpg::testHero(0,42);g.p.gold=999999;g.crystals=9;g.p.life=99;g.p.mana=99;g.rations=g.charts=g.charms=9;g.owned=(1u<<18)-1;v.effect=Effect::None;v.message="";
  for(Page page:{Page::Bag,Page::TownBag,Page::BagGear})for(unsigned pg=0;pg<(page==Page::BagGear?3u:1u);++pg)for(unsigned slot=0;slot<6;++slot){v.page=page;v.choice=slot;v.gearIndex=pg;render(c,g,v);c.save(root+"/dungeon-bag-"+std::to_string(int(page))+"-"+std::to_string(pg)+"-"+std::to_string(slot)+".ppm");}
  v.page=Page::DungeonLoot;v.message="CAJADO DO TROVAO";render(c,g,v);c.save(root+"/dungeon-loot.ppm");v.message="";v.page=Page::DungeonVictory;g.gainXp=190;g.gainGold=65;render(c,g,v);c.save(root+"/dungeon-victory.ppm");
  for(unsigned cls=0;cls<4;++cls){g=rpg::testHero(cls,42);g.city=1;g.crystals=1;rpg::enterDungeon(g);g.dungeonXY=0x72;g.phase=rpg::Phase::Hero;g.enemyId=2;g.enemyHp=19;v.page=Page::Dungeon;v.effectOnHero=false;
    for(auto fx:{Effect::Slash,Effect::Projectile,Effect::Thrust,Effect::Lightning,Effect::Rage})for(unsigned step=0;step<8;++step){v.effect=fx;v.effectFrame=step;render(c,g,v);c.save(root+"/dungeon-fx-"+std::to_string(cls)+"-"+std::to_string(int(fx))+"-"+std::to_string(step)+".ppm");}}
  // Camp previews use the actual regional scenery, not the earlier no-card fallback fixture.
  pack.clear();pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
  v.message="";v.effect=Effect::None;
  for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls)for(unsigned food=0;food<2;++food)for(unsigned kit=0;kit<2;++kit){g=rpg::testHero(cls,42);g.city=city;g.sleepKit=kit;g.rations=1;g.p.hp=1;g.p.mp=0;rpg::startCamp(g,food,kit);g.campRoll=20;rpg::acceptCamp(g);v.page=Page::CampRest;
    for(unsigned step=0;step<4;++step){v.campProgress=step*333;render(c,g,v,step);c.save(root+"/camp-rest-"+std::to_string(city)+"-"+std::to_string(cls)+"-"+std::to_string(food)+"-"+std::to_string(kit)+"-"+std::to_string(step)+".ppm");}}
  g=rpg::testHero(0,42);g.p.hp=1;g.p.gold=999999;g.rations=9;g.sleepKit=true;menu.campRation=true;menu.campKit=true;v.itemId=rpg::gearOffer(1,1);
  for(Page page:{Page::CampSetup,Page::CampRoll,Page::CampKit,Page::GearSell,Page::Bag}){v.page=page;g.campRoll=1;menu.rollReady=true;render(c,g,v);c.save(root+"/camp-page-"+std::to_string(int(page))+".ppm");}
  v.page=Page::Bag;v.choice=6;render(c,g,v);c.save(root+"/camp-bag-kit.ppm");
  g=rpg::testHero(0,42);v.message="";v.effect=Effect::None;
  for(unsigned scene=0;scene<4;++scene){menu.storyIndex=scene;v.page=Page::Prologue;render(c,g,v);c.save(root+"/lore-opening-"+std::to_string(scene)+".ppm");}
  for(unsigned city=0;city<4;++city){g.city=city;for(Page pg:{Page::People,Page::Village,Page::Ruins,Page::CityGoods,Page::Forge,Page::Guild,Page::Tavern}){v.page=pg;render(c,g,v);c.save(root+"/lore-city-"+std::to_string(city)+"-"+std::to_string(int(pg))+".ppm");}for(unsigned npc=0;npc<3;++npc){menu.personIndex=npc;v.page=Page::Dialogue;g.dungeonClears=npc==2?1:0;render(c,g,v);c.save(root+"/lore-npc-"+std::to_string(city)+"-"+std::to_string(npc)+".ppm");}}
  g=rpg::testHero(0,42);for(unsigned veteran=0;veteran<2;++veteran){g.tutorial=veteran;g.guardianDefeated=veteran;g.dungeonClears=veteran;for(unsigned i=0;i<6;++i){menu.chapterIndex=i;v.page=Page::Journal;render(c,g,v);c.save(root+"/lore-journal-"+std::to_string(veteran)+"-"+std::to_string(i)+".ppm");}}
  for(unsigned i=0;i<8;++i){menu.regionIndex=i;v.page=Page::Continent;render(c,g,v);c.save(root+"/lore-region-"+std::to_string(i)+".ppm");}v.page=Page::Map;render(c,g,v);c.save(root+"/lore-map.ppm");
  menu.clockValid=true;snprintf(menu.clockTime,sizeof(menu.clockTime),"21:30:00");snprintf(menu.clockDate,sizeof(menu.clockDate),"07/10/2026");g=rpg::testHero(0,42);v.message="";
  for(unsigned period=0;period<4;++period){menu.worldPeriod=worldClock::Period(period);snprintf(menu.clockTime,sizeof(menu.clockTime),"%02u:30:00",period==0?5:period==1?12:period==2?18:22);for(Page pg:{Page::Home,Page::Map,Page::Village,Page::Explore,Page::CampSetup,Page::Clock,Page::TimeSettings,Page::TimeEdit}){v.page=pg;render(c,g,v);c.save(root+"/day-period-"+std::to_string(period)+"-"+std::to_string(int(pg))+".ppm");}}
  menu.clockValid=false;v.page=Page::TimeSettings;render(c,g,v);c.save(root+"/day-unsynced.ppm");v.page=Page::Clock;render(c,g,v);c.save(root+"/day-clock-unsynced.ppm");
  menu.sessionStarted=true;menu.clockValid=true;menu.notice="";menu.renderNow=1000;menu.letterStarted=0;g=rpg::testHero(0,42);g.tutorial=true;rpg::offerEvent(g,20733,10);
  for(unsigned tier=0;tier<4;++tier){g.eventTier=tier;for(Page pg:{Page::Letters,Page::Letter,Page::LetterRefuse,Page::EventTravel,Page::Clock}){v.page=pg;render(c,g,v);c.save(root+"/event-page-"+std::to_string(tier)+"-"+std::to_string(int(pg))+".ppm");}
   rpg::acceptEvent(g,49);v.page=Page::Battle;for(unsigned i=0;i<4;++i){v.effect=i>=2?Effect::Slash:Effect::None;v.effectOnHero=i>=2;v.effectFrame=i;render(c,g,v,i);c.save(root+"/event-battle-"+std::to_string(tier)+"-"+std::to_string(i)+".ppm");}v.effect=Effect::None;v.page=Page::EventResult;for(auto result:{rpg::Phase::Won,rpg::Phase::Lost,rpg::Phase::Fled}){g.phase=result;render(c,g,v);c.save(root+"/event-result-"+std::to_string(tier)+"-"+std::to_string(int(result))+".ppm");}g=rpg::testHero(0,42);g.tutorial=true;rpg::offerEvent(g,20733,10);
  }v.page=Page::Letter;for(unsigned t:{0u,120u,240u,440u,450u}){menu.renderNow=t;render(c,g,v);c.save(root+"/event-scroll-"+std::to_string(t)+".ppm");}

  // Title frames are independent of SD and do not alter the character or save bytes.
  uint8_t beforeTitle[rpg::SAVE_SIZE];rpg::encode(g,17,beforeTitle);v.page=Page::Title;menu.notice="";
  std::vector<uint16_t> titleFrames;
  for(unsigned enabled=0;enabled<2;++enabled){menu.hasContinue=enabled;for(unsigned frame:{0u,12u,36u,96u,192u,319u}){render(c,g,v,frame);c.save(root+"/title-"+std::to_string(enabled)+"-"+std::to_string(frame)+".ppm");if(frame==0)titleFrames.assign(c.pixels,c.pixels+240*320);else assert(!std::equal(titleFrames.begin(),titleFrames.end(),c.pixels));uint8_t after[rpg::SAVE_SIZE];rpg::encode(g,17,after);assert(!memcmp(beforeTitle,after,sizeof(after)));}}
  v.page=Page::Campaign;for(unsigned progress=0;progress<5;++progress){g=rpg::testHero(0,42);g.tutorial=progress>=1;g.ruinsWins=progress>=2;g.guardianDefeated=progress>=3;g.dungeonClears=progress>=4;render(c,g,v);c.save(root+"/title-objective-"+std::to_string(progress)+".ppm");}
  v.page=Page::Dialogue;g.city=0;menu.personIndex=0;for(unsigned cls=0;cls<4;++cls){g.p.cls=cls;render(c,g,v);c.save(root+"/title-nara-"+std::to_string(cls)+".ppm");}
  for(unsigned cls=0;cls<4;++cls){g=rpg::create(cls,42);v.page=Page::Evolution;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);for(unsigned lv=1;lv<=20;++lv){v.evolutionLevel=lv;render(c,g,v,0);if(lv==1||lv==4||lv==20)c.save(root+"/evolution-"+std::to_string(cls)+"-"+std::to_string(lv)+".ppm");}rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));}
  v.message="";v.effect=Effect::None;menu.connected=false;
  for(unsigned cls=0;cls<4;++cls)for(unsigned lv:{1u,2u,3u,5u,8u,9u,10u,12u,13u,14u,15u,16u,17u,18u,20u}){g=rpg::create(cls,42);g.p.xp=rpg::dndXp[lv-1];rpg::levelUp(g);g.p.mp=g.p.maxmp;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);v.page=Page::Powers;for(unsigned i=0;i<powersUi::count(cls);++i){v.powerIndex=i;render(c,g,v);c.save(root+"/powers-"+std::to_string(cls)+"-"+std::to_string(lv)+"-"+std::to_string(i)+".ppm");}rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));}
  for(unsigned lv:{9u,10u,17u,18u,20u}){g=rpg::create(0,42);g.p.xp=rpg::dndXp[lv-1];rpg::levelUp(g);g.p.mp=0;v.page=Page::Skills;v.message="";render(c,g,v);c.save(root+"/powers-mastery-skills-"+std::to_string(lv)+".ppm");}
  for(unsigned id=0;id<18;++id)for(unsigned beat=0;beat<3;++beat){g=rpg::create(id%4,42);g.enemyId=id;g.enemyHp=rpg::encounterHp(g,id);g.phase=rpg::Phase::Hero;g.enemyBeat=beat;v.page=Page::EnemyInfo;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);render(c,g,v);rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));if(beat==1)c.save(root+"/enemy-info-"+std::to_string(id)+".ppm");}
  for(unsigned mask:{0u,1u,(1u<<18)-1}){g=rpg::create(0,42);g.seenEnemies=mask;v.page=Page::Bestiary;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);for(unsigned id=0;id<18;++id){v.bestiaryIndex=id;render(c,g,v);if(id==5)c.save(root+"/bestiary-"+std::to_string(mask)+".ppm");}rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));}
  g=rpg::create(1,42);g.p.xp=900;rpg::levelUp(g);v.page=Page::OathConfirm;render(c,g,v);c.save(root+"/powers-oath.ppm");
  g=rpg::create(0,42);rpg::begin(g,7);v.page=Page::Battle;for(auto effect:{Effect::MagicDarts,Effect::FlameVolley,Effect::FireBurst,Effect::Radiant,Effect::DivineSlash})for(unsigned frame=0;frame<8;++frame){v.effect=effect;v.effectFrame=frame;v.effectOnHero=effect==Effect::Radiant;render(c,g,v);if(frame==4)c.save(root+"/powers-effect-"+std::to_string(int(effect))+".ppm");}
  g=rpg::create(1,42);g.p.xp=rpg::dndXp[10];rpg::levelUp(g);rpg::begin(g,7);v.page=Page::Battle;v.effect=Effect::DivineSlash;v.effectOnHero=false;
  for(unsigned frame=0;frame<8;++frame){uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);v.effectFrame=frame;render(c,g,v);rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));c.save(root+"/powers-smite-"+std::to_string(frame)+".ppm");}v.effect=Effect::None;
  v.message="";v.effect=Effect::None;g=rpg::create(2,42);g.tutorial=true;
  for(unsigned topic=0;topic<launchUi::guideCount;++topic){v.page=Page::Guide;v.guideIndex=topic;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);render(c,g,v);rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));c.save(root+"/launch-guide-"+std::to_string(topic)+".ppm");}
  for(unsigned dest=1;dest<4;++dest){v.tripDestination=dest;v.page=Page::TravelConfirm;render(c,g,v);c.save(root+"/launch-trip-"+std::to_string(dest)+".ppm");}
  v.page=Page::Recovery;g.p.hp=1;render(c,g,v);c.save(root+"/launch-recovery.ppm");
  for(unsigned id:{1u,2u,3u,5u,7u,8u})for(unsigned beat=0;beat<3;++beat){g=rpg::create(2,42);g.ruinsWins=3;rpg::begin(g,id);g.enemyBeat=beat;v.page=Page::Battle;render(c,g,v);c.save(root+"/launch-intent-"+std::to_string(id)+"-"+std::to_string(beat)+".ppm");}
  g=rpg::create(2,42);g.p.xp=2700;rpg::levelUp(g);g.p.gold=9999;g.owned=1u<<6;g.equipped[0]=7;v.page=Page::GearBuy;v.itemId=8;render(c,g,v);c.save(root+"/launch-gear.ppm");
  g=rpg::create(2,77);g.p.xp=64000;rpg::levelUp(g);g.tutorial=true;g.city=2;g.dungeonClears=1;v.message="";v.effect=Effect::None;menu.personIndex=0;
  for(unsigned mission=1;mission<=2;++mission){g.campaignFlags=mission-1;v.campaignChoice=mission;v.page=Page::CampaignTask;render(c,g,v);c.save(root+"/campaign-task-"+std::to_string(mission)+".ppm");rpg::startCampaign(g,mission);g.enemyHp=0;rpg::finish(g);v.page=Page::CampaignResult;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);for(unsigned i=0;i<4;++i){v.campaignScene=i;render(c,g,v);c.save(root+"/campaign-result-"+std::to_string(mission)+"-"+std::to_string(i)+".ppm");}rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));rpg::resolveCampaign(g);}
  g.phase=rpg::Phase::Lost;g.campaignStage=2;v.page=Page::CampaignResult;render(c,g,v);c.save(root+"/campaign-loss.ppm");g.phase=rpg::Phase::Fled;render(c,g,v);g.campaignStage=0;g.phase=rpg::Phase::Home;g.campaignFlags=3;v.page=Page::Dialogue;render(c,g,v);c.save(root+"/campaign-sabela.ppm");v.page=Page::Campaign;render(c,g,v);c.save(root+"/campaign-aurora.ppm");
  // All origins, races, speaker faces and fallbacks use the shared renderer without mutating saves.
  for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race){g=rpg::create(cls,71);g.originStory=true;g.race=race;v.page=Page::Prologue;menu.storyReplay=false;
   uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);
   for(unsigned i=0;i<story::originCount(g);++i){menu.storyIndex=i;render(c,g,v);if(!race)c.save(root+"/origens-intro-"+std::to_string(cls)+"-"+std::to_string(i)+".ppm");}
   rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));
  }
  g=rpg::create(0,71);g.tutorial=true;g.dungeonClears=1;
  for(unsigned city=0;city<4;++city){g.city=city;for(unsigned i=0;i<3;++i){menu.personIndex=i;v.page=Page::Dialogue;render(c,g,v);c.save(root+"/origens-npc-"+std::to_string(city)+"-"+std::to_string(i)+".ppm");}}
  g.city=1;g.dungeonClears=0;menu.personIndex=2;v.page=Page::Dialogue;render(c,g,v);c.save(root+"/origens-desconhecido.ppm");
  makeFallback();g.originStory=true;menu.storyIndex=4;v.page=Page::Prologue;render(c,g,v);c.save(root+"/origens-sem-cartao.ppm");
  pack.clear();pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES);g=rpg::create(0,77);g.p.xp=265000;rpg::levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=true;g.city=3;g.dungeonClears=1;g.campaignFlags=3;v=ViewState{};v.page=Page::CampaignTask;v.campaignChoice=3;render(c,g,v);c.save(root+"/aurora-task.ppm");assert(!rpg::startCampaign(g,3));g.enemyHp=0;rpg::finish(g);v.page=Page::CampaignResult;uint8_t auroraBefore[rpg::SAVE_SIZE],auroraAfter[rpg::SAVE_SIZE];rpg::encode(g,1,auroraBefore);for(unsigned i=0;i<6;++i){v.campaignScene=i;render(c,g,v);c.save(root+"/aurora-scene-"+std::to_string(i)+".ppm");}rpg::encode(g,1,auroraAfter);assert(!memcmp(auroraBefore,auroraAfter,sizeof(auroraBefore)));assert(rpg::resolveCampaign(g));v.page=Page::Campaign;render(c,g,v);c.save(root+"/aurora-objective.ppm");for(unsigned i=0;i<3;++i){v.page=Page::Dialogue;menu.personIndex=i;render(c,g,v);c.save(root+"/aurora-dialogue-"+std::to_string(i)+".ppm");}v.page=Page::Journal;menu.chapterIndex=4;render(c,g,v);c.save(root+"/aurora-journal.ppm");
  pack.clear();pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES);
  for(unsigned city=0;city<4;++city)for(unsigned person=0;person<3;++person)for(unsigned progress=0;progress<6;++progress){g=rpg::create(0,77);g.city=city;g.tutorial=true;if(progress>=1){g.ruinsWins=3;g.guardianDefeated=true;}if(progress>=2)g.dungeonClears=1;if(progress>=3)g.campaignFlags=progress==3?1:progress==4?3:7;v=ViewState{};v.page=Page::Dialogue;menu.personIndex=person;unsigned pages=story::conversationPages(story::conversation(g,person));uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);for(unsigned page=0;page<pages;++page){v.dialoguePage=page;render(c,g,v);if(person==0&&(progress==0||progress==5))c.save(root+"/voices-"+std::to_string(city)+"-"+std::to_string(progress)+"-"+std::to_string(page)+".ppm");}rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));}
  g=rpg::create(0,77);g.city=1;menu.personIndex=0;v=ViewState{};v.page=Page::Dialogue;v.dialogueAnimate=true;v.dialogueStartFrame=100;uint8_t animBefore[rpg::SAVE_SIZE],animAfter[rpg::SAVE_SIZE];rpg::encode(g,1,animBefore);std::vector<uint16_t> rolled,opened;for(unsigned tick=0;tick<=6;++tick){render(c,g,v,100+tick);c.save(root+"/voices-opening-"+std::to_string(tick)+".ppm");if(!tick)rolled.assign(c.pixels,c.pixels+240*320);if(tick==6)opened.assign(c.pixels,c.pixels+240*320);}assert(rolled!=opened);render(c,g,v,110);assert(!memcmp(c.pixels,opened.data(),240*320*2));rpg::encode(g,1,animAfter);assert(!memcmp(animBefore,animAfter,sizeof(animBefore)));
  for(unsigned city=0;city<4;++city){g=rpg::create(0,77);g.p.xp=265000;rpg::levelUp(g);g.tutorial=true;g.city=city;g.dungeonClears=1;g.campaignFlags=7;g.rations=3;g.p.gold=600;unsigned m=4+city;v=ViewState{};v.page=Page::CampaignTask;v.campaignChoice=m;render(c,g,v);c.save(root+"/anwen-task-"+std::to_string(city)+".ppm");assert(!rpg::startCampaign(g,m));if(!rpg::campaignDonation(m)){g.enemyHp=0;rpg::finish(g);v.page=Page::CampaignResult;}else v.page=Page::ContributionResult;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);for(unsigned page=0;page<story::contributionPages(m);++page){v.campaignScene=page;render(c,g,v);c.save(root+"/anwen-result-"+std::to_string(city)+"-"+std::to_string(page)+".ppm");}rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));if(!rpg::campaignDonation(m)){assert(rpg::resolveCampaign(g));}v.page=Page::Campaign;render(c,g,v);c.save(root+"/anwen-progress-"+std::to_string(city)+".ppm");v.page=Page::Journal;menu.chapterIndex=4;render(c,g,v);}
  g.campaignFlags=127;v.page=Page::Campaign;render(c,g,v);c.save(root+"/anwen-complete.ppm");v.page=Page::Dialogue;menu.personIndex=0;v.dialogueAnimate=false;for(unsigned page=0;page<story::conversationPages(story::conversation(g,0));++page){v.dialoguePage=page;render(c,g,v);}v.page=Page::CampaignResult;g.campaignFlags=7;g.campaignStage=5;g.city=1;g.phase=rpg::Phase::Lost;render(c,g,v);c.save(root+"/anwen-loss.ppm");
  puts("PASS: shared firmware renderer; text bounds; regional camp scenes, kit slot, 56 distinct backgrounds; travel frames; full-frame pixel parity; optional card states, contracts, forge, equipment and combat.");
}



