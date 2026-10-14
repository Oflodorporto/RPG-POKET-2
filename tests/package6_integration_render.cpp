#include "SceneCanvas.h"
#include "../firmware/RPG_POKET_2/Persistence.h"
int main(int argc,char** argv){
 assert(argc==2);artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);
 std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);
 pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);
 assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
 Canvas c;unsigned frames=0,cases=0;
 for(unsigned cls=0;cls<4;++cls)for(unsigned seed:{1u,7u,42u,65535u}){
  auto g=rpg::create(cls,seed);g.p.xp=rpg::dndXp[4];rpg::levelUp(g);
  g.city=1;g.crystals=1;g.tutorial=true;assert(!rpg::enterDungeon(g));
  auto spawn=rpg::dungeonSpawn(g,0);bool placed=false;
  for(unsigned d=0;d<4&&!placed;++d){int x=spawn.x-rpg::dungeonDx[d],y=spawn.y-rpg::dungeonDy[d];
   if(!rpg::dungeonWall(g,x,y)&&rpg::dungeonEnemyAt(g,x,y)<0){
    g.dungeonXY=x|(y<<4);rpg::dungeonFace(g,d);placed=true;}}
  assert(placed&&!rpg::dungeonMove(g,1,0,0)&&g.phase==rpg::Phase::Hero);
  assert(rpg::persistentValid(g));ViewState v;v.page=Page::Dungeon;
  assert(!rpg::act(g,rpg::Action::Attack,&v.heroTrace)&&v.heroTrace.valid);
  assert(rpg::persistentValid(g));const auto before=g;
  uint8_t bytes[160];rpg::encodePersistent(g,1,0,bytes);
  v.effect=Effect::Slash;v.combatResult=heroCombatResult(g,rpg::Action::Attack);
  for(unsigned ms:{0u,240u,480u,900u}){
   v.effectElapsedMs=ms;v.effectFrame=ms*8/1200;v.effectProgress=ms*1000/1200;
   render(c,g,v);assert(!memcmp(&g,&before,sizeof(g)));++frames;
   if(cls==0&&ms==480)c.save(std::string(argv[1])+"/generated-trace-seed-"+std::to_string(seed)+".ppm");
  }
  v.effect=Effect::None;v.traceOpen=true;
  for(unsigned page=0;page<combatTraceUi::pageCount(v.heroTrace);++page){
   v.tracePage=page;render(c,g,v);assert(!memcmp(&g,&before,sizeof(g)));++frames;}
  uint8_t after[160];rpg::encodePersistent(g,1,0,after);assert(!memcmp(bytes,after,sizeof(bytes)));
  rpg::Game loaded;uint32_t seq=0;assert(rpg::decodePersistent(after,0,loaded,seq)==rpg::Decode::Ok&&seq==1);
  assert(!memcmp(&loaded,&before,sizeof(loaded)));++cases;
 }
 free(artMemory);artMemory=nullptr;
 printf("PASS: %u generated-map real-combat render cases, %u reveal/receipt frames, valid Save30 round trips, no Game/RNG mutation\n",cases,frames);
}
