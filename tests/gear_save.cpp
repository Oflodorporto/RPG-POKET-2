#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
struct Memory {
  uint8_t bytes[2][SAVE_SIZE]{};bool exists[2]={},fail=false;
  Read read(const char* key,uint8_t* out){int i=*key=='b';if(!exists[i])return Read::Missing;memcpy(out,bytes[i],SAVE_SIZE);return Read::Ok;}
  bool write(const char* key,const uint8_t* in){if(fail)return false;int i=*key=='b';memcpy(bytes[i],in,SAVE_SIZE);exists[i]=true;return true;}
};
void same(const Game& a,const Game& b){uint8_t x[SAVE_SIZE],y[SAVE_SIZE];encode(a,7,x);encode(b,7,y);assert(!memcmp(x,y,SAVE_SIZE));}

// Legacy equipment regressions select a city that actually sells the item.
const char* buyStockGear(rpg::Game& g,uint8_t id){uint8_t old=g.city;if(!rpg::cityStock(g,id))g.city=3;auto err=rpg::buyGear(g,id);g.city=old;return err;}
int main(){
  assert(!gearId(0)&&!gearId(19)&&!gearId(255));
  assert(gearBonus(0)==0&&gearBonus(255)==0&&gearSlot(0)==255);
  assert(!gearOwns(UINT32_MAX,0)&&!gearOwns(UINT32_MAX,255));
  assert(!gearOffer(4,0)&&!gearOffer(0,9));
  for(int cls=0;cls<4;++cls){
    for(int ix=0;ix<9;++ix){uint8_t id=gearOffer(cls,ix);assert(gearId(id));assert(gearFamily(id)==(ix<3?cls:ix<6?4:5));assert(gearTier(id)==ix%3+1);}
    for(uint8_t id=1;id<=18;++id){
      auto g=testHero(cls,73);g.city=3;g.p.gold=1000;auto original=g;
      if(gearFamily(id)<4&&gearFamily(id)!=cls){assert(buyStockGear(g,id));same(g,original);continue;}
      if(gearLevel(id)>3){assert(buyStockGear(g,id));same(g,original);}
      g.p.level=gearLevel(id)>3?gearLevel(id):3;g.p.mp=g.p.maxmp=maxMana(cls,g.p.level);
      uint8_t tier=gearTier(id),slot=gearSlot(id);assert(gearPrice(id)==30*tier*tier);assert(gearBonus(id)==tier*(slot==0?2:3));
      g.p.gold=cityGearPrice(g,id)-1;original=g;assert(buyStockGear(g,id));same(g,original);
      g.p.gold=cityGearPrice(g,id);assert(!buyStockGear(g,id)&&!g.p.gold&&gearOwns(g.owned,id));
      original=g;assert(buyStockGear(g,id));same(g,original);
      auto mp=g.p.mp;assert(!equipGear(g,id));assert(g.equipped[slot]==id&&g.p.atk==original.p.atk&&g.p.def==original.p.def);
      assert(effectiveAttack(g)==original.p.atk+(slot==0?gearBonus(id):0));
      assert(effectiveDefense(g)==original.p.def+(slot==1?gearBonus(id):0));
      assert(g.p.maxmp==maxMana(cls,g.p.level)+(slot==2?gearBonus(id):0)&&g.p.mp==mp&&valid(g));
      uint8_t data[SAVE_SIZE];encode(g,99,data);Game restored;uint32_t seq;assert(decode(data,restored,seq)==Decode::Ok&&seq==99);same(g,restored);
      assert(!equipGear(g,id));assert(!g.equipped[slot]&&gearOwns(g.owned,id)&&g.p.mp==mp&&valid(g));
    }
  }
  auto g=testHero(0,17);auto original=g;assert(equipGear(g,1));same(g,original);
  g.p.level=8;g.p.maxmp=maxMana(0,8);g.p.mp=0;g.p.gold=1000;
  for(uint8_t id:{uint8_t(1),uint8_t(3),uint8_t(13),uint8_t(16),uint8_t(18)})assert(!buyStockGear(g,id));
  assert(gearOwnedCount(g.owned)==5&&gearOwnedAt(g.owned,0)==1&&gearOwnedAt(g.owned,4)==18&&!gearOwnedAt(g.owned,5));
  assert(!equipGear(g,1)&&!equipGear(g,3)&&g.equipped[0]==3&&gearOwns(g.owned,1));
  g.p.atk=g.p.def=255;assert(effectiveAttack(g)==261);assert(!equipGear(g,13)&&effectiveDefense(g)==258);
  assert(!equipGear(g,18)&&g.p.mp==0&&g.p.maxmp==28);rest(g);assert(g.p.mp==28);
  assert(!equipGear(g,16)&&g.p.mp==22&&g.p.maxmp==22);assert(!equipGear(g,16)&&g.p.mp==19&&g.p.maxmp==19);
  assert(!equipGear(g,18)&&g.p.mp==19&&g.p.maxmp==28);assert(valid(g));
  // Invalid IDs, ownership, slot, class, level and mana caps are never accepted as saves.
  original=g;for(int bad=0;bad<7;++bad){g=original;
    if(bad==0)g.owned|=1u<<18;
    if(bad==1)g.equipped[0]=255;
    if(bad==2)g.equipped[0]=2;
    if(bad==3)g.equipped[1]=1;
    if(bad==4){g.owned|=1u<<3;g.equipped[0]=4;}
    if(bad==5)g.p.level=3;
    if(bad==6)g.p.maxmp=19;
    assert(!valid(g));uint8_t bytes[SAVE_SIZE];encode(g,1,bytes);Game restored;uint32_t seq;
    assert(decode(bytes,restored,seq)==Decode::Corrupt);
  }g=original;
  // Each equipment operation is rejected during every non-Home phase, with no mutation.
  for(int phase=1;phase<=5;++phase){g.phase=Phase(phase);original=g;assert(buyStockGear(g,2));same(g,original);assert(equipGear(g,3));same(g,original);}
  // Version 1/2 migration preserves every existing field, including all six phases.
  for(int version=1;version<=2;++version)for(int phase=0;phase<=5;++phase){
    g=testHero(1,45);g.tutorial=true;g.p.gold=230;g.p.xp=99;g.p.life=3;g.p.mana=7;
    if(version==2){g.ruinsWins=3;g.guardianDefeated=true;g.enemyId=3;g.enemyHp=28;}
    g.phase=Phase(phase);if(phase==3)g.enemyHp=0;if(phase==4)g.p.hp=0;
    uint8_t bytes[SAVE_SIZE];encode(g,18,bytes);put16(bytes,4,version);rpg::put16(bytes,6,64);memset(bytes+51,0,9);
    if(version==1)memset(bytes+48,0,12);put32(bytes,60,crc(bytes,60));
    Memory mem;mem.exists[0]=true;memcpy(mem.bytes[0],bytes,SAVE_SIZE);Journal<Memory> j(mem);Game loaded;
    assert(j.load(loaded)==Load::Ok);g.seenEnemies=(g.phase==Phase::Home?0:1u<<g.enemyId)|(g.guardianDefeated?1u<<3:0);same(g,loaded);assert(!loaded.owned&&!loaded.equipped[0]&&!loaded.equipped[1]&&!loaded.equipped[2]);
    mem.fail=true;assert(!j.save(loaded));assert(get16(mem.bytes[0],4)==version);
    mem.fail=false;assert(j.save(loaded)&&get16(mem.bytes[1],4)==27);Journal<Memory> boot(mem);assert(boot.load(loaded)==Load::Ok);same(g,loaded);
    if(g.phase==Phase::Enemy){enemy(g);enemy(loaded);same(g,loaded);}
  }
  // Atomic purchase/save retry never charges twice; reboot sees the whole old/new state.
  Memory mem;Journal<Memory> j(mem);Game loaded;assert(j.load(loaded)==Load::Empty);
  g=testHero(0,7);g.p.gold=30;assert(j.save(g));original=g;assert(!buyStockGear(g,1));mem.fail=true;assert(!j.save(g));
  Journal<Memory> oldBoot(mem);assert(oldBoot.load(loaded)==Load::Ok);same(original,loaded);
  mem.fail=false;assert(j.save(g));Journal<Memory> newBoot(mem);assert(newBoot.load(loaded)==Load::Ok);same(g,loaded);
  assert(!equipGear(g,1));assert(j.save(g));newBoot.load(loaded);same(g,loaded);
  // Equipment damage uses the Heltec formulas with bonuses, without extra RNG draws.
  for(int cls=0;cls<4;++cls)for(uint32_t seed=1;seed<=100;++seed){
    g=testHero(cls,seed);g.p.level=8;g.p.maxmp=maxMana(cls,8);g.p.gold=1000;
    assert(!buyStockGear(g,cls*3+3)&&!equipGear(g,cls*3+3));assert(!buyStockGear(g,15)&&!equipGear(g,15));g.ruinsWins=3;begin(g,3);
    auto oracle=g;int damage=rollDamage(oracle,int(g.p.atk)+6,enemySpec(3).def);bool dodge=random(oracle)%100<6;
    act(g,Action::Attack);assert(g.enemyHp==28-(dodge?0:std::min(28,damage))&&g.randomState==oracle.randomState);
    if(g.phase==Phase::Enemy){oracle=g;int hp=g.p.hp;damage=rollDamage(oracle,enemySpec(3).atk,int(g.p.def)+9);dodge=random(oracle)%100<6;
      enemy(g);assert(g.p.hp==hp-(dodge?0:std::min(hp,damage))&&g.randomState==oracle.randomState);}
  }
  g=testHero(0,1);g.p.gold=100;assert(!buyStockGear(g,16)&&!equipGear(g,16));g.p.xp=120;g.p.atk=255;begin(g);g.enemyHp=0;finish(g);
  assert(g.p.level==4&&g.p.maxmp==maxMana(0,4)+3&&valid(g));
  puts("PASS: catalog/prices/class/levels, purchase/equip/remove/swap, mana clamp/no refill, 12 legacy migrations, atomic retry/reboot, invalid saves, 400 equipped battles, level-up mana bonus.");
}
