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
    assert(tx>=0&&ty>=0&&tx+int(strlen(s))*6*size<=240&&ty+8*size<=320);
    while(*s){unsigned ch=(unsigned char)*s++;for(int i=0;i<5;++i)for(int j=0;j<8;++j)if(font[ch*5+i]&(1<<j))fillRect(tx+i*size,ty+j*size,size,size,color);tx+=6*size;}
  }
  void draw16bitRGBBitmap(int x,int y,uint16_t* b,int w,int h){for(int j=0;j<h;++j)for(int i=0;i<w;++i)pixel(x+i,y+j,b[j*w+i]);}
  void save(const std::string& path){std::ofstream f(path,std::ios::binary);f<<"P6\n240 320\n255\n";for(auto p:pixels){char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};f.write(rgb,3);}}
};
int main(int argc,char** argv){
  assert(argc==2);artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);makeFallback();
  std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);Canvas c;rpg::Game g=rpg::create(0,32);ViewState v;std::string root=argv[1];
  const Page pages[]={Page::Choose,Page::Help,Page::Home,Page::Battle,Page::Skills,Page::Bag,Page::Result,Page::SaveError,Page::Blocked};
  const char* names[]={"classe","guia","refugio","batalha","habilidades","pocoes","vitoria","erro-save","save-bloqueado"};
  for(unsigned i=0;i<9;++i){v.page=pages[i];g.phase=i==6?rpg::Phase::Won:rpg::Phase::Hero;g.gainXp=12;g.gainGold=6;v.message=i==3?"Voce causou 8 de dano":"";render(c,g,v);c.save(root+"/"+names[i]+".ppm");}
  for(int cls=0;cls<4;++cls){g=rpg::create(cls,1);v.page=Page::Skills;render(c,g,v);c.save(root+"/habilidade-"+std::to_string(cls)+".ppm");}
  g.phase=rpg::Phase::Lost;v.page=Page::Result;render(c,g,v);c.save(root+"/derrota.ppm");
  g.phase=rpg::Phase::Enemy;v.page=Page::Battle;v.message="Critico inimigo! -8 HP";render(c,g,v);c.save(root+"/turno-inimigo.ppm");
  g=rpg::create(0,1);g.p.gold=30;g.p.life=2;
  const Page townPages[]={Page::Village,Page::Shop,Page::Buy,Page::TownBag,Page::Character};
  const char* townNames[]={"vila","loja","compra","bolsa-vila","personagem"};
  for(int i=0;i<5;++i){v.page=townPages[i];v.message="";v.choice=0;render(c,g,v);c.save(root+"/"+townNames[i]+".ppm");}
  g=rpg::create(0,1);v.message="";
  for(int wins=0;wins<=3;++wins){g.ruinsWins=wins;v.page=Page::Ruins;render(c,g,v);c.save(root+"/mapa-"+std::to_string(wins)+".ppm");}
  g.guardianDefeated=true;render(c,g,v);c.save(root+"/mapa-vencido.ppm");
  for(int id=0;id<4;++id){rpg::begin(g,id);v.page=Page::Battle;render(c,g,v);c.save(root+"/inimigo-"+std::to_string(id)+".ppm");g.phase=rpg::Phase::Home;}
  g=rpg::create(0,4);g.ruinsWins=3;rpg::begin(g,3);v.page=Page::Battle;
  for(int id=0;id<4;++id){g.enemyId=id;g.enemyHp=rpg::enemySpec(id).hp;for(unsigned frame=0;frame<8;++frame){render(c,g,v,frame);c.save(root+"/sprite-"+std::to_string(id)+"-"+std::to_string(frame)+".ppm");}}
  for(int effect=1;effect<=4;++effect){v.effect=Effect(effect);v.effectOnHero=effect==3;for(unsigned frame=0;frame<8;++frame){v.effectFrame=frame;render(c,g,v,frame);c.save(root+"/efeito-"+std::to_string(effect)+"-"+std::to_string(frame)+".ppm");}}
  v.effect=Effect::None;
  for(int cls=0;cls<4;++cls){g=rpg::create(cls,8);rpg::begin(g);v.page=Page::Battle;v.message="";
    for(unsigned frame=0;frame<6;++frame){v.heroFrame=frame;render(c,g,v,0);c.save(root+"/heroi-"+std::to_string(cls)+"-"+std::to_string(frame)+".ppm");}}
  v.heroFrame=0;
  // Equipment pages: every catalog name/tier/class and full equip/retire previews.
  g=rpg::create(0,8);g.p.gold=999999;v.message="";v.gearIndex=0;v.itemId=1;
  const Page gearPages[]={Page::Market,Page::Inventory,Page::GearShop,Page::GearBag,Page::GearBuy,Page::GearEquip};
  const char* gearNames[]={"mercado","inventario","equip-loja","equip-vazio","equip-compra","equip-confirmar"};
  for(int i=0;i<5;++i){v.page=gearPages[i];render(c,g,v);c.save(root+"/"+gearNames[i]+".ppm");}
  for(int id=1;id<=18;++id){g=rpg::create(id<=12?(id-1)/3:0,8);g.p.level=8;g.p.mp=g.p.maxmp=rpg::maxMana(g.p.cls,8);g.p.gold=999999;
    v.page=Page::GearShop;v.gearIndex=id<=12?(id-1)%3:id-10;render(c,g,v);c.save(root+"/equip-item-"+std::to_string(id)+".ppm");
    v.page=Page::GearBuy;v.itemId=id;render(c,g,v);
    assert(!rpg::buyGear(g,id));v.page=Page::GearEquip;render(c,g,v);
    assert(!rpg::equipGear(g,id));rpg::rest(g);v.page=Page::GearEquip;render(c,g,v);
  }
  g=rpg::create(0,8);g.p.level=8;g.p.mp=g.p.maxmp=rpg::maxMana(0,8);g.p.gold=999999;
  for(uint8_t id:{uint8_t(1),uint8_t(3),uint8_t(13),uint8_t(16),uint8_t(18)})assert(!rpg::buyGear(g,id));
  assert(!rpg::equipGear(g,1));v.page=Page::GearBag;v.gearIndex=0;render(c,g,v);c.save(root+"/equip-bolsa.ppm");
  v.page=Page::GearEquip;v.itemId=3;render(c,g,v);c.save(root+"/equip-confirmar.ppm");
  assert(!rpg::equipGear(g,18));rpg::rest(g);v.itemId=18;render(c,g,v);c.save(root+"/equip-retirar.ppm");
  v.itemId=16;render(c,g,v);c.save(root+"/equip-trocar.ppm");
  v.page=Page::GearShop;v.gearIndex=0;v.message="Classe ou nivel insuficiente";render(c,g,v);c.save(root+"/equip-erro.ppm");v.message="";
  g.p.level=99;g.p.atk=g.p.def=255;g.p.hp=g.p.maxhp=65535;g.p.mp=g.p.maxmp=119;g.p.gold=999999;g.p.xp=UINT32_MAX;
  for(Page page:gearPages){v.page=page;render(c,g,v);}
  g=rpg::create(0,73);g.p.level=8;g.p.mp=g.p.maxmp=rpg::totalMana(g);g.p.gold=999999;v.message="";
  v.page=Page::Forge;render(c,g,v);c.save(root+"/ferreiro.ppm");
  for(uint8_t slot=0;slot<3;++slot){v.page=Page::Upgrade;v.forgeSlot=slot;
    for(int tier=0;tier<4;++tier){g.forge[slot]=tier;g.p.mp=g.p.maxmp=rpg::totalMana(g);render(c,g,v);c.save(root+"/forja-"+std::to_string(slot)+"-"+std::to_string(tier)+".ppm");}
    g.forge[slot]=0;}
  v.forgeSlot=2;v.message="Ouro insuficiente";render(c,g,v);c.save(root+"/forja-sem-ouro.ppm");
  v.message="Nivel insuficiente";render(c,g,v);c.save(root+"/forja-sem-nivel.ppm");v.message="";
  g.p.level=99;g.p.atk=g.p.def=255;g.forge[0]=g.forge[1]=g.forge[2]=3;g.owned=(1u<<18)-1;g.equipped[0]=3;g.equipped[1]=15;g.equipped[2]=18;g.p.mp=g.p.maxmp=rpg::totalMana(g);
  v.forgeSlot=2;render(c,g,v);c.save(root+"/forja-mana-max.ppm");v.page=Page::Forge;render(c,g,v);v.page=Page::Character;render(c,g,v);
  g=rpg::create(0,73);v.message="";v.page=Page::Tavern;render(c,g,v);c.save(root+"/taverna.ppm");
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
  g=rpg::create(0,1);const Backdrop* seen[64]={};int count=0;
  for(int page=0;page<=int(Page::Updates);++page){const auto* bg=&backdropFor(Page(page),g);if(Page(page)==Page::Travel)continue;for(int i=0;i<count;++i)assert(seen[i]!=bg);seen[count++]=bg;}
  for(auto phase:{rpg::Phase::Won,rpg::Phase::Lost}){g.phase=phase;const auto* bg=&backdropFor(Page::Result,g);for(int i=0;i<count;++i)assert(seen[i]!=bg);seen[count++]=bg;}
  for(int id:{0,1,3}){g.enemyId=id;const auto* bg=&backdropFor(Page::Battle,g);for(int i=0;i<count;++i)assert(seen[i]!=bg);seen[count++]=bg;}
  assert(count==50);
  for(int i=0;i<count;++i){drawBackdrop(c,*seen[i]);for(int y=0;y<160;++y)for(int x=0;x<120;++x){uint16_t expected=seen[i]->palette()[seen[i]->pixels()[y*120+x]];assert(c.pixels[y*2*240+x*2]==expected&&c.pixels[(y*2+1)*240+x*2+1]==expected);}}
  // Largest legal display values must also fit on screen.
  g.p.level=99;g.p.hp=g.p.maxhp=65535;g.p.mp=g.p.maxmp=110;g.p.gold=999999;g.p.xp=UINT32_MAX;g.p.life=g.p.mana=99;
  for(Page page:townPages){v.page=page;render(c,g,v);}
  assert(hit(14,220,14,220,102)&&!hit(116,220,14,220,102)&&!hit(13,220,14,220,102));

  g=rpg::create(0,1);v.message="";menu.previews[0]=g;menu.slots[0]=rpg::Load::Ok;menu.slots[1]=rpg::Load::Blocked;
  for(Page page:{Page::Menu,Page::Slots,Page::SlotConfirm,Page::DeleteSlot,Page::Settings,Page::Wifi,Page::Keyboard,Page::Tests,Page::Map}){v.page=page;render(c,g,v);c.save(root+"/menu-"+std::to_string(int(page))+".ppm");}
  for(unsigned cls=0;cls<4;++cls){g=rpg::create(cls,1);for(unsigned a=0;a<4;++a)for(unsigned dest=0;dest<4;++dest){if(a==dest)continue;menu.journey=Journey{};assert(menu.journey.start(a,dest,0));v.page=Page::Travel;for(unsigned tick=0;tick<=3000;tick+=150){menu.journey.tick(tick);render(c,g,v,tick/120);} }}
  menu.journey=Journey{};menu.journey.start(0,3,0);v.page=Page::Travel;for(unsigned tick:{0u,750u,1500u,2250u,3000u}){menu.journey.tick(tick);render(c,g,v,tick/120);c.save(root+"/viagem-"+std::to_string(tick)+".ppm");}
  v.page=Page::Keyboard;for(unsigned mode=0;mode<3;++mode){menu.keyboard=mode;render(c,g,v);c.save(root+"/teclado-"+std::to_string(mode)+".ppm");}
  for(unsigned race=0;race<4;++race)for(unsigned cls=0;cls<4;++cls){menu.draftRace=race;v.choice=cls;g=rpg::create(cls,1);g.race=race;for(unsigned color=0;color<8;++color){menu.draftShirt=g.shirt=color;menu.draftPants=g.trousers=7-color;v.page=Page::Clothes;render(c,g,v,3);c.save(root+"/raca-"+std::to_string(race)+"-classe-"+std::to_string(cls)+"-cor-"+std::to_string(color)+".ppm");}}
  for(Page page:{Page::Race,Page::Guild,Page::GuildJoin,Page::GuildMissions,Page::Club,Page::ClubBattle,Page::ClubResult}){v.page=page;v.message="";render(c,g,v);c.save(root+"/nova-"+std::to_string(int(page))+".ppm");}
  v.page=Page::Keyboard;for(unsigned mode=0;mode<3;++mode)for(unsigned pg=0;pg<3;++pg){menu.keyboard=mode;menu.keyPage=pg;render(c,g,v);c.save(root+"/keyboard-"+std::to_string(mode)+"-"+std::to_string(pg)+".ppm");}
  // Stable clothing preview, even during former attack-frame ticks.
  v.page=Page::Clothes;render(c,g,v,0);std::vector<uint16_t> stable(c.pixels,c.pixels+240*320);render(c,g,v,5);assert(std::equal(stable.begin(),stable.end(),c.pixels));
  v.page=Page::Updates;menu.connected=true;render(c,g,v);assert(c.pixels[15*240+229]==0x07e0);c.save(root+"/atualizacao.ppm");
  for(unsigned state=0;state<=unsigned(updater::State::Error);++state){updateInfo.state=updater::State(state);updateInfo.busy=state==1||(state>=4&&state<=8);updateInfo.progress=57;strcpy(updateInfo.version,"2026.10.06-ota1");strcpy(updateInfo.message,"Mantenha a alimentacao");render(c,g,v);c.save(root+"/update-"+std::to_string(state)+".ppm");}menu.connected=false;updateInfo=updater::Info{};
  makeFallback();v.page=Page::Battle;render(c,g,v);c.save(root+"/sem-cartao.ppm");
  puts("PASS: shared firmware renderer; text bounds; 299+ screens; 50 distinct backgrounds; 1008 travel frames; full-frame pixel parity; optional card states, contracts, forge, equipment and combat.");
}

