#include "Persistence.h"
#include "ResetEpoch.h"
#include <cassert>
#include <map>
#include <string>
#include <vector>
#include <cstdio>
using namespace rpg;
struct Store {
 bool ready=true; unsigned writes=0,erasures=0;std::map<std::string,std::vector<uint8_t>> blobs;
 Read read(const char*k,uint8_t*b,unsigned n){auto i=blobs.find(k);if(i==blobs.end())return Read::Missing;if(i->second.size()!=n)return Read::Corrupt;memcpy(b,i->second.data(),n);return Read::Ok;}
 bool write(const char*k,const uint8_t*b,unsigned n){++writes;blobs[k]={b,b+n};return true;}
 Read presence(const char*k){return blobs.count(k)?Read::Ok:Read::Missing;}
 bool erase(const char*k){++erasures;blobs.erase(k);return true;}
};
using B=PersistentSlotBackend<Store>;using J=PersistentJournal<B>;
using Bytes=std::vector<uint8_t>;
Bytes packed(const Game&g,unsigned seq,unsigned slot){Bytes b(PERSISTENT_SAVE_SIZE);encodePersistent(g,seq,slot,b.data());return b;}
Game hero(unsigned slot){auto g=create(slot,8801+slot);g.race=slot;g.shirt=slot+2;g.trousers=7-slot;g.fatigue=34+slot;g.p.gold=1200+slot;g.tutorial=true;return g;}
Store initial(){Store s;for(unsigned slot=0;slot<3;++slot){B b(s);b.slot=slot;J j(b);Game h;assert(j.load(h)==Load::Empty);assert(j.save(hero(slot)));assert(j.save(hero(slot)));}s.writes=s.erasures=0;return s;}
Bytes future(const Game&g,unsigned seq,unsigned slot,unsigned format){auto b=packed(g,seq,slot);put16(b.data(),4,format);b[143]=1;put32(b.data(),148,0x5abc9912);b[153]=1;put32(b.data(),156,crc(b.data(),156));return b;}
struct ResetStore{
 Store data;Bytes epoch;Read epochStatus=Read::Ok;unsigned reads=0,writes=0,erasures=0,presences=0;ResetStore(){epoch.resize(RESET_EPOCH_SIZE);encodeResetEpoch(ResetPhase::Done,epoch.data());}
 Read readEpoch(uint8_t*b){++reads;if(epochStatus==Read::Ok)memcpy(b,epoch.data(),epoch.size());return epochStatus;}
 bool writeEpoch(const uint8_t*){++writes;return false;}
 Read newFamilyPresence(){++presences;return data.blobs.empty()?Read::Missing:Read::Ok;}
 Read readLegacyDeleted(uint8_t,bool&){assert(false);return Read::Error;}
 bool writeLegacyDeleted(uint8_t){++writes;return false;}
 Read legacyPresence(const char*){assert(false);return Read::Error;}
 bool eraseLegacy(const char*){++erasures;return false;}
};
int main(){
 static_assert(CURRENT_SAVE_FORMAT==29&&PERSISTENT_SAVE_FORMAT==29&&PERSISTENT_SAVE_SIZE==160&&SAVE_RESET_EPOCH==1,"Exact released Save29");
 unsigned probes=0;const auto base=initial();
 for(unsigned slot=0;slot<3;++slot)for(unsigned side=0;side<2;++side)for(unsigned seq:{0u,1u,3u,UINT32_MAX,0x80000002u})for(unsigned format:{30u,31u}){
  auto s=base;B io(s);io.slot=slot;J j(io);Game loaded=hero(slot);assert(j.load(loaded)==Load::Ok);const auto beforeGame=packed(loaded,1,slot);
  s.blobs[io.key(side)]=future(hero(slot),seq,slot,format);const auto before=s.blobs;
  // Existing stale RAM observer cannot erase a newer-format side either.
  assert(!j.save(loaded));assert(s.blobs==before&&!s.writes&&!s.erasures);
  J boot(io);assert(boot.load(loaded)==Load::Blocked&&boot.blocked&&boot.active==-1);assert(packed(loaded,1,slot)==beforeGame);
  assert(!boot.save(hero(slot)));assert(s.blobs==before&&!s.writes&&!s.erasures);
  // Every untouched slot remains loadable, not affected by the blocked slot.
  for(unsigned other=0;other<3;++other)if(other!=slot){B ob(s);ob.slot=other;J oj(ob);Game oh;assert(oj.load(oh)==Load::Ok&&packed(oh,1,other)==packed(hero(other),1,other));}
  ++probes;
 }
 for(unsigned slot=0;slot<3;++slot){auto s=base;B io(s);io.slot=slot;for(unsigned side=0;side<2;++side)s.blobs[io.key(side)]=future(hero(slot),3+side,slot,30);auto before=s.blobs;Game h;J j(io);assert(j.load(h)==Load::Blocked&&!j.save(hero(slot))&&s.blobs==before&&!s.writes&&!s.erasures);++probes;}
 // Future reserved bytes also block under format29, never silently normalize.
 for(unsigned offset:{143u,148u,149u,150u,151u,153u,154u,155u}){auto s=base;B io(s);auto &v=s.blobs["a"];v[offset]=1;put32(v.data(),156,crc(v.data(),156));auto before=s.blobs;Game h;J j(io);assert(j.load(h)==Load::Blocked&&!j.save(hero(0))&&s.blobs==before&&!s.writes&&!s.erasures);++probes;}
 for(unsigned mode=0;mode<4;++mode){ResetStore rs;rs.data=base;for(unsigned slot=0;slot<3;++slot){B io(rs.data);io.slot=slot;rs.data.blobs[io.key(false)]=future(hero(slot),3,slot,30);}const auto before=rs.data.blobs;
  if(mode==1)rs.epochStatus=Read::Missing;if(mode==2)encodeResetEpoch(ResetPhase::Pending,rs.epoch.data());if(mode==3)rs.epoch[4]^=1;
  assert(ensureResetEpoch(rs)==(mode==0?ResetResult::Ready:ResetResult::Blocked));assert(rs.data.blobs==before&&!rs.writes&&!rs.erasures);if(mode==0)assert(rs.reads==1&&!rs.presences);++probes;
 }
 printf("PASS %u probes: exact released Save29 fail-closed rollback, unsupported record on either side at older/newer/wrap/ambiguous sequence, all3slots, stale writer, both Save30 records, all8reservedbytes, epoch1 Done/missing/pending/corrupt no writes or erases.\n",probes);
}
