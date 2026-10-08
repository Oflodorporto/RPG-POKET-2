#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
struct Memory {
  uint8_t data[2][SAVE_SIZE]{};bool exists[2]={},fail=false;
  Read read(const char* k,uint8_t* out){int i=*k=='b';if(!exists[i])return Read::Missing;memcpy(out,data[i],SAVE_SIZE);return Read::Ok;}
  bool write(const char* k,const uint8_t* in){if(fail)return false;int i=*k=='b';memcpy(data[i],in,SAVE_SIZE);exists[i]=true;return true;}
};
void same(const Game& a,const Game& b){uint8_t x[rpg::SAVE_SIZE],y[rpg::SAVE_SIZE];encode(a,7,x);encode(b,7,y);assert(!memcmp(x,y,rpg::SAVE_SIZE));}

// Legacy equipment regressions select a city that actually sells the item.
const char* buyStockGear(rpg::Game& g,uint8_t id){uint8_t old=g.city;if(!rpg::cityStock(g,id))g.city=3;auto err=rpg::buyGear(g,id);g.city=old;return err;}
int main(){
  for(int cls=0;cls<4;++cls)for(uint8_t slot=0;slot<3;++slot){
    auto g=testHero(cls,42);g.p.gold=10000;uint16_t base=slot==0?40:slot==1?45:35;
    for(int tier=0;tier<3;++tier){
      assert(forgePrice(g,slot)==base*(tier+1)*(tier+1));
      if(tier){g.p.level=forgeRequirement(tier)-1;g.p.maxmp=totalMana(g);g.p.mp=0;auto old=g;assert(upgradeForge(g,slot));same(g,old);}
      g.p.level=tier==0?3:forgeRequirement(tier);g.p.maxmp=totalMana(g);g.p.mp=0;
      uint32_t gold=g.p.gold;auto price=forgePrice(g,slot);g.p.gold=price-1;auto old=g;assert(upgradeForge(g,slot));same(g,old);g.p.gold=gold;
      assert(!upgradeForge(g,slot));assert(g.forge[slot]==tier+1&&g.p.gold==gold-price&&g.p.mp==0&&valid(g));
      assert(effectiveAttack(g)==g.p.atk+(slot==0?(tier+1)*2:0));assert(effectiveDefense(g)==g.p.def+(slot==1?(tier+1)*3:0));
      assert(g.p.maxmp==maxMana(cls,g.p.level)+(slot==2?(tier+1)*3:0));
      uint8_t bytes[rpg::SAVE_SIZE];encode(g,99,bytes);Game copy;uint32_t seq;assert(decode(bytes,copy,seq)==Decode::Ok&&seq==99);same(g,copy);
    }
    auto old=g;assert(upgradeForge(g,slot)&&!forgePrice(g,slot));same(g,old);
    for(int phase=1;phase<=5;++phase){g.phase=Phase(phase);old=g;assert(upgradeForge(g,slot));same(g,old);}
  }
  auto g=testHero(0,2);auto old=g;assert(upgradeForge(g,3)&&upgradeForge(g,255)&&!forgePrice(g,255));same(g,old);
  // Permanent slot bonus survives removal/swapping of catalog items.
  g.p.level=8;g.p.gold=10000;g.p.mp=g.p.maxmp=maxMana(0,8);
  for(uint8_t slot=0;slot<3;++slot)for(int tier=0;tier<3;++tier)assert(!upgradeForge(g,slot));
  for(uint8_t id:{uint8_t(3),uint8_t(15),uint8_t(18)})assert(!buyStockGear(g,id)&&!equipGear(g,id));
  assert(effectiveAttack(g)==18&&effectiveDefense(g)==22&&g.p.maxmp==37);rest(g);assert(g.p.mp==37);
  assert(!equipGear(g,18)&&g.p.maxmp==28&&g.p.mp==28);assert(!equipGear(g,18)&&g.p.maxmp==37&&g.p.mp==28);
  g.p.atk=g.p.def=255;assert(effectiveAttack(g)==267&&effectiveDefense(g)==273&&valid(g));
  // v3 equipment save in all phases: no field lost; permanent bonuses start at zero.
  for(int phase=0;phase<=5;++phase){
    g=testHero(1,73);g.p.level=8;g.p.mp=g.p.maxmp=maxMana(1,8);g.p.gold=1000;g.ruinsWins=3;g.guardianDefeated=true;
    for(uint8_t id:{uint8_t(6),uint8_t(15),uint8_t(18)})assert(!buyStockGear(g,id)&&!equipGear(g,id));
    g.phase=Phase(phase);if(phase==3)g.enemyHp=0;if(phase==4)g.p.hp=0;
    uint8_t bytes[rpg::SAVE_SIZE];encode(g,12,bytes);put16(bytes,4,3);rpg::put16(bytes,6,64);bytes[58]=0;put32(bytes,60,crc(bytes,60));
    Memory mem;mem.exists[0]=true;memcpy(mem.data[0],bytes,rpg::SAVE_SIZE);Journal<Memory> j(mem);Game loaded;assert(j.load(loaded)==Load::Ok);same(g,loaded);
    mem.fail=true;assert(!j.save(loaded)&&get16(mem.data[0],4)==3);mem.fail=false;assert(j.save(loaded)&&get16(mem.data[1],4)==15);
    Journal<Memory> boot(mem);assert(boot.load(loaded)==Load::Ok);same(g,loaded);
    if(phase==2){enemy(g);enemy(loaded);same(g,loaded);}
  }
  // Save failure/reboot exposes complete old or new purchase, never another charge.
  Memory mem;Journal<Memory> j(mem);Game loaded;assert(j.load(loaded)==Load::Empty);g=testHero(0,7);g.p.gold=100;assert(j.save(g));old=g;
  assert(!upgradeForge(g,0));mem.fail=true;assert(!j.save(g));Journal<Memory> boot(mem);assert(boot.load(loaded)==Load::Ok);same(old,loaded);
  mem.fail=false;assert(j.save(g));boot.load(loaded);same(g,loaded);assert(g.p.gold==60&&g.forge[0]==1);
  mem.data[j.active][58]^=1;Journal<Memory> recovered(mem);assert(recovered.load(loaded)==Load::Recovered);same(old,loaded);
  for(uint8_t a=0;a<4;++a)for(uint8_t b=0;b<4;++b)for(uint8_t c=0;c<4;++c){
    g=testHero(0,1);g.p.level=8;g.forge[0]=a;g.forge[1]=b;g.forge[2]=c;g.p.mp=g.p.maxmp=totalMana(g);
    uint8_t encoded[rpg::SAVE_SIZE];encode(g,3,encoded);uint32_t sequence;assert(valid(g)&&decode(encoded,loaded,sequence)==Decode::Ok);same(g,loaded);
  }
  g=testHero(0,3);g.p.gold=100;assert(!upgradeForge(g,2));assert(!buyStockGear(g,16)&&!equipGear(g,16));g.p.xp=120;begin(g);g.enemyHp=0;finish(g);
  assert(g.p.level==4&&g.p.maxmp==maxMana(0,4)+6&&g.forge[2]==1&&valid(g));
  g.forge[0]=4;assert(!valid(g));g=testHero(0,1);g.forge[0]=2;assert(!valid(g));
  uint8_t bytes[rpg::SAVE_SIZE];encode(testHero(0,1),1,bytes);bytes[58]=64;put32(bytes,92,crc(bytes,92));uint32_t seq;assert(decode(bytes,loaded,seq)==Decode::Corrupt);
  // Damage parity with Heltec: base + permanent + catalog, same RNG order.
  for(int cls=0;cls<4;++cls)for(uint32_t seed=1;seed<=100;++seed){
    g=testHero(cls,seed);g.p.level=8;g.p.maxmp=totalMana(g);g.p.gold=10000;
    for(int tier=0;tier<3;++tier){assert(!upgradeForge(g,0));assert(!upgradeForge(g,1));}
    assert(!buyStockGear(g,cls*3+3)&&!equipGear(g,cls*3+3));assert(!buyStockGear(g,15)&&!equipGear(g,15));g.ruinsWins=3;begin(g,3);
    auto oracle=g;int damage=rollDamage(oracle,int(g.p.atk)+12,6);bool dodge=random(oracle)%100<6;
    act(g,Action::Attack);assert(g.enemyHp==28-(dodge?0:std::min(28,damage)));
    if(g.phase==Phase::Enemy){oracle=g;int hp=g.p.hp;damage=rollDamage(oracle,6,int(g.p.def)+18);dodge=random(oracle)%100<6;enemy(g);assert(g.p.hp==hp-(dodge?0:std::min(hp,damage)));}
  }
  puts("PASS: forge prices/levels/caps, all classes/slots, no mana refill, stacked bonuses/swaps, v3 migration in six phases, atomic retry/reboot, invalid saves and 400 combat comparisons.");
}
