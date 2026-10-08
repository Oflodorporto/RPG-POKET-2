#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstdio>
using namespace rpg;
struct Memory {
  uint8_t data[2][rpg::SAVE_SIZE]{};bool exists[2]={},fail=false;
  Read read(const char* k,uint8_t* out){int i=*k=='b';if(!exists[i])return Read::Missing;memcpy(out,data[i],rpg::SAVE_SIZE);return Read::Ok;}
  bool write(const char* k,const uint8_t* in){if(fail)return false;int i=*k=='b';memcpy(data[i],in,rpg::SAVE_SIZE);exists[i]=true;return true;}
};
void same(const Game& a,const Game& b){uint8_t x[rpg::SAVE_SIZE],y[rpg::SAVE_SIZE];encode(a,1,x);encode(b,1,y);assert(!memcmp(x,y,rpg::SAVE_SIZE));}
void win(Game& g,uint8_t enemyId){assert(begin(g,enemyId));g.enemyHp=0;finish(g);assert(g.phase==Phase::Won);assert(home(g));}
int main(){
  for(uint8_t id=1;id<=3;++id){
    auto g=testHero(0,11);g.city=1;g.ruinsWins=3;win(g,2);assert(!g.questId);assert(!acceptQuest(g,id)&&g.questLevel==3&&!g.questProgress);
    auto old=g;assert(acceptQuest(g,id));same(g,old);assert(claimQuest(g));same(g,old);
    // Unrelated enemy, loss, flee and repeated finish never advance a specific contract.
    if(id>1){win(g,0);assert(!g.questProgress);}
    auto target=contract(id).enemy<0?0:contract(id).enemy;
    for(int count=0;count<contract(id).count;++count){
      assert(begin(g,target));g.enemyHp=0;finish(g);assert(g.questProgress==count+1);old=g;finish(g);same(g,old);home(g);
    }
    assert(questComplete(g));old=g;assert(abandonQuest(g));same(g,old);
    auto gold=g.p.gold;auto xp=g.p.xp;auto reward=contractGold(id,3),rewardXp=contractXp(id,3);
    assert(!claimQuest(g)&&g.p.gold==gold+reward&&!g.questId&&!g.questProgress&&!g.questLevel);assert(g.p.xp==xp+rewardXp);
    old=g;assert(claimQuest(g));same(g,old);assert(!acceptQuest(g,id)&&!g.questProgress);
    assert(!abandonQuest(g)&&!g.questId);
  }
  assert(contractXp(1,3)==30&&contractXp(2,3)==42&&contractXp(3,3)==72);
  assert(contractGold(1,3)==19&&contractGold(2,3)==22&&contractGold(3,3)==39);
  for(uint8_t level=3;level<=99;++level)for(uint8_t id=1;id<=3;++id){assert(contractXp(id,level)==uint32_t(xpNeeded(level))*contract(id).percent/100);assert(contractGold(id,level)==contract(id).gold+(level-1)*2);}
  auto g=testHero(1,7);assert(!acceptQuest(g,1));assert(begin(g));g.p.hp=0;finish(g);assert(!g.questProgress);home(g);rest(g);
  begin(g);g.phase=Phase::Fled;home(g);assert(!g.questProgress);
  for(int phase=1;phase<=5;++phase){g.phase=Phase(phase);auto old=g;assert(acceptQuest(g,2)&&claimQuest(g)&&abandonQuest(g));same(g,old);}
  g=testHero(0,1);assert(acceptQuest(g,0)&&acceptQuest(g,4));
  g.p.gold=1000;assert(!upgradeForge(g,2)&&!buyGear(g,16)&&!equipGear(g,16));assert(!acceptQuest(g,1));g.questProgress=3;g.p.xp=119;
  assert(!claimQuest(g)&&g.p.level==4&&g.p.xp==29&&g.p.maxmp==maxMana(0,4)+6&&valid(g));
  g=testHero(0,1);assert(!acceptQuest(g,3));g.questProgress=1;g.p.level=4;g.p.maxmp=totalMana(g);auto xp=contractXp(g.questId,g.questLevel);assert(xp==72);
  g.p.gold=999999;auto old=g;assert(claimQuest(g));same(g,old);g.p.gold=999999-39;assert(!claimQuest(g)&&g.p.gold==999999);
  // Atomic accept/progress/claim retry and reboot. No doubled XP/gold on retry.
  Memory mem;Journal<Memory> j(mem);Game loaded;assert(j.load(loaded)==Load::Empty);g=testHero(0,5);g.city=1;assert(j.save(g));old=g;
  assert(!acceptQuest(g,1));mem.fail=true;assert(!j.save(g));Journal<Memory> reboot(mem);assert(reboot.load(loaded)==Load::Ok);same(old,loaded);
  mem.fail=false;assert(j.save(g));for(int n=0;n<3;++n){win(g,0);assert(j.save(g));reboot.load(loaded);same(g,loaded);}
  old=g;assert(!claimQuest(g));mem.fail=true;assert(!j.save(g));reboot.load(loaded);same(old,loaded);
  mem.fail=false;assert(j.save(g));reboot.load(loaded);same(g,loaded);assert(!g.questId&&g.p.gold==31&&g.p.xp==54);
  // v1..4 migration in all phases, with v3 equipment and v4 permanent upgrades.
  for(int version=1;version<=4;++version)for(int phase=0;phase<=5;++phase){
    g=testHero(1,123);g.p.gold=1000;
    if(version>=3){assert(!buyGear(g,4)&&!equipGear(g,4));assert(!buyGear(g,16)&&!equipGear(g,16));}
    if(version>=4)assert(!upgradeForge(g,2));g.phase=Phase(phase);if(phase==3)g.enemyHp=0;if(phase==4)g.p.hp=0;
    uint8_t bytes[rpg::SAVE_SIZE];encode(g,11,bytes);put16(bytes,4,version);rpg::put16(bytes,6,64);put32(bytes,60,crc(bytes,60));uint32_t seq;
    assert(decode(bytes,loaded,seq)==Decode::Ok&&seq==11);same(g,loaded);assert(!loaded.questId&&!loaded.questProgress&&!loaded.questLevel);
    if(phase==2){enemy(g);enemy(loaded);same(g,loaded);}
  }
  // Contract fields and gear bits coexist through all possible mission progress values.
  for(uint8_t id=1;id<=3;++id)for(uint8_t progress=0;progress<=contract(id).count;++progress){
    g=testHero(0,13);g.p.level=8;g.p.maxmp=totalMana(g);g.p.gold=10000;g.owned=(1u<<18)-1;g.equipped[0]=3;g.equipped[1]=15;g.equipped[2]=18;
    g.forge[0]=g.forge[1]=g.forge[2]=3;g.p.maxmp=totalMana(g);assert(!acceptQuest(g,id));g.questProgress=progress;
    uint8_t bytes[rpg::SAVE_SIZE];encode(g,4,bytes);uint32_t seq;assert(valid(g)&&decode(bytes,loaded,seq)==Decode::Ok);same(g,loaded);
  }
  for(int bad=0;bad<4;++bad){g=testHero(0,3);g.questId=2;g.questLevel=3;if(bad==0)g.questProgress=3;if(bad==1)g.questLevel=4;if(bad==2)g.questId=0;if(bad==3)g.questLevel=0;assert(!valid(g));}
  puts("PASS: three contracts, fixed level rewards, correct enemy/progress, no retroactive/duplicate wins, loss/flee, abandon/caps/level-up, atomic retry/reboot, 24 legacy migrations and packed gear/quest coexistence.");
}
