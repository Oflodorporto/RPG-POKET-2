#include "Persistence.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <fstream>
using namespace rpg;
using Bytes=std::vector<uint8_t>;
struct Cut{};
struct Store{
 bool ready=true;std::map<std::string,Bytes> blobs;unsigned op=0,cut=0,fail=0;bool after=false,lostAck=false;
 void before(){++op;if(cut==op&&!after)throw Cut{};}void end(){if(cut==op&&after)throw Cut{};}
 void clearFault(){op=cut=fail=0;after=lostAck=false;}
 Read read(const char*k,uint8_t*b,unsigned n){before();if(op==fail){end();return Read::Error;}auto it=blobs.find(k);Read r=Read::Missing;if(it!=blobs.end()){if(it->second.size()>n)r=Read::Error;else if(it->second.size()!=n)r=Read::Corrupt;else{memcpy(b,it->second.data(),n);r=Read::Ok;}}end();return r;}
 bool write(const char*k,const uint8_t*b,unsigned n){before();bool bad=op==fail;if(!bad||lostAck)blobs[k]={b,b+n};end();return !bad;}
 Read presence(const char*k){before();auto r=op==fail?Read::Error:blobs.count(k)?Read::Ok:Read::Missing;end();return r;}
 bool erase(const char*k){before();bool bad=op==fail;if(!bad||lostAck)blobs.erase(k);end();return !bad;}
};
using B=PersistentSlotBackend<Store>;using J=PersistentJournal<B>;
Bytes packed(const Game&g,unsigned seq=1,unsigned slot=0,unsigned format=30){Bytes b(160);encodePersistentVersion(g,seq,slot,b.data(),format);return b;}
bool same(const Game&a,const Game&b){return packed(a)==packed(b);}
#include "geometry.inc"
unsigned cuts=0,faults=0;
void checkOthers(const Store&a,const Store&b,unsigned slot){B io(const_cast<Store&>(a));io.slot=slot;for(const auto& row:a.blobs)if(row.first!=io.key(false)&&row.first!=io.key(true))assert(b.blobs.at(row.first)==row.second);}
void verify(const Store&initial,unsigned slot,const Game&old,const Game&next){
 assert(persistentValid(old)&&persistentValid(next));Store count=initial;B b(count);b.slot=slot;J j(b);Game h;assert(j.load(h)==Load::Ok&&same(h,old));count.clearFault();assert(j.save(next));unsigned total=count.op;
 for(unsigned k=1;k<=total;++k)for(bool after:{false,true}){Store s=initial;B b(s);b.slot=slot;J j(b);assert(j.load(h)==Load::Ok);s.clearFault();s.cut=k;s.after=after;try{j.save(next);}catch(const Cut&){}s.clearFault();J boot(b);assert(boot.load(h)==Load::Ok);assert(same(h,old)||same(h,next));checkOthers(initial,s,slot);++cuts;}
 for(unsigned k=1;k<=total;++k)for(bool lost:{false,true}){Store s=initial;B b(s);b.slot=slot;J j(b);assert(j.load(h)==Load::Ok);s.clearFault();s.fail=k;s.lostAck=lost;bool ok=j.save(next);s.clearFault();J boot(b);assert(boot.load(h)==Load::Ok);assert(ok?same(h,next):(same(h,next)||same(h,old)));assert(j.save(next));J again(b);assert(again.load(h)==Load::Ok&&same(h,next));checkOthers(initial,s,slot);++faults;}
}
Store seeded(const Game&g,unsigned format=29){Store s;for(unsigned slot=0;slot<3;++slot){B b(s);b.slot=slot;uint8_t life[16];encodeSlotLife(slot,SlotLife::Live,life);s.blobs[b.metaKey()]={life,life+16};s.blobs[b.key(false)]=packed(g,16,slot,format);s.blobs[b.key(true)]=packed(g,17,slot,format);s.blobs[b.guideKey()]={2,3,4,5};}s.blobs["epoch-sentinel"]={1,2,3};return s;}
int main(){
 static_assert(PERSISTENT_SAVE_FORMAT==30&&PERSISTENT_SAVE_SIZE==160&&SAVE_RESET_EPOCH==1,"Schema");
 std::ifstream f("save29-fixtures.bin",std::ios::binary);assert(f);Bytes all{std::istreambuf_iterator<char>(f),{}};assert(all.size()%172==0);unsigned count=0,active=0;Game legacy;
 for(unsigned offset=0;offset<all.size();offset+=172){const auto* p=all.data()+offset;Game g;uint32_t seq=0;assert(decodePersistent(p,p[152],g,seq)==Decode::Ok&&persistentValid(g));assert(!g.dungeonSeed&&!g.dungeonGenerator&&!g.dungeonRun);assert(packed(g,seq,p[152],29)==Bytes(p,p+160));auto newBytes=packed(g,seq,p[152]);Game newer;uint32_t nextseq=0;assert(decodePersistent(newBytes.data(),p[152],newer,nextseq)==Decode::Ok&&nextseq==seq&&same(g,newer));assert(geometry(g)==get32(p,164));auto next=g;assert(random(next)==get32(p,160));if(inDungeon(g)){++active;if(!eventExpedition(g)&&!islandDungeon(g)&&g.phase==Phase::Home)legacy=g;}++count;}
 assert(inDungeon(legacy)&&leaveDungeon(legacy)&&!inDungeon(legacy));legacy.crystals=9;assert(persistentValid(legacy));auto entry=legacy;assert(!enterDungeon(entry)&&entry.dungeonRun==1&&entry.dungeonSeed&&entry.dungeonGenerator==1&&entry.randomState==legacy.randomState&&entry.crystals+1==legacy.crystals&&persistentValid(entry));
 auto entered=seeded(entry,30);auto old=seeded(legacy,29);
 for(unsigned slot=0;slot<3;++slot){verify(old,slot,legacy,entry);auto exit=entry;assert(leaveDungeon(exit)&&exit.dungeonSeed==entry.dungeonSeed&&exit.dungeonRun==entry.dungeonRun&&!exit.dungeonGenerator);verify(entered,slot,entry,exit);}
 // Seed/run identity is independent from combat RNG and persists through exit.
 auto exited=entry;assert(leaveDungeon(exited));auto diverged=exited;random(diverged);assert(!enterDungeon(exited)&&!enterDungeon(diverged));assert(exited.dungeonSeed==diverged.dungeonSeed&&exited.dungeonRun==2&&geometry(exited)==geometry(diverged));
 auto wrap=entry;assert(leaveDungeon(wrap));wrap.dungeonRun=0xffffff;auto before=wrap;assert(!enterDungeon(wrap)&&wrap.dungeonRun==1&&wrap.dungeonSeed!=before.dungeonSeed&&wrap.randomState==before.randomState);for(unsigned slot=0;slot<3;++slot)verify(seeded(before,30),slot,before,wrap);
 // Boss and chest settlement include reward/masks/identity in one blob.
 auto fight=entry;dungeonSetFloor(fight,1);fight.dungeonEnemies=63;fight.dungeonLoot=8;fight.dungeonXY=0x27;dungeonFace(fight,0);assert(begin(fight,8)&&persistentValid(fight));auto won=fight;won.enemyHp=0;finish(won);assert(persistentValid(won));auto resolved=won;assert(dungeonResolve(resolved)&&resolved.dungeonClears==won.dungeonClears+1&&persistentValid(resolved));auto paid=resolved;assert(dungeonCollect(paid)&&paid.dungeonLoot&128&&persistentValid(paid));const auto paidOnce=paid;dungeonCollect(paid);assert(same(paid,paidOnce));auto dead=fight;dead.p.hp=0;finish(dead);assert(persistentValid(dead));auto recovered=dead;assert(dungeonResolve(recovered)&&!inDungeon(recovered)&&!recovered.dungeonGenerator&&recovered.dungeonSeed==dead.dungeonSeed&&recovered.dungeonRun==dead.dungeonRun&&persistentValid(recovered));
 for(unsigned slot=0;slot<3;++slot){verify(seeded(fight,30),slot,fight,won);verify(seeded(won,30),slot,won,resolved);verify(seeded(resolved,30),slot,resolved,paid);verify(seeded(dead,30),slot,dead,recovered);}
 // Full-byte invalid schema mutations: no unknown version or unrepresentable run can enter memory.
 auto invalid=entry;invalid.dungeonRun=0x1000000;assert(!persistentValid(invalid));invalid=entry;invalid.dungeonGenerator=2;assert(!persistentValid(invalid));invalid=entry;invalid.dungeonSeed=0;assert(!persistentValid(invalid));invalid=entry;invalid.dungeonRun=0;assert(!persistentValid(invalid));invalid=entry;clearDungeon(invalid);invalid.dungeonGenerator=1;assert(!persistentValid(invalid));
 auto future=packed(entry);future[143]=2;put32(future.data(),156,crc(future.data(),156));Game g;uint32_t seq;assert(decodePersistent(future.data(),0,g,seq)==Decode::Unsupported);auto s=seeded(legacy,29);s.blobs["b"]=future;auto original=s.blobs;B b(s);J j(b);assert(j.load(g)==Load::Blocked&&!j.save(legacy)&&s.blobs==original);
 printf("PASS %u exact Save29 fixtures migrated losslessly (%u active fixed dungeon), byte-canonical progression, geometry/nextRNG; %u power cuts and %u fault/retry probes across all3slots for migration/crystalentry/exit/24bitwrap/bossreward/clear/chest/death.\n",count,active,cuts,faults);
}
