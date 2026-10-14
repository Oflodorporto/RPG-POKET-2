#include "TestHero.h"
#include "../firmware/RPG_POKET_2/Persistence.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
using namespace rpg;
struct Cut{};
struct Store{
 bool ready=true;std::map<std::string,std::vector<uint8_t>> blobs;
 unsigned op=0,cut=0,fail=0;bool after=false,lostAck=false;
 void before(){++op;if(cut==op&&!after)throw Cut{};}
 void end(){if(cut==op&&after)throw Cut{};}
 void clearFault(){op=cut=fail=0;after=lostAck=false;}
 Read read(const char* key,uint8_t* b,unsigned n){before();if(op==fail){end();return Read::Error;}auto it=blobs.find(key);Read r=Read::Missing;if(it!=blobs.end()){if(it->second.size()>n)r=Read::Error;else if(it->second.size()!=n)r=Read::Corrupt;else{memcpy(b,it->second.data(),n);r=Read::Ok;}}end();return r;}
 bool write(const char* key,const uint8_t* b,unsigned n){before();bool bad=op==fail;if(!bad||lostAck)blobs[key]={b,b+n};end();return !bad;}
 Read presence(const char* key){before();auto r=op==fail?Read::Error:blobs.count(key)?Read::Ok:Read::Missing;end();return r;}
 bool erase(const char* key){before();bool bad=op==fail;if(!bad||lostAck)blobs.erase(key);end();return !bad;}
};
using Backend=PersistentSlotBackend<Store>;using J=PersistentJournal<Backend>;
std::vector<uint8_t> packed(const Game& g,uint32_t seq=1,uint8_t slot=0){std::vector<uint8_t>b(PERSISTENT_SAVE_SIZE);encodePersistent(g,seq,slot,b.data());return b;}
bool same(const Game& a,const Game& b){return packed(a)==packed(b);}
Game hero(unsigned s){auto g=create(s%4,501+s);g.race=s%4;g.shirt=(s+2)%8;g.trousers=(s+5)%8;g.fatigue=30+s;g.rations=5;g.p.gold=100+s;return g;}
unsigned checks=0,cuts=0,faults=0;
void seed(Store& s,unsigned slot,const Game& g){Backend b(s);b.slot=slot;J j(b);Game h;assert(j.load(h)==Load::Empty&&j.save(g));auto older=g;older.p.gold-=1;assert(j.save(older)&&j.save(g));s.clearFault();}
void assertOthers(const Store& a,const Store& b,unsigned slot){for(unsigned x=0;x<3;++x)if(x!=slot){Backend io(const_cast<Store&>(a));io.slot=x;for(const char* k:{io.key(false),io.key(true),io.metaKey(),io.guideKey()}){auto p=a.blobs.find(k),q=b.blobs.find(k);assert((p==a.blobs.end())==(q==b.blobs.end()));if(p!=a.blobs.end())assert(p->second==q->second);}}}
void verifySaveCuts(const Store& start,unsigned slot,const Game& old,const Game& next,bool empty){
 Store count=start;Backend b(count);b.slot=slot;J j(b);Game loaded;assert(j.load(loaded)==(empty?Load::Empty:Load::Ok));count.clearFault();assert(j.save(next));unsigned total=count.op;
 for(unsigned k=1;k<=total;++k)for(bool after:{false,true}){
  Store s=start;Backend io(s);io.slot=slot;J live(io);assert(live.load(loaded)==(empty?Load::Empty:Load::Ok));s.clearFault();s.cut=k;s.after=after;try{live.save(next);}catch(const Cut&){}s.clearFault();J boot(io);auto result=boot.load(loaded);
  assert(result==Load::Ok||result==Load::Empty);if(result==Load::Ok)assert(same(loaded,next)||(!empty&&same(loaded,old)));else assert(empty);
  assertOthers(start,s,slot);++cuts;
 }
 for(unsigned k=1;k<=total;++k)for(bool lost:{false,true}){
  Store s=start;Backend io(s);io.slot=slot;J live(io);assert(live.load(loaded)==(empty?Load::Empty:Load::Ok));s.clearFault();s.fail=k;s.lostAck=lost;bool saved=live.save(next);s.clearFault();J boot(io);auto result=boot.load(loaded);assert(result==Load::Ok||result==Load::Empty);if(saved)assert(result==Load::Ok&&same(loaded,next));else if(result==Load::Ok)assert(same(loaded,next)||(!empty&&same(loaded,old)));assertOthers(start,s,slot);
  // Retry through the SAME object; a lost acknowledgement must not erase the
  // replacement or charge/update the game a second time.
  assert(live.save(next));J again(io);assert(again.load(loaded)==Load::Ok&&same(loaded,next));++faults;
 }
}
// Daily travel is serialized field by field; no legacy trip surrogate may
// weaken combat, event, resource or exclusion invariants during validation.
uint32_t seedForRoll(unsigned roll){for(uint32_t seed=1;;++seed){Game probe;probe.randomState=seed;if(1+random(probe)%20==roll)return seed;}}
Game dailyHero(unsigned cls,unsigned city,unsigned supplies){
 auto g=create(cls,117+cls);g.p.xp=dndXp[19];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;
 g.tutorial=true;g.p.gold=1000;g.city=city;g.rations=9;g.charts=supplies&1?2:0;g.charms=supplies&2?2:0;g.sleepKit=supplies&4;
 g.race=cls;g.shirt=cls+2;g.trousers=7-cls;g.fatigue=95;return g;
}
std::vector<Game> dailySamples;
std::vector<std::pair<Game,Game>> dailyTransitions;
unsigned dailyMask=0,dailyChecks=0;
void dailySnapshot(const Game& g){
 assert(persistentValid(g));assert(g.tripStage?(!valid(g)&&g.tripStage==4):valid(g));
 auto bytes=packed(g,42,2);Game restored=hero(0);uint32_t n=0;
 const auto& d=g.daily;const uint8_t fields[]={uint8_t(d.phase),d.origin,d.totalDays,d.day,d.rationsPaid,d.nightsDone,d.region,d.roll,d.difficulty,d.survivalBonus,d.luckBonus,d.kit,d.total,d.enemy,d.supplies,d.result,d.policy,d.routeVersion};
 assert(sizeof(fields)==18&&!memcmp(bytes.data()+125,fields,sizeof(fields))&&!bytes[143]&&get32(bytes.data(),144)==SAVE_RESET_EPOCH&&bytes[152]==2);
 assert(decodePersistent(bytes.data(),2,restored,n)==Decode::Ok&&n==42&&same(g,restored));
 auto left=g,right=restored;assert(random(left)==random(right)&&same(left,right));
 if(g.tripStage){dailyMask|=1u<<unsigned(g.daily.phase);bool found=false;
  for(const auto& old:dailySamples)if(old.daily.phase==g.daily.phase&&old.phase==g.phase&&old.daily.result==g.daily.result)found=true;
  if(!found)dailySamples.push_back(g);
 }
 ++dailyChecks;
}
template<class Step>void dailyTransition(Game& g,Step step,bool capture){
 const auto before=g;auto resumed=g;auto bytes=packed(g,23);uint32_t n=0;
 assert(decodePersistent(bytes.data(),0,resumed,n)==Decode::Ok);
 assert(step(g)&&step(resumed)&&same(g,resumed));dailySnapshot(g);
 if(capture)dailyTransitions.push_back({before,g});
}
void dailyJourney(Game g,unsigned destination,unsigned mode,bool capture=false){
 const unsigned origin=g.city;const bool escort=g.eventStage==2&&g.eventKind==2;
 // Cover a persisted unsafe original road roll and all three night outcomes.
 g.randomState=seedForRoll(mode?1:20);assert(!prepareDailyTrip(g,destination));dailySnapshot(g);
 dailyTransition(g,[](Game& h){return acceptDailyRoad(h);},capture);
 while(g.tripStage){
  if(dailyBattle(g)){
   // Preserve ordinary combat turns, rewards and terminal phases byte-exactly.
   dailyTransition(g,[](Game& h){return !act(h,Action::Defensive);},capture);
   if(g.phase==Phase::Enemy)dailyTransition(g,[](Game& h){enemy(h);return true;},capture);
   if(g.phase==Phase::Hero){
    bool night=g.daily.phase==DailyPhase::NightBattle;
    dailyTransition(g,[=](Game& h){if(night&&mode==3){h.p.hp=0;finish(h);}else if(night&&mode==2)h.phase=Phase::Fled;else {h.enemyHp=0;finish(h);}return true;},capture);
   }
   dailyTransition(g,[](Game& h){return resolveDailyBattle(h);},capture);
  }else if(g.daily.phase==DailyPhase::Leg){
   g.randomState=seedForRoll(mode?1:20);
   dailyTransition(g,[](Game& h){return finishDailyLeg(h);},capture);
  }else if(g.daily.phase==DailyPhase::NightRoll){
   dailyTransition(g,[](Game& h){return acceptDailyNight(h);},capture);
  }else if(g.daily.phase==DailyPhase::Rest){
   dailyTransition(g,[](Game& h){return finishDailyRest(h);},capture);
  }else if(g.daily.phase==DailyPhase::Tent){
   dailyTransition(g,[](Game& h){return continueDailyTrip(h);},capture);
  }else assert(false);
 }
 assert(g.city==((mode==3||(mode==2&&escort))?origin:destination));assert(dailyStateEmpty(g.daily));
}
void dailyCodecTests(){
 for(unsigned cls=0;cls<4;++cls)for(unsigned from=0;from<4;++from)for(unsigned to=0;to<4;++to)if(from!=to)
  for(unsigned supplies=0;supplies<8;++supplies)for(unsigned mode=0;mode<4;++mode)
   dailyJourney(dailyHero(cls,from,supplies),to,mode,cls==0&&from==0&&to==1&&supplies==7);
 assert(dailyMask==0xfe);
 // Active letter escort battles retain the true event and combat state.
 for(unsigned level:{5u,10u,18u})for(unsigned mode=0;mode<4;++mode){
  auto g=create(0,42);g.p.xp=dndXp[level-1];levelUp(g);g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;g.tutorial=true;g.rations=9;g.sleepKit=true;
  assert(offerEvent(g,20733,10));g.eventKind=2;assert(!acceptEvent(g,49));
  dailyJourney(g,eventDestination(g),mode);
 }
 for(const auto& original:dailySamples){
  auto bytes=packed(original,51);Game h;uint32_t n=0;
  for(unsigned offset=125;offset<=142;++offset)for(unsigned value=0;value<256;++value){
   auto changed=bytes;changed[offset]=value;put32(changed.data(),156,crc(changed.data(),156));
   const auto status=decodePersistent(changed.data(),0,h,n);
   if(status==Decode::Ok)assert(persistentValid(h)&&!valid(h)&&packed(h,n)==changed);
   ++dailyChecks;
  }
  // Daily validation cannot bypass any ordinary battle/resource invariants.
  std::vector<Game> invalid;
  auto bad=original;bad.p.hp=bad.p.maxhp+1;invalid.push_back(bad);
  bad=original;bad.p.maxmp++;invalid.push_back(bad);
  bad=original;bad.owned=1u<<GEAR_COUNT;invalid.push_back(bad);
  bad=original;bad.questProgress=1;invalid.push_back(bad);
  bad=original;bad.attributes[0]=21;invalid.push_back(bad);
  bad=original;bad.rations=10;invalid.push_back(bad);
  bad=original;bad.eventStage=7;invalid.push_back(bad);
  bad=original;bad.dungeonFlags=1;invalid.push_back(bad);
  bad=original;bad.discovery=1;invalid.push_back(bad);
  bad=original;bad.campStage=1;invalid.push_back(bad);
  bad=original;bad.campaignStage=1;invalid.push_back(bad);
  for(const auto& state:invalid){assert(!persistentValid(state));auto changed=packed(state);assert(decodePersistent(changed.data(),0,h,n)==Decode::Corrupt);++dailyChecks;}
 }
 // Save29 never admits a legacy single-roll journey, even when its frozen
 // Save28 representation remains completely valid.
 for(unsigned stage=1;stage<=3;++stage){auto old=dailyHero(0,0,0);old.randomState=seedForRoll(stage==2?1:20);assert(!prepareTrip(old,1));if(stage>1)assert(acceptTrip(old));assert(old.tripStage==stage&&valid(old)&&!persistentValid(old));auto bytes=packed(old);Game h;uint32_t n=0;assert(decodePersistent(bytes.data(),0,h,n)==Decode::Corrupt);}
 // Daily None means every extension byte is zero. Future policy/route versions
 // block the journal rather than silently falling back to an older side.
 auto idle=hero(0);auto clean=packed(idle);Game h;uint32_t n=0;
 for(unsigned offset=125;offset<=142;++offset){auto changed=clean;changed[offset]=1;put32(changed.data(),156,crc(changed.data(),156));assert(decodePersistent(changed.data(),0,h,n)!=Decode::Ok);++dailyChecks;}
 for(unsigned offset:{141u,142u,143u})for(bool malformedDay:{false,true}){
  Store s;seed(s,0,idle);auto changed=packed(dailySamples.front(),4);changed[offset]=2;if(malformedDay)changed[128]=0;put32(changed.data(),156,crc(changed.data(),156));s.blobs["b"]=changed;
  Backend io(s);J journal(io);const auto before=s.blobs;assert(journal.load(h)==Load::Blocked&&!journal.save(idle)&&before==s.blobs);++dailyChecks;
 }
 // Crash/retry coverage includes ration debit, committed dice, combat results,
 // recovery and arrival. A reboot can reveal only one complete state.
 for(const auto& step:dailyTransitions){Store s;for(unsigned slot=0;slot<3;++slot)seed(s,slot,slot==1?step.first:hero(slot));verifySaveCuts(s,1,step.first,step.second,false);}
 printf("PASS Save29 daily: %u checks, %zu distinct phase/outcome snapshots, %zu journal transitions\n",dailyChecks,dailySamples.size(),dailyTransitions.size());
}

int main(){
 dailyCodecTests();
 static_assert(PERSISTENT_SAVE_SIZE==160&&CURRENT_SAVE_FORMAT==30&&SAVE_SIZE==128&&LEGACY_SAVE_FORMAT==28,"Separate formats");
 for(unsigned slot=0;slot<3;++slot)for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race)for(unsigned shirt=0;shirt<8;++shirt)for(unsigned pants=0;pants<8;++pants){
  auto g=create(cls,151+slot);g.race=race;g.shirt=shirt;g.trousers=pants;g.fatigue=(shirt*13+pants)%101;auto b=packed(g,UINT32_MAX,slot);Game h;uint32_t seq=0;assert(decodePersistent(b.data(),slot,h,seq)==Decode::Ok&&same(g,h)&&seq==UINT32_MAX);assert(decodePersistent(b.data(),(slot+1)%3,h,seq)==Decode::Unsupported);++checks;
 }
 // Per-turn snapshots retain every field and exactly the same next RNG/action.
 for(unsigned cls=0;cls<4;++cls)for(unsigned seedN=1;seedN<=150;++seedN){auto g=hero(cls);g.randomState=seedN;begin(g,4);for(unsigned turn=0;turn<120;++turn){if(g.phase==Phase::Hero){if(act(g,Action(turn%6)))act(g,Action::Attack);}else if(g.phase==Phase::Enemy)enemy(g);else break;auto b=packed(g,turn,cls%3);Game h;uint32_t seq;assert(decodePersistent(b.data(),cls%3,h,seq)==Decode::Ok&&same(g,h));if(g.phase==Phase::Enemy){auto a=g,c=h;enemy(a);enemy(c);assert(same(a,c));}++checks;}}
 auto g=hero(0);auto clean=packed(g);Game h;uint32_t seq;
 for(unsigned off=0;off<PERSISTENT_SAVE_SIZE;++off)for(unsigned value=0;value<256;value+=17){auto b=clean;b[off]=value;if(off>=8&&off<156)put32(b.data(),156,crc(b.data(),156));auto result=decodePersistent(b.data(),0,h,seq);if(result==Decode::Ok){assert(persistentValid(h));assert(packed(h,seq)==b);}++checks;}
 for(unsigned version=1;version<=28;++version){auto b=clean;put16(b.data(),4,version);put32(b.data(),156,crc(b.data(),156));assert(decodePersistent(b.data(),0,h,seq)==Decode::Unsupported);}
 for(unsigned discriminator=0;discriminator<7;++discriminator){auto bad=g;if(discriminator==0)bad.p.cls=255;if(discriminator==1)bad.p.level=0;if(discriminator==2)bad.p.level=255;if(discriminator==3)bad.enemyId=255;if(discriminator==4)bad.phase=Phase(255);if(discriminator==5)bad.race=255;if(discriminator==6)bad.fatigue=255;assert(!persistentValid(bad));++checks;}
 Store base;for(unsigned s=0;s<3;++s)seed(base,s,hero(s));
 for(unsigned slot=0;slot<3;++slot){Backend io(base);io.slot=slot;J j(io);assert(j.load(h)==Load::Ok&&same(h,hero(slot)));auto next=h;next.p.gold+=17;--next.rations;next.fatigue+=4;verifySaveCuts(base,slot,h,next,false);
  Store empty=base;Backend e(empty);e.slot=slot;assert(e.remove());empty.clearFault();verifySaveCuts(empty,slot,Game{},next,true);
  Store fresh;for(unsigned other=0;other<3;++other)if(other!=slot)seed(fresh,other,hero(other));verifySaveCuts(fresh,slot,Game{},next,true);
  Store damaged=base;Backend d(damaged);d.slot=slot;auto good=damaged.blobs[d.key(false)];
  damaged.blobs[d.key(false)][20]^=1;J recover(d);assert(recover.load(h)==Load::Recovered&&h.fatigue==hero(slot).fatigue&&h.p.gold==hero(slot).p.gold-1);
  damaged.blobs[d.key(true)][20]^=1;auto before=damaged.blobs;J bad(d);assert(bad.load(h)==Load::Blocked&&!bad.save(next)&&damaged.blobs==before);
  for(auto kind:{0,1,2}){damaged=base;auto& bytes=damaged.blobs[d.key(false)];if(kind==0)put16(bytes.data(),4,31);if(kind==1)put32(bytes.data(),144,2);if(kind==2)bytes[152]=(slot+1)%3;put32(bytes.data(),156,crc(bytes.data(),156));before=damaged.blobs;J future(d);assert(future.load(h)==Load::Blocked&&!future.save(next)&&before==damaged.blobs);}
  damaged=base;damaged.blobs.erase(d.metaKey());before=damaged.blobs;J missing(d);assert(missing.load(h)==Load::Blocked&&!missing.save(next)&&before==damaged.blobs);
  damaged=base;damaged.blobs[d.metaKey()][2]^=1;before=damaged.blobs;J corrupt(d);assert(corrupt.load(h)==Load::Blocked&&!corrupt.save(next)&&before==damaged.blobs);
  damaged=base;damaged.blobs.erase(d.key(false));damaged.blobs.erase(d.key(true));J absent(d);assert(absent.load(h)==Load::Blocked);
  // Deletion is the explicit recovery path; hidden leftovers never revive.
  damaged=base;Store count=damaged;Backend cb(count);cb.slot=slot;count.clearFault();assert(cb.remove());unsigned ops=count.op;
  for(unsigned k=1;k<=ops;++k)for(bool after:{false,true}){Store s=base;Backend del(s);del.slot=slot;s.cut=k;s.after=after;try{del.remove();}catch(const Cut&){}s.clearFault();J boot(del);auto result=boot.load(h);assert(result==Load::Ok||result==Load::Empty);if(result==Load::Ok)assert(same(h,hero(slot)));assertOthers(base,s,slot);++cuts;}
 }
 // Equal sequence with different bytes and half-range ordering are ambiguous.
 for(unsigned mode=0;mode<2;++mode){Store s;Backend b(s);seed(s,0,g);auto one=packed(g,9);auto different=g;++different.p.gold;auto two=packed(different,mode?9+0x80000000u:9);s.blobs["a"]=one;s.blobs["b"]=two;J j(b);assert(j.load(h)==Load::Blocked);}
 // Wrap is valid, but a stale observer cannot overwrite another object's save.
 {Store s;Backend b(s);seed(s,0,g);s.blobs["a"]=packed(g,UINT32_MAX);s.blobs["b"]=packed(g,UINT32_MAX-1);J j(b),stale(b);assert(j.load(h)==Load::Ok&&stale.load(h)==Load::Ok);auto next=g;++next.p.gold;assert(j.save(next)&&j.seq==0);assert(!stale.save(g));J boot(b);assert(boot.load(h)==Load::Ok&&same(h,next));}
 // Same-object compensation after an uncertain committed write preserves the
 // commit, then writes a fresh sequence on the other side instead of erasing it.
 {Store s;Backend b(s);seed(s,0,g);J j(b);assert(j.load(h)==Load::Ok);auto next=g;++next.p.gold;Store probe=s;Backend pb(probe);J pj(pb);assert(pj.load(h)==Load::Ok);probe.clearFault();assert(pj.save(next));unsigned lastRead=probe.op;s.clearFault();s.fail=lastRead;assert(!j.save(next));s.clearFault();uint32_t old=j.seq;assert(j.save(g)&&j.seq==old+2);J boot(b);assert(boot.load(h)==Load::Ok&&same(h,g));}
 {Store s;Backend b(s);J j(b);assert(j.load(h)==Load::Empty);b.slot=1;auto before=s.blobs;assert(!j.save(g)&&s.blobs==before);}
 {Store s;Backend b(s);J j(b);assert(j.load(h)==Load::Empty);s.ready=false;auto before=s.blobs;assert(!j.save(g)&&!b.remove()&&before==s.blobs);}
 printf("PASS Save29: %u codec/action checks, %u power cuts, %u error/ACK retries; 3 slots, clothing, RNG, lifecycle, corruption, wrap, compensation\n",checks,cuts,faults);
 return 0;
}
