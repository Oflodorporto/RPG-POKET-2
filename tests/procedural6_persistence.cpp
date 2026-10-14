// Reuse the exact production-journal fault injector and equality checks.
#define main existingSaveRegressionMain
#include "save29.cpp"
#undef main
#include <utility>
int main(){
 std::vector<std::pair<Game,Game>> transitions;
 auto g=hero(0);g.city=1;g.crystals=3;auto before=g;assert(!enterDungeon(g));transitions.push_back({before,g});
 for(unsigned i:{0u,1u,2u,3u}){auto p=dungeonPickup(g,i);g.dungeonXY=p.x|(p.y<<4);before=g;assert(dungeonCollect(g));transitions.push_back({before,g});}
 g.dungeonXY=0x17;before=g;assert(dungeonStairs(g));transitions.push_back({before,g});
 g.dungeonEnemies=63;g.dungeonXY=0x27;dungeonFace(g,0);assert(begin(g,8));before=g;g.enemyHp=0;finish(g);transitions.push_back({before,g});before=g;assert(dungeonResolve(g));transitions.push_back({before,g});
 before=g;assert(dungeonCollect(g));transitions.push_back({before,g});before=g;assert(leaveDungeon(g));transitions.push_back({before,g});before=g;assert(!enterDungeon(g));transitions.push_back({before,g});
 auto target=dungeonSpawn(g,0);for(unsigned d=0;d<4;++d){int x=target.x-dungeonDx[d],y=target.y-dungeonDy[d];if(!dungeonWall(g,x,y)&&dungeonEnemyAt(g,x,y)<0){g.dungeonXY=x|(y<<4);dungeonFace(g,d);break;}}assert(begin(g,target.id));before=g;g.p.hp=0;finish(g);transitions.push_back({before,g});before=g;assert(dungeonResolve(g));transitions.push_back({before,g});
 for(unsigned slot=0;slot<3;++slot)for(const auto& step:transitions){
  assert(persistentValid(step.first)&&persistentValid(step.second));Store base;for(unsigned s=0;s<3;++s)seed(base,s,s==slot?step.first:hero(s));
  verifySaveCuts(base,slot,step.first,step.second,false);
 }
 // Migrate legacy bytes at an arbitrary partially explored position without
 // changing geometry, loot eligibility, RNG, fatigue or the other two slots.
 for(unsigned slot=0;slot<3;++slot)for(unsigned mode=0;mode<3;++mode){auto old=hero(slot);old.city=1;old.crystals=2;
  if(mode){assert(!enterDungeon(old,0));old.dungeonLoot=8;old.dungeonEnemies=1;old.dungeonXY=0x73;if(mode==2){dungeonSetFloor(old,1);old.dungeonXY=0x71;}}
  Store base;for(unsigned s=0;s<3;++s)seed(base,s,s==slot?old:hero(s));Backend b(base);b.slot=slot;
  for(bool side:{false,true}){auto& bytes=base.blobs[b.key(side)];Game record;uint32_t seq=0;assert(decodePersistent(bytes.data(),slot,record,seq)==Decode::Ok);encodePersistentVersion(record,seq,slot,bytes.data(),29);}
  auto raw=base.blobs;J read(b);Game h;assert(read.load(h)==Load::Ok&&same(h,old)&&base.blobs==raw&&!h.dungeonGenerator);auto next=old;
  if(mode)assert(!dungeonMove(next,0,0,1));else assert(!enterDungeon(next));
  verifySaveCuts(base,slot,old,next,false);
  assert(read.save(next));J boot(b);assert(boot.load(h)==Load::Ok&&same(h,next));if(mode)assert(!h.dungeonGenerator);
 }
 printf("PASS procedural6 journal: %zu atomic entry/move/stairs/reward/death transitions, %u power cuts, %u I/O/lost-ACK retries, all 3 slots; Save29 migration is read-only until normal save\n",transitions.size(),cuts,faults);
}
