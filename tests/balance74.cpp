#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/Campaign.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
using namespace rpg;
int main(int argc,char** argv){
 const unsigned levels[]={1,3,5,8,12,15,18,20};unsigned cases=0;
 for(auto level:levels)for(unsigned id=0;id<29;++id){auto g=create(2,13);g.p.xp=dndXp[level-1];levelUp(g);g.enemyId=id;unsigned xp=combatXp(g);assert(xp>=1&&xp<=3276);g.city=3;assert(combatXp(g)==xp);g.city=0;assert(combatXp(g)==xp);if(level<=enemyRewardLevel(id)+2)assert(xp==combatXpBase(id));++cases;}
 auto g=create(2,17);g.guildMember=true;assert(combatXpBase(0)==20&&combatXpBase(1)==40&&combatXpBase(5)==500&&combatXpBase(7)==2666);
 assert(contractXp(g,4,1)==200&&contractXp(g,4,12)==10&&contractXp(g,13,18)==10000&&contractXp(g,13,20)==10000);
 assert(contractGold(g,4,1)==18&&contractGold(g,4,18)==18&&contractGold(g,13,18)==119);
 for(unsigned id=1;id<=15;++id)for(unsigned l=1;l<20;++l)assert(contractXp(g,id,l+1)<=contractXp(g,id,l));
 g.p.xp=100000;levelUp(g);g.enemyId=0;assert(g.p.level==12&&combatXp(g)==1);g.enemyId=7;assert(combatXp(g)==2666);g.enemyId=5;assert(combatXp(g)==15);
 // Updating does not recalculate existing receipts/progress or touch the serialized format.
 g.questId=4;g.questProgress=3;g.questLevel=12;g.p.gold=12345;g.owned=64;g.equipped[0]=7;g.phase=Phase::Won;g.enemyId=0;g.enemyHp=0;g.gainXp=80;g.gainGold=4;
 uint8_t before[SAVE_SIZE],after[SAVE_SIZE];assert(valid(g));encode(g,42,before);Game boot;uint32_t seq;assert(decode(before,boot,seq)==Decode::Ok);encode(boot,seq,after);assert(!memcmp(before,after,SAVE_SIZE));auto xp=totalExperience(boot);finish(boot);assert(totalExperience(boot)==xp&&boot.gainXp==80);assert(home(boot));assert(!claimQuest(boot)&&totalExperience(boot)==xp+10&&boot.p.gold==12363);xp=totalExperience(boot);assert(claimQuest(boot)&&totalExperience(boot)==xp);
 // Legacy slots retain historical reward formulas. No free XP in PvP.
 Game legacy;legacy.enemyId=10;assert(combatXp(legacy)==1);legacy.enemyId=0;assert(combatXp(legacy)==8&&contractXp(legacy,1,3)==30&&campaignXp(legacy,1)==100);
 auto duel=create(2,23);duel.guildMember=true;duel.p.gold=100;duel.p.xp=6500;levelUp(duel);assert(!reserveFight(duel,123));assert(startFight(duel,123));assert(settleFight(duel,123,1)&&totalExperience(duel)==6500&&!settleFight(duel,123,1));
 for(unsigned kind=0;kind<8;++kind){auto e=create(2,31);e.eventLevel=1;e.eventKind=kind;assert(eventXp(e)==200);e.eventLevel=18;assert(eventXp(e)==((kind==2||kind==3||kind==5||kind==7)?12000:8000));}
 if(argc>1){std::ofstream out(argv[1]);assert(out);out<<"hero_level,enemy,challenge,old_kill_xp,new_kill_xp,gold,old_ideal_kills,new_ideal_kills,old_xp_hour,new_xp_hour,old_hunt_bonus,new_hunt_bonus,old_hunt_xp_hour,new_hunt_xp_hour\n";
 // Explicit pacing model: 30s per won encounter; regional hunt (actual kill count) +60s navigation, no losses/rests.
 for(auto level:levels)for(unsigned id=0;id<29;++id){auto h=create(2,42);h.p.xp=dndXp[level-1];levelUp(h);h.enemyId=id;unsigned city=enemyRewardLevel(id)<5?0:enemyRewardLevel(id)<10?1:enemyRewardLevel(id)<18?2:3,q=contractOffer(city,0),old=enemySpec(id).xp*10,now=combatXp(h),oldBonus=unsigned(dndXpNeeded(level))*contract(q).percent/100,bonus=contractXp(h,q,level);out<<level<<','<<id<<','<<enemyRewardLevel(id)<<','<<old<<','<<now<<','<<unsigned(enemySpec(id).gold)<<','<<(level==20?0:old?double(dndXpNeeded(level))/old:0)<<','<<(level==20?0:double(dndXpNeeded(level))/now)<<','<<old*120<<','<<now*120<<','<<oldBonus<<','<<bonus<<','<<(old*contract(q).count+oldBonus)*3600/(contract(q).count*30+60)<<','<<(now*contract(q).count+bonus)*3600/(contract(q).count*30+60)<<'\n';}}
 printf("PASS: balance74 %u level/enemy combinations, fixed difficulty, positive floor, contract abuse, Save28 byte identity, once-only claim, legacy and PvP preservation.\n",cases);
}
