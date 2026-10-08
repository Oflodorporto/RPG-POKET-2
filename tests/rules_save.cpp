#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/TouchGate.h"
#include <cassert>
#include "../firmware/RPG_POKET_2/CombatFx.h"
#include <cstdio>
using namespace rpg;
struct Memory {
  uint8_t data[2][SAVE_SIZE]{};bool exists[2]={},failWrite=false,failRead=false;
  Read read(const char* k,uint8_t* out){int i=*k=='b';if(failRead)return Read::Error;if(!exists[i])return Read::Missing;memcpy(out,data[i],SAVE_SIZE);return Read::Ok;}
  bool write(const char* k,const uint8_t* in){if(failWrite)return false;int i=*k=='b';memcpy(data[i],in,SAVE_SIZE);exists[i]=true;return true;}
};
void same(const Game& a,const Game& b){uint8_t x[SAVE_SIZE],y[SAVE_SIZE];encode(a,7,x);encode(b,7,y);assert(!memcmp(x,y,SAVE_SIZE));}
int main(){
  assert(xpNeeded(1)==40&&xpNeeded(3)==120&&xpNeeded(99)==65535);
  assert(recovery(1,18,false)==5&&recovery(17,18,false)==1&&recovery(18,18,false)==0&&recovery(0,1,false)==1);
  for(int cls=0;cls<4;++cls){
    Game g=testHero(cls,42);assert(valid(g));assert(g.p.life==0&&g.p.mana==2&&g.p.level==3);begin(g);
    auto old=g;g.p.mp=0;auto noMana=g;assert(act(g,Action::Offensive));same(g,noMana);
    g=old;assert(act(g,Action::Life));same(g,old);assert(act(g,Action::Mana));same(g,old);
    g.p.hp=1;assert(!act(g,Action::Defensive));assert(g.guard==(cls<2?75:50));assert(g.p.hp==(cls==3?7:1));assert(g.phase==Phase::Enemy);enemy(g);assert(!g.guard);
    g=old;g.p.mp=0;assert(!act(g,Action::Mana));assert(g.p.mana==1&&g.p.mp==g.p.maxmp/2&&g.phase==Phase::Enemy);
  }
  // The old Heltec formulas are the parity oracle, including RNG call order,
  // precision avoiding the dodge draw and guard rounding upwards.
  for(int cls=0;cls<4;++cls)for(uint32_t seed=1;seed<=200;++seed){
    auto g=testHero(cls,seed);begin(g);auto oracle=g;
    int d=std::max(1,int(g.p.atk)+int(random(oracle)%3)-(cls==0?0:4)/3);
    bool crit=random(oracle)%100<12;if(crit)d+=d>>1;
    d=cls==0?d*180/100:cls==3?d*2:d*150/100;
    bool dodge=cls!=2&&random(oracle)%100<6;
    act(g,Action::Offensive);assert(g.enemyHp==17-(dodge?0:std::min(17,d)));assert(g.crit==crit&&g.dodge==dodge);
  }
  auto g=testHero(0,51);begin(g);g.p.xp=118;g.enemyHp=1;g.p.atk=255;
  for(int i=0;g.phase==Phase::Hero&&i<20;++i){act(g,Action::Attack);if(g.phase==Phase::Enemy)enemy(g);}
  assert(g.phase==Phase::Won&&g.p.level==4&&g.p.maxhp==20&&g.p.gold==6&&g.p.xp==10);
  auto won=g;assert(act(g,Action::Attack));assert(!enemy(g));same(g,won);
  assert(home(g));assert(rest(g));assert(g.p.hp==g.p.maxhp&&g.p.mp==g.p.maxmp);
  g=testHero(0,3);begin(g);g.phase=Phase::Enemy;g.p.hp=1;g.p.xp=2;
  for(int i=0;g.phase!=Phase::Lost&&i<100;++i){g.phase=Phase::Enemy;enemy(g);}
  assert(g.phase==Phase::Lost&&g.p.xp==0&&g.gainXp==2);assert(home(g)&&g.p.hp==1);
  bool escaped=false,failed=false;
  for(uint32_t s=1;s<200;++s){g=testHero(0,s);begin(g);act(g,Action::Flee);escaped|=g.phase==Phase::Fled;failed|=g.phase==Phase::Enemy;assert(g.p.gold==0&&g.p.xp==0);}assert(escaped&&failed);
  // Exercise whole battles, every class/action and serialization on each turn.
  for(int cls=0;cls<4;++cls)for(uint32_t seed=1;seed<=400;++seed){
    g=testHero(cls,seed);g.p.life=3;begin(g);
    for(int t=0;t<200;++t){
      if(g.phase==Phase::Hero){auto e=act(g,Action((seed+t)%6));if(e)act(g,Action::Attack);}
      else if(g.phase==Phase::Enemy)enemy(g);else break;
      assert(valid(g));uint8_t b[SAVE_SIZE];encode(g,123,b);Game copy;uint32_t seq=0;assert(decode(b,copy,seq)==Decode::Ok&&seq==123);same(g,copy);
      if(g.phase==Phase::Enemy){Game next1=g,next2=copy;enemy(next1);enemy(next2);same(next1,next2);}
    }
    assert(g.phase==Phase::Won||g.phase==Phase::Lost||g.phase==Phase::Fled);
  }
  Memory mem;Journal<Memory> j(mem);Game loaded;assert(j.load(loaded)==Load::Empty);
  g=testHero(0,99);assert(j.save(g));auto checkpoint=g;begin(g);assert(j.save(g));
  Journal<Memory> reboot(mem);assert(reboot.load(loaded)==Load::Ok);same(g,loaded);
  mem.failWrite=true;auto beforeSeq=reboot.seq;assert(!reboot.save(g)&&reboot.seq==beforeSeq);mem.failWrite=false;
  mem.data[1][22]^=1;Journal<Memory> recovery(mem);assert(recovery.load(loaded)==Load::Recovered);same(checkpoint,loaded);
  assert(recovery.save(g));Journal<Memory> r2(mem);assert(r2.load(loaded)==Load::Ok);same(g,loaded);
  mem.data[0][4]=14;Journal<Memory> future(mem);assert(future.load(loaded)==Load::Blocked&&!future.save(g));
  Memory corrupt;corrupt.exists[0]=true;Journal<Memory> bad(corrupt);assert(bad.load(loaded)==Load::Blocked&&!bad.save(g));
  Memory inaccessible;inaccessible.failRead=true;Journal<Memory> broken(inaccessible);assert(broken.load(loaded)==Load::Blocked);
  // Readback failure must not be reported as successful even if NVS wrote.
  struct ReadbackFail:Memory{bool write(const char* k,const uint8_t* b){bool ok=Memory::write(k,b);failRead=true;return ok;}} rb;
  Journal<ReadbackFail> rbj(rb);assert(rbj.load(loaded)==Load::Empty);assert(!rbj.save(g)&&rbj.seq==0);
  rb.failRead=false;Journal<ReadbackFail> rbboot(rb);assert(rbboot.load(loaded)==Load::Ok);same(g,loaded);
  TouchGate gate;assert(!gate.update(true,false,0));assert(!gate.update(true,false,61));assert(gate.update(true,true,70));
  assert(!gate.update(false,false,100));assert(!gate.update(true,true,200));assert(!gate.update(true,false,210));assert(!gate.update(true,false,271));assert(gate.update(true,true,300));
  // Heltec shop prices, exact payment, limits, outside-combat inventory.
  assert(potionPrice(false)==10&&potionPrice(true)==12);
  for(int item=0;item<2;++item){
    g=testHero(0,1);auto empty=g;assert(buyPotion(g,item));same(g,empty);
    g.p.gold=potionPrice(item);assert(!buyPotion(g,item));assert(g.p.gold==0&&(item?g.p.mana:g.p.life)==(item?3:1));
    auto fullHp=g;assert(usePotionAtHome(g,item));same(g,fullHp);
    if(item)g.p.mp=1;else g.p.hp=1;
    assert(!usePotionAtHome(g,item));assert((item?g.p.mp:g.p.hp)==(item?8:6));assert(g.phase==Phase::Home);
    g.p.gold=100;if(item)g.p.mana=99;else g.p.life=99;auto capped=g;assert(buyPotion(g,item));same(g,capped);
    begin(g);auto fight=g;assert(buyPotion(g,item)&&usePotionAtHome(g,item));same(g,fight);
  }
  Memory purchases;Journal<Memory> pj(purchases);assert(pj.load(loaded)==Load::Empty);g=testHero(0,2);g.p.gold=30;assert(pj.save(g));
  assert(!buyPotion(g,false));purchases.failWrite=true;assert(!pj.save(g));assert(g.p.gold==20&&g.p.life==1);
  purchases.failWrite=false;assert(pj.save(g));Journal<Memory> purchaseBoot(purchases);assert(purchaseBoot.load(loaded)==Load::Ok);same(g,loaded);
  assert(pj.save(g));Journal<Memory> retryBoot(purchases);assert(retryBoot.load(loaded)==Load::Ok&&loaded.p.gold==20&&loaded.p.life==1);
  // Migration from each version-1 phase preserves the exact legacy payload.
  for(int phase=0;phase<6;++phase){
    auto old=testHero(1,123);old.p.gold=37;old.p.life=4;old.tutorial=true;old.phase=Phase(phase);
    if(old.phase==Phase::Won){old.enemyHp=0;old.gainGold=6;old.gainXp=12;}
    if(old.phase==Phase::Lost)old.p.hp=0;
    uint8_t bytes[SAVE_SIZE];encode(old,19,bytes);put16(bytes,4,1);rpg::put16(bytes,6,64);memset(bytes+48,0,12);put32(bytes,60,crc(bytes,60));
    Memory legacy;legacy.exists[0]=true;memcpy(legacy.data[0],bytes,SAVE_SIZE);
    Journal<Memory> migration(legacy);assert(migration.load(loaded)==Load::Ok);same(old,loaded);
    assert(loaded.enemyId==2&&loaded.ruinsWins==0&&!loaded.guardianDefeated);
    legacy.failWrite=true;assert(!migration.save(loaded));assert(!memcmp(bytes,legacy.data[0],SAVE_SIZE));
    legacy.failWrite=false;assert(migration.save(loaded));assert(get16(legacy.data[1],4)==13);
    Journal<Memory> migrated(legacy);assert(migrated.load(g)==Load::Ok);same(old,g);
    if(old.phase==Phase::Enemy){enemy(old);enemy(g);same(old,g);}
  }
  // Gate does not consume RNG; each win advances once; boss rewards and retry.
  g=testHero(2,54);auto locked=g;assert(!explore(g,true));same(g,locked);
  bool seen[8]={};for(int seed=1;seed<=200;++seed){g=testHero(0,seed);assert(explore(g));assert(g.enemyId==1||g.enemyId==4);seen[g.enemyId]=true;}
  assert(seen[1]&&seen[4]);
  g=testHero(2,7);g.city=1;g.p.atk=255;g.p.def=255;
  for(int id=0;id<4;++id){
    assert(begin(g,id));assert(g.enemyHp==enemySpec(id).hp);auto gold=g.p.gold;
    for(int turn=0;turn<100&&g.phase!=Phase::Won;++turn){if(g.phase==Phase::Hero)act(g,Action::Attack);else enemy(g);}
    assert(g.phase==Phase::Won&&g.p.gold==gold+enemySpec(id).gold&&g.gainXp==enemySpec(id).xp);
    assert(g.ruinsWins==std::min(3,id+1)&&g.guardianDefeated==(id==3));
    auto result=g;finish(g);same(result,g);
    uint8_t bytes[SAVE_SIZE];encode(g,12,bytes);uint32_t seq;assert(decode(bytes,loaded,seq)==Decode::Ok);same(g,loaded);
    assert(home(g));rest(g);
  }
  assert(g.guardianDefeated&&g.p.life>=1&&g.p.mana>=3);assert(explore(g,true));
  g=testHero(0,1);g.enemyId=10;assert(!valid(g));g=testHero(0,1);g.ruinsWins=4;assert(!valid(g));
  // Every enemy uses its own attack/defense, and losing/fleeing grants no progress.
  for(int id=0;id<4;++id)for(uint32_t seed=1;seed<=100;++seed){
    g=testHero(1,seed);g.ruinsWins=3;assert(begin(g,id));auto oracle=g;
    int damage=rollDamage(oracle,g.p.atk,enemySpec(id).def);bool dodge=random(oracle)%100<6;
    act(g,Action::Attack);assert(g.enemyHp==enemySpec(id).hp-(dodge?0:std::min<int>(enemySpec(id).hp,damage)));
    if(g.phase==Phase::Enemy){oracle=g;int hp=g.p.hp;damage=rollDamage(oracle,enemySpec(id).atk,g.p.def);dodge=random(oracle)%100<6;enemy(g);assert(g.p.hp==hp-(dodge?0:std::min(hp,damage)));}
  }
  for(int id=0;id<4;++id){
    g=testHero(0,2);g.ruinsWins=id==3?3:0;begin(g,id);auto wins=g.ruinsWins;
    g.p.hp=0;finish(g);assert(g.phase==Phase::Lost&&g.ruinsWins==wins&&!g.guardianDefeated);
    home(g);rest(g);begin(g,id);for(int t=0;t<100&&g.phase!=Phase::Fled;++t){g.phase=Phase::Hero;act(g,Action::Flee);}
    assert(g.phase==Phase::Fled&&g.ruinsWins==wins&&!g.guardianDefeated);
  }
  // Fast release now rearms at 30 ms; shorter jitter and invalid samples do not.
  TouchGate fast;assert(!fast.update(true,false,100));assert(!fast.update(true,false,130));assert(fast.update(true,true,138));
  for(uint32_t t=146;t<3000;t+=8)assert(!fast.update(true,true,t));
  assert(!fast.update(true,false,3000));assert(!fast.update(true,false,3024));assert(!fast.update(true,true,3025));
  assert(!fast.update(true,false,3040));assert(!fast.update(false,false,3050));assert(!fast.update(true,false,3060));assert(!fast.update(true,true,3080));
  assert(!fast.update(true,false,3090));assert(!fast.update(true,false,3120));assert(fast.update(true,true,3128));
  TouchGate wrap;assert(!wrap.update(true,false,UINT32_MAX-10));assert(!wrap.update(true,false,20));assert(wrap.update(true,true,28));
  CombatFx fx;assert(!fx.active());fx.start(Effect::Slash,false,UINT32_MAX-100);assert(fx.active()&&fx.frame(UINT32_MAX-40)==1);assert(!fx.expire(378));assert(fx.expire(379)&&!fx.active());
  puts("PASS: 30ms release, held press, short release jitter, invalid samples, timer wrap; nonblocking effect lifetime.");
  puts("PASS: ruins roster, 3-win gate, all enemies, rewards exactly once, boss persistence, v1 migration of all 6 phases, failed migration write, resumed legacy enemy turn.");
  puts("PASS: Heltec formulas, 1600 battles, four classes, skills/potions/escape/results, restart, CRC, journal fallback, unknown version, I/O failures, held touch; shop prices/limits, outside-combat potion, atomic purchase and save retry.");
}

