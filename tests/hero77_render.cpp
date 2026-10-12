#define main priorRenderRegression
#include "render.cpp"
#undef main
#include "../firmware/RPG_POKET_2/HeroPresentation.h"
#include <array>
#include <set>
#include <bitset>
#include <filesystem>
#include <iomanip>

namespace {
constexpr uint16_t sentinel=0x1234;
struct HeroCanvas:Canvas {
 struct Label {int x,y;std::string text;};std::vector<Label> labels;
 void print(const char* s){labels.push_back({tx,ty,s});Canvas::print(s);}
 void clear(){labels.clear();}
 bool has(const char* s)const{for(const auto& label:labels)if(label.text==s)return true;return false;}
};
Effect effectFor(const rpg::Game& g,rpg::Action a){
 using A=rpg::Action;
 if(a==A::DivineSmite)return g.dodge?Effect::Slash:Effect::DivineSlash;
 if(a==A::Attack||a==A::Offensive){if(!g.dodge&&g.dndProgression&&g.p.cls==1&&g.p.level>=11)return Effect::DivineSlash;
  return a==A::Offensive?(g.p.cls==0?Effect::Lightning:g.p.cls==3?Effect::Rage:Effect::Slash):g.p.cls==0?Effect::Projectile:g.p.cls==2?Effect::Thrust:Effect::Slash;}
 if(a==A::MagicMissile)return Effect::MagicDarts;
 if(a==A::BurningHands||a==A::ScorchingRay)return Effect::FlameVolley;
 if(a==A::Fireball)return Effect::FireBurst;
 if(a==A::SacredWeapon||a==A::TurnUndead)return Effect::Radiant;
 if(a==A::RagePower)return Effect::Rage;
 return Effect::Shield;
}
bool onHero(rpg::Action a){using A=rpg::Action;return a==A::Defensive||a==A::ShieldSpell||a==A::LayHands||a==A::SecondWind||a==A::RagePower||a==A::ActionSurge||a==A::SacredWeapon;}
rpg::Game hero(unsigned cls,bool modern=true){auto g=modern?rpg::create(cls,77):rpg::testHero(cls,77);if(modern){g.p.xp=rpg::dndXp[19];rpg::levelUp(g);g.oath=cls==1;}g.city=1;g.race=cls;g.shirt=5;g.trousers=7;g.p.hp=g.p.maxhp/2;g.p.mp=g.p.maxmp;assert(rpg::begin(g,2));return g;}
void immutable(HeroCanvas& c,const rpg::Game& g,const ViewState& v){uint8_t before[sizeof(g)],save[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];memcpy(before,&g,sizeof(g));rpg::encode(g,77,save);c.clear();render(c,g,v,0);rpg::encode(g,77,after);assert(!memcmp(before,&g,sizeof(g))&&!memcmp(save,after,sizeof(save)));}
void stableOutside(const Canvas& first,const Canvas& next,int top,int bottom){for(int y=0;y<320;++y)if(y<top||y>=bottom)for(int x=0;x<240;++x)assert(first.pixels[y*240+x]==next.pixels[y*240+x]);}
unsigned changed(const Canvas& c){unsigned count=0;for(auto p:c.pixels)count+=p!=sentinel;return count;}
bool differs(const Canvas& a,const Canvas& b){return memcmp(a.pixels,b.pixels,sizeof(a.pixels))!=0;}
// Host-only raster audit. Each opaque 2x2 billboard write is represented, including
// its replicated edge pixel. Nothing is inferred from nominal sprite rectangles.
using RasterMask=std::bitset<240*172>;
void writeMask(std::ofstream& out,const RasterMask& mask){
 std::array<uint8_t,(240*172)/8> packed{};for(unsigned i=0;i<mask.size();++i)if(mask[i])packed[i/8]|=uint8_t(1u<<(i%8));out.write(reinterpret_cast<const char*>(packed.data()),packed.size());
}
void writeNumber(std::ofstream& out,unsigned value){for(unsigned i=0;i<4;++i)out.put(char(value>>(i*8)));}
struct EnemyRaster {RasterMask body,head,torso;};
EnemyRaster enemyRaster(unsigned id,unsigned pose,unsigned distance,bool expedition,int recoil=0){
 const auto frame=enemyArt::body(id,enemyArt::Pose(pose));const auto image=enemyArt::decode(frame);
 assert(image.pixels&&image.w==80&&image.h==86);
 const int envelopeW=expedition?(id>=25?64:80):48,envelopeH=expedition?(id>=25?80:86):64;
 // Same centered, full-opacity projection/fit/sampling as DungeonView's billboard.
 // Walls are deliberately absent; this measures centered full silhouettes,
 // not the percentage of a smaller silhouette partially hidden by a wall.
 const int height=std::min(250,int(172*.8f/distance)),width=height*envelopeW/envelopeH;
 const int base=int(86+86.f/distance);const auto fit=enemyArt::fitWorld(frame,120+recoil,base,width,height);
 assert(fit.anchorX==120+recoil);if(frame.anchor==enemyArt::Anchor::Ground)assert(fit.anchorY==base);else assert(fit.y==base-fit.h);
 EnemyRaster result;
 for(int x=std::max(0,fit.x);x<std::min(240,fit.x+fit.w);x+=2)for(int y=std::max(0,fit.y);y<std::min(172,fit.y+fit.h);y+=2){
  const unsigned ix=(x-fit.x)*image.w/fit.w,iy=(y-fit.y)*image.h/fit.h;
  if(image.pixels[iy*image.w+ix]==enemyArt::TRANSPARENT)continue;
  // Explicit canonical, opaque-only core: central half x20..59, upper y0..23
  // (head) and middle y24..61 (torso). These are screen-readability regions,
  // not anatomical claims for nonhumanoid creatures, wings or extended weapons.
  const bool central=ix>=20&&ix<60,head=central&&iy<24,torso=central&&iy>=24&&iy<62;
  for(int dx=0;dx<2&&x+dx<240;++dx)for(int dy=0;dy<2&&y+dy<172;++dy){const unsigned i=(y+dy)*240+x+dx;result.body.set(i);if(head)result.head.set(i);if(torso)result.torso.set(i);}
 }
 assert(result.body.any());return result;
}
struct RasterProbe:Canvas {
 RasterMask marked;
 void fillRect(int x,int y,int w,int h,uint16_t color){if(color!=sentinel)return;for(int dy=0;dy<h;++dy)for(int dx=0;dx<w;++dx)if(x+dx>=0&&x+dx<240&&y+dy>=0&&y+dy<172)marked.set((y+dy)*240+x+dx);}
};
unsigned verifyRealBillboards(){
 // Independently check the oracle against real drawDungeon calls in all three
 // envelope branches. A temporary host palette tags opaque writes, including
 // writes subsequently hidden by the hero. Original palette bytes are restored.
 unsigned checks=0;
 for(unsigned mode=0;mode<6;++mode)for(unsigned distance:{1u,2u})for(unsigned pose=0;pose<(distance==1?4u:1u);++pose){
  auto g=rpg::create(3,42);g.dungeonFlags=1;g.phase=rpg::Phase::Hero;g.enemyHp=100;g.dungeonLoot=255;
  const bool expedition=mode!=0&&mode!=3;const unsigned spawnIndex=mode>=3?2:0;
  if(mode==0){g.city=1;g.dungeonXY=(3-distance)|(7<<4);rpg::dungeonFace(g,1);}
  else if(mode==3){g.city=1;g.dungeonXY=7|((3+distance)<<4);rpg::dungeonFace(g,0);}
  else {g.eventStage=2;g.eventKind=3;g.eventTier=mode==1?0:mode==2?2:mode==4?1:3;g.city=g.eventTier;g.eventLevel=mode==1?2:10;g.dungeonXY=(mode>=4?3:1)|((1+distance)<<4);rpg::dungeonFace(g,0);if(mode>=4)rpg::dungeonSetFloor(g,1);}
  const auto spawn=rpg::dungeonSpawn(g,spawnIndex);g.enemyId=spawn.id;g.dungeonEnemies=((1u<<rpg::dungeonSpawnCount(g))-1)&~(1u<<spawnIndex);
  ViewState v;v.page=Page::Dungeon;
  if(pose){v.effect=Effect::Slash;v.effectProgress=pose==1?0:pose==2?350:550;v.effectFrame=v.effectProgress*8/1000;v.combatResult={CombatFeedback::Damage,4,false,pose!=3};v.effectOnHero=pose!=3;}
  const int recoil=pose==3?combatRecoil(v.combatResult,v.effectProgress):0;
  const auto expected=enemyRaster(spawn.id,pose,distance,expedition,recoil);
  auto* palette=artMemory+assetMeta[enemyArt::enemies[spawn.id].palette].offset;std::array<uint8_t,32> original{};memcpy(original.data(),palette,32);
  for(unsigned i=1;i<16;++i){palette[i*2]=sentinel&255;palette[i*2+1]=sentinel>>8;}
  RasterProbe present,absent;drawDungeon(present,g,v);g.dungeonEnemies|=1u<<spawnIndex;drawDungeon(absent,g,v);
  memcpy(palette,original.data(),32);assert(absent.marked.none());assert(present.marked==expected.body);++checks;
 }
 return checks;
}
void exactForegroundFit(unsigned cls,const CombatResult& result,unsigned p,const DungeonHeroPose& pose){
 using H=HeroChoreography;const auto h=heroChoreography(cls,result);const bool active=h!=H::None&&p<1000;
 const bool cast=h==H::StaffBolt||h==H::MagicDarts||h==H::Fireball||h==H::LegacyLightning||h==H::ScorchingRays;
 const bool thrust=h==H::SpearThrust||h==H::PrecisionThrust;
 const bool cut=h==H::SwordCut||h==H::SwordPower||h==H::DivineSmite||h==H::AxeCut||h==H::RageStrike;
 assert(pose.frame==unsigned(active&&(cast||thrust||cut)&&p>=200&&p<750));
 const auto crop=dungeonWeaponCrops[cls][pose.frame];int originalHeight=crop.h*2,motionX=0;
 if(active){const unsigned phase=p<200?0:p<491?1:p<650?2:p<850?3:4;
  if(thrust){const int dy[]={4,2,-8,-2,0};originalHeight-=dy[phase];}
  else if(cut){const int dy[]={4,1,-7,-2,0},dx[]={3,1,-4,-1,0};originalHeight-=dy[phase];motionX=dx[phase];}
  else if(cast){const int dy[]={3,-3,-5,-1,0};originalHeight-=dy[phase];}
  else if(h==H::LayHands||h==H::SecondWind)originalHeight-=phase<2?8:phase==2?3:0;
  else if(h==H::ArcaneGuard||h==H::ShieldGuard||h==H::SpearGuard||h==H::AxeGuard||h==H::RageBuff||h==H::ActionSurge||h==H::TurnUndead||h==H::FlameCone)originalHeight-=phase<4?-3:0;
 }
 // Fixed 85% composition adjustment, preserving the approved crop and motion.
 // Both dimensions still enlarge the authored source by at least 1.5x.
 assert(pose.w==std::min(88,int(crop.w)*2)*85/100&&pose.h==originalHeight*85/100);
 assert(pose.x==240-pose.w+motionX*85/100+1&&pose.y==172-pose.h);
 assert(pose.w>=49&&pose.w<=74&&pose.w*2>=crop.w*3&&pose.h*2>=crop.h*3);
 const int ox=cls==0?(pose.frame?38:7):crop.w/2,oy=cls==0?6:crop.h/2;
 assert(pose.originX==pose.x+ox*pose.w/crop.w&&pose.originY==pose.y+oy*pose.h/crop.h);
 assert(pose.palmX==std::min(230,pose.x+pose.w*3/4)&&pose.palmY==pose.y+pose.h*4/5);
 assert(pose.originX>=150&&pose.originX<240&&pose.originY>=34&&pose.originY<172);
 assert(pose.palmX>=150&&pose.palmX<240&&pose.palmY>=34&&pose.palmY<172);
}
struct OverlapCase {unsigned cls=0,action=0,phase=0;bool modern=false;int recoil=0;RasterMask foreground;};
struct Silhouette {
 unsigned id,pose,distance;bool expedition;std::array<EnemyRaster,7> raster;
 unsigned maxCovered=0,maxHead=0,maxTorso=0,comparisons=0;OverlapCase worst;
};
Canvas overlapCanvas(const Silhouette& shape,const OverlapCase& sample){
 Canvas out;out.fillScreen(0x18e3);const auto& raster=shape.raster[sample.recoil];
 for(unsigned i=0;i<240*172;++i){if(raster.body[i])out.pixels[i]=raster.head[i]?0xffe0:raster.torso[i]?0x07ff:UI_GREEN;if(sample.foreground[i])out.pixels[i]=raster.body[i]?0xf81f:UI_WHITE;}
 return out;
}
void contactSheet(const std::string& path,const std::vector<Silhouette>& shapes,bool expedition,unsigned distance){
 constexpr unsigned width=120*4,tileHeight=108,height=tileHeight*29;std::vector<uint16_t> pixels(width*height,0x18e3);
 for(const auto& shape:shapes)if(shape.expedition==expedition&&shape.distance==distance){auto tile=overlapCanvas(shape,shape.worst);
  char label[24];snprintf(label,sizeof(label),"ID%02u P%u %5.2f%%",shape.id,shape.pose,shape.maxCovered*100.0/shape.raster[shape.worst.recoil].body.count());
  tile.setTextSize(1);tile.setTextColor(UI_WHITE);tile.setCursor(0,176);tile.print(label);
  snprintf(label,sizeof(label),"H%u T%u C%u A%u",shape.maxHead,shape.maxTorso,shape.worst.cls,shape.worst.action);tile.setCursor(0,194);tile.print(label);
  for(unsigned y=0;y<86;++y)for(unsigned x=0;x<120;++x)pixels[(shape.id*tileHeight+y)*width+shape.pose*120+x]=tile.pixels[(y*2)*240+x*2];
  // Keep text at native size beneath the half-size raster thumbnail.
  for(unsigned y=0;y<8;++y)for(unsigned x=0;x<120;++x){pixels[(shape.id*tileHeight+88+y)*width+shape.pose*120+x]=tile.pixels[(176+y)*240+x];pixels[(shape.id*tileHeight+98+y)*width+shape.pose*120+x]=tile.pixels[(194+y)*240+x];}
 }
 std::ofstream out(path,std::ios::binary);assert(out);out<<"P6\n"<<width<<' '<<height<<"\n255\n";for(auto p:pixels){char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};out.write(rgb,3);}assert(out);
}

}

int main(int argc,char** argv){
 const std::string root=argc>1?argv[1]:"build/screens/hero77";artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);
 std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);menu=MenuState{};
 std::set<unsigned> usedFrames[4],choreographies;unsigned actionCases=0,battleFrames=0,dungeonFrames=0;HeroCanvas c;
 std::ofstream report(root+"/render-coverage.csv");assert(report);report<<"class,progression,action,choreography,battle_frames,dungeon_frames\n";
 const unsigned phases[]={0,60,180,199,200,350,490,491,550,649,650,749,750,849,850,900,999};
 for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned index=0;index<=unsigned(rpg::Action::DivineSmite);++index){
  auto g=hero(cls,modern);auto a=rpg::Action(index);if(modern&&cls==0&&a==rpg::Action::Offensive)a=rpg::Action::MagicMissile;if(modern&&cls==0&&a==rpg::Action::Defensive)a=rpg::Action::ShieldSpell;
  unsigned hp=g.p.hp;if(rpg::act(g,a)||a==rpg::Action::Life||a==rpg::Action::Mana||a==rpg::Action::Flee)continue;
  ++actionCases;ViewState v;v.page=Page::Battle;v.effect=effectFor(g,a);v.combatResult=heroCombatResult(g,a,hp);v.effectOnHero=onHero(a);
  const auto choreography=heroChoreography(cls,v.combatResult);assert(choreography!=HeroChoreography::None);choreographies.insert(unsigned(choreography));CombatFx timer;timer.start(v.effect,v.effectOnHero,0,v.combatResult);Canvas fixed;bool first=true;
  for(unsigned p:phases){v.effectProgress=p;v.effectElapsedMs=p*timer.length()/1000;v.effectFrame=p*8/1000;
   auto pose=heroPose(cls,v.combatResult,p,v.effectElapsedMs);assert(pose.heroFrame<6);usedFrames[cls].insert(pose.heroFrame);assert(pose.x>=-8&&pose.x<=12&&pose.y>=-4&&pose.y<=4);
   immutable(c,g,v);++battleFrames;if(first){fixed=c;first=false;}else stableOutside(fixed,c,86,192);
   if(p==180||p==550||p==900)c.save(root+"/battle-"+std::to_string(cls)+"-"+std::to_string(index)+"-"+(modern?"new":"legacy")+"-"+std::to_string(p)+".ppm");
  }
  auto recovered=heroPose(cls,v.combatResult,1000,timer.length());assert(recovered.heroFrame==0&&recovered.x==0&&recovered.y==0);usedFrames[cls].insert(0);
  g.dungeonFlags=5;g.dungeonXY=0x72;g.dungeonEnemies=0;v.page=Page::Dungeon;first=true;
  for(unsigned p:phases){v.effectProgress=p;v.effectElapsedMs=p*timer.length()/1000;v.effectFrame=p*8/1000;immutable(c,g,v);++dungeonFrames;if(first){fixed=c;first=false;}else stableOutside(fixed,c,34,172);
   if(p==180||p==550||p==900)c.save(root+"/dungeon-"+std::to_string(cls)+"-"+std::to_string(index)+"-"+(modern?"new":"legacy")+"-"+std::to_string(p)+".ppm");
  }
  report<<cls<<','<<(modern?"new":"legacy")<<','<<index<<','<<unsigned(choreography)<<','<<std::size(phases)<<','<<std::size(phases)<<'\n';
 }
 // Non-hero snapshots, inventory and escape cannot acquire hero choreography.
 for(unsigned cls=0;cls<4;++cls){CombatResult result;assert(heroChoreography(cls,result)==HeroChoreography::None);result.hasHeroAction=true;result.enemyAction=true;assert(heroChoreography(cls,result)==HeroChoreography::None);result.enemyAction=false;
  for(auto a:{rpg::Action::Life,rpg::Action::Mana,rpg::Action::Flee}){result.heroAction=a;assert(heroChoreography(cls,result)==HeroChoreography::None);}
 }
 // Personalization remains the original source/mask path, pixel-for-pixel, for
 // every race/class/outfit and every frame actually used by the choreographies.
 assert(choreographies.size()==23);unsigned personalFrames=0;
 for(unsigned cls=0;cls<4;++cls)for(unsigned race=0;race<4;++race)for(unsigned shirt=0;shirt<8;++shirt)for(unsigned pants=0;pants<8;++pants)for(unsigned f=0;f<6;++f){
  auto g=hero(cls);g.race=race;g.shirt=shirt;g.trousers=pants;c.fillScreen(sentinel);personalSprite(c,g,8,77,f);
  const auto* source=reinterpret_cast<const uint16_t*>(assetBytes(raceAsset[race][cls][f]));const auto* mask=assetBytes(raceMask[race][cls][f]);
  for(int y=0;y<120;++y)for(int x=0;x<112;++x){unsigned index=y*112+x;auto color=outfitPixel(source[index],mask[index],shirt,pants);assert(c.pixels[(77+y)*240+8+x]==(color==0xf81f?sentinel:color));}
  ++personalFrames;
 }
 // Input rectangles keep inclusive lower/exclusive upper edges.
 for(const auto r:panelUi::battleButtons){assert(r.contains(r.x,r.y)&&r.contains(r.x+r.w-1,r.y+r.h-1));assert(!r.contains(r.x-1,r.y)&&!r.contains(r.x,r.y-1)&&!r.contains(r.x+r.w,r.y)&&!r.contains(r.x,r.y+r.h));}
 // Explicit outcome snapshots exercise display semantics independently of RNG.
 for(bool dungeon:{false,true})for(auto kind:{CombatFeedback::Damage,CombatFeedback::Miss,CombatFeedback::NoDamage,CombatFeedback::Heal,CombatFeedback::Guard,CombatFeedback::Buff}){
  auto g=hero(2);ViewState v;v.page=dungeon?Page::Dungeon:Page::Battle;v.effect=Effect::Thrust;v.effectProgress=600;v.effectElapsedMs=400;v.effectFrame=4;v.combatResult={kind,uint16_t(kind==CombatFeedback::Damage||kind==CombatFeedback::Heal?9:0),kind==CombatFeedback::Damage,false};v.combatResult.hasHeroAction=true;v.combatResult.heroAction=kind==CombatFeedback::Heal?rpg::Action::SecondWind:kind==CombatFeedback::Guard?rpg::Action::Defensive:kind==CombatFeedback::Buff?rpg::Action::ActionSurge:rpg::Action::Attack;v.effectOnHero=onHero(v.combatResult.heroAction);if(dungeon){g.dungeonFlags=5;g.dungeonXY=0x72;}
  immutable(c,g,v);assert((combatRecoil(v.combatResult,600)>0)==v.combatResult.injury());if(kind==CombatFeedback::Miss)assert(c.has("ESQUIVA")&&!c.has("-9"));if(kind==CombatFeedback::NoDamage)assert(c.has("SEM DANO"));if(kind==CombatFeedback::Heal)assert(c.has("+9 HP"));if(kind==CombatFeedback::Damage)assert(c.has("CRIT -9"));c.save(root+"/outcome-"+std::to_string(unsigned(kind))+"-"+std::to_string(dungeon)+".ppm");
 }
 // Combined overlay isolation: every compact original pose, both projection
 // envelopes and both distances versus the actual enlarged hero raster.
 const std::string combined="build/screens/combined77";std::filesystem::create_directories(combined);
 const unsigned realProjectionChecks=verifyRealBillboards();
 std::vector<Silhouette> silhouettes;silhouettes.reserve(29*4*2*2);
 for(bool expedition:{false,true})for(unsigned id=0;id<enemyArt::ENEMY_COUNT;++id)for(unsigned pose=0;pose<enemyArt::POSE_COUNT;++pose)for(unsigned distance:{1u,2u}){
  Silhouette shape{};shape.id=id;shape.pose=pose;shape.distance=distance;shape.expedition=expedition;
  for(int recoil=0;recoil<=6;++recoil)shape.raster[recoil]=enemyRaster(id,pose,distance,expedition,recoil);
  silhouettes.push_back(std::move(shape));
 }
 std::ofstream maskDump;
 if(getenv("RPG_OCCLUSION_DUMP")){maskDump.open(combined+"/raster-diagnostic.bin",std::ios::binary);assert(maskDump);writeNumber(maskDump,unsigned(silhouettes.size()));
  for(const auto& shape:silhouettes){for(unsigned n:{shape.id,shape.pose,shape.distance,unsigned(shape.expedition)})writeNumber(maskDump,n);for(const auto& raster:shape.raster){writeMask(maskDump,raster.body);writeMask(maskDump,raster.head|raster.torso);}}
 }
 std::ofstream occlusion(combined+"/hero-foreground.csv");assert(occlusion);occlusion<<"class,progression,action,progress,recoil,foreground_pixels,combat_band_pct,max_1tile_pct,max_2tile_pct,max_head_pixels,max_torso_pixels\n";
 unsigned maxPixels=0,foregroundSamples=0,thresholdViolations=0,maxHead=0,maxTorso=0;double maxNearPct=0,maxFarPct=0;size_t worstShape=0;
 auto auditForeground=[&](const rpg::Game& g,const ViewState& v,unsigned action,bool modern){
  const unsigned p=combatProgress(v.effectProgress,v.effectFrame),ms=v.effectElapsedMs==UINT32_MAX?v.effectFrame*1100/8:v.effectElapsedMs;
  const auto pose=dungeonHeroPose(g.p.cls,v.effect==Effect::None?CombatResult{}:v.combatResult,p,ms);assert(pose.frame<2);exactForegroundFit(g.p.cls,v.effect==Effect::None?CombatResult{}:v.combatResult,p,pose);assert(pose.originX>=pose.x&&pose.originX<pose.x+pose.w&&pose.originY>=pose.y&&pose.originY<pose.y+pose.h);assert(pose.palmX>=pose.x&&pose.palmX<pose.x+pose.w&&pose.palmY>=pose.y&&pose.palmY<pose.y+pose.h);
  c.fillScreen(sentinel);drawDungeonHero(c,g,v);const unsigned pixels=changed(c);bool grounded=false;RasterMask foreground;
  const auto crop=dungeonWeaponCrops[g.p.cls][pose.frame];const auto* authored=dungeonArt::weapons[g.p.cls][pose.frame];
  for(int y=0;y<320;++y)for(int x=0;x<240;++x){uint16_t expected=sentinel;
   if(x>=150&&x>=pose.x&&x<pose.x+pose.w&&y>=34&&y<172&&y>=pose.y&&y<pose.y+pose.h){auto source=authored[(crop.y+(y-pose.y)*crop.h/pose.h)*64+crop.x+(x-pose.x)*crop.w/pose.w];if(source!=0xf81f)expected=source;}
   assert(c.pixels[y*240+x]==expected);if(expected!=sentinel){assert(x>=150&&y>=34&&y<172);grounded|=y==171;foreground.set(y*240+x);}
  }
  assert(pixels&&grounded&&pixels*1000<=240*138*136);assert(foreground.count()==pixels);
  // Full opaque enemy pose is a conservative envelope during defeat fade.
  const int recoil=v.effect!=Effect::None&&!v.effectOnHero?combatRecoil(v.combatResult,p):0;assert(recoil>=0&&recoil<=6);
  if(maskDump.is_open()){for(unsigned n:{unsigned(g.p.cls),action,p,unsigned(modern),unsigned(recoil)})writeNumber(maskDump,n);writeMask(maskDump,foreground);}
  OverlapCase sample{g.p.cls,action,p,modern,recoil,foreground};double nearPct=0,farPct=0;unsigned sampleHead=0,sampleTorso=0;
  for(size_t index=0;index<silhouettes.size();++index){auto& shape=silhouettes[index];const int shapeRecoil=shape.distance==1?recoil:0;const auto& raster=shape.raster[shapeRecoil];
   const unsigned covered=(raster.body&foreground).count(),head=(raster.head&foreground).count(),torso=(raster.torso&foreground).count();const double pct=covered*100.0/raster.body.count();
   if(!shape.comparisons||covered>shape.maxCovered){shape.maxCovered=covered;shape.worst=sample;shape.worst.recoil=shapeRecoil;}
   ++shape.comparisons;shape.maxHead=std::max(shape.maxHead,head);shape.maxTorso=std::max(shape.maxTorso,torso);sampleHead=std::max(sampleHead,head);sampleTorso=std::max(sampleTorso,torso);
   if(shape.distance==1){nearPct=std::max(nearPct,pct);if(pct>maxNearPct){maxNearPct=pct;worstShape=index;}}
   else {farPct=std::max(farPct,pct);maxFarPct=std::max(maxFarPct,pct);}
   // Keep the approved readability limits; record all failures before asserting
   // so a genuine asset/layout conflict has an actionable report and fixture.
   if((shape.distance==1&&covered*100>raster.body.count()*4)||(shape.distance==2&&covered)||head||torso)++thresholdViolations;
  }
  maxHead=std::max(maxHead,sampleHead);maxTorso=std::max(maxTorso,sampleTorso);
  if(pixels>maxPixels){maxPixels=pixels;c.save(combined+"/foreground-max-occupation.ppm");}
  occlusion<<unsigned(g.p.cls)<<','<<(modern?"new":"legacy")<<','<<action<<','<<p<<','<<recoil<<','<<pixels<<','<<pixels*100.0/(240*138)<<','<<nearPct<<','<<farPct<<','<<sampleHead<<','<<sampleTorso<<'\n';++foregroundSamples;
 };
 for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned index=0;index<=unsigned(rpg::Action::DivineSmite);++index){
  auto g=hero(cls,modern);auto a=rpg::Action(index);if(modern&&cls==0&&a==rpg::Action::Offensive)a=rpg::Action::MagicMissile;if(modern&&cls==0&&a==rpg::Action::Defensive)a=rpg::Action::ShieldSpell;unsigned hp=g.p.hp;if(rpg::act(g,a)||a==rpg::Action::Life||a==rpg::Action::Mana||a==rpg::Action::Flee)continue;ViewState v;v.effect=effectFor(g,a);v.combatResult=heroCombatResult(g,a,hp);v.effectOnHero=onHero(a);
  CombatFx timer;timer.start(v.effect,v.effectOnHero,0,v.combatResult);
  for(unsigned p:phases){v.effectProgress=p;v.effectElapsedMs=p*timer.length()/1000;v.effectFrame=p*8/1000;auditForeground(g,v,index,modern);}
  v.effectProgress=1000;v.effectElapsedMs=timer.length();v.effectFrame=8;auditForeground(g,v,index,modern);
 }
 for(unsigned cls=0;cls<4;++cls){auto g=hero(cls);ViewState v;v.effect=Effect::None;v.effectProgress=0;auditForeground(g,v,UINT16_MAX,true);}
 std::ofstream familyReport(combined+"/enemy-pose-occlusion.csv");assert(familyReport);familyReport<<"enemy_id,pose,envelope,distance,opaque_pixels,max_covered,max_pct,max_head_pixels,max_torso_pixels,comparisons,worst_class,worst_progression,worst_action,worst_phase,worst_recoil\n";
 for(const auto& shape:silhouettes){const unsigned total=shape.raster[shape.worst.recoil].body.count();familyReport<<shape.id<<','<<shape.pose<<','<<(shape.expedition?"expedition":"normal")<<','<<shape.distance<<','<<total<<','<<shape.maxCovered<<','<<shape.maxCovered*100.0/total<<','<<shape.maxHead<<','<<shape.maxTorso<<','<<shape.comparisons<<','<<shape.worst.cls<<','<<(shape.worst.modern?"new":"legacy")<<','<<shape.worst.action<<','<<shape.worst.phase<<','<<shape.worst.recoil<<'\n';assert(shape.comparisons==foregroundSamples);}
 const auto& worst=silhouettes[worstShape];overlapCanvas(worst,worst.worst).save(combined+"/worst-overlap.ppm");
 for(bool expedition:{false,true})for(unsigned distance:{1u,2u})contactSheet(combined+"/contact-"+(expedition?"expedition":"normal")+"-"+std::to_string(distance)+"tile.ppm",silhouettes,expedition,distance);
 std::ofstream summary(combined+"/occlusion-summary.txt");assert(summary);summary<<std::fixed<<std::setprecision(4)
  <<"29 original enemies x 4 poses x 2 envelopes x 2 distances = "<<silhouettes.size()<<" silhouettes.\n"
  <<foregroundSamples<<" actual foreground samples; "<<uint64_t(foregroundSamples)*silhouettes.size()<<" comparisons.\n"
  <<"17 action phases plus 1000 recovery, all legal legacy/modern actions/classes; four explicit idle samples.\n"
  <<"Foreground: exact85% dimensions and motion plus1px right, bottom172; unchanged authored crops, >=1.5x source size, visible origins/palms.\n"
  <<"Centered full-opacity, wall-unclipped masks with actual phase recoil only at 1 tile (dungeonEnemyAhead); projected logical 80x86 through the production world-root fit and 2x2 billboard sampling.\n"
  <<realProjectionChecks<<" independent real drawDungeon tagged-palette projection checks, all 3 envelope branches and Ground/Virtual anchors (all 4 poses at 1 tile, idle at 2).\n"
  <<"Protected opaque canonical regions: head x[20,60) y[0,24), torso x[20,60) y[24,62). Nonhumanoid readability cores, not anatomical labels.\n"
  <<"Limits: <=4% opaque overlap at 1 tile; 0 pixels at 2 tiles; 0 protected head/torso pixels.\n"
  <<"Worst 1 tile: "<<maxNearPct<<"%; worst 2 tiles: "<<maxFarPct<<"%; maximum head "<<maxHead<<" pixels, torso "<<maxTorso<<" pixels; failing comparisons "<<thresholdViolations<<".\n"
  <<"Worst ID "<<worst.id<<", pose "<<worst.pose<<", "<<(worst.expedition?"expedition":"normal")<<", class "<<worst.worst.cls<<", action "<<worst.worst.action<<", phase "<<worst.worst.phase<<", recoil "<<worst.worst.recoil<<"; "<<worst.maxCovered<<'/'<<worst.raster[worst.worst.recoil].body.count()<<" opaque pixels.\n"
  <<"QA colors: green enemy, yellow protected head, cyan protected torso, white foreground, magenta overlap.\n";
 summary.close();familyReport.close();occlusion.close();if(maskDump.is_open())maskDump.close();
 printf("Combined 29-original-enemy audit: %u foregrounds x %zu silhouettes, %u projection checks; near %.4f%%, far %.4f%%; protected head %u/torso %u pixels; %u failing comparisons.\n",foregroundSamples,silhouettes.size(),realProjectionChecks,maxNearPct,maxFarPct,maxHead,maxTorso,thresholdViolations);fflush(stdout);
 // Idle/no-effect and expired motion return to the exact stationary frame0.
 for(unsigned cls=0;cls<4;++cls){auto g=hero(cls);ViewState v;v.combatResult=heroCombatResult(g,rpg::Action::Attack);v.effect=Effect::None;Canvas idle;idle.fillScreen(sentinel);drawDungeonHero(idle,g,v);const auto reference=dungeonHeroPose(cls,CombatResult{},0,0);assert(reference.frame==0);
  for(unsigned p:{0u,200u,550u,999u,1000u}){v.effectProgress=p;v.effectElapsedMs=p;Canvas next;next.fillScreen(sentinel);drawDungeonHero(next,g,v);assert(!differs(idle,next));auto pose=dungeonHeroPose(cls,CombatResult{},p,p);assert(pose.frame==0&&pose.x==reference.x&&pose.y==reference.y&&pose.w==reference.w&&pose.h==reference.h&&pose.originX==reference.originX&&pose.originY==reference.originY&&pose.palmX==reference.palmX&&pose.palmY==reference.palmY);}
  auto recovered=dungeonHeroPose(cls,v.combatResult,1000,1200);assert(recovered.frame==0&&recovered.x==reference.x&&recovered.y==reference.y&&recovered.originX==reference.originX&&recovered.originY==reference.originY&&recovered.palmX==reference.palmX&&recovered.palmY==reference.palmY);
 }
 // Mage charge pixels are attached to the exact returned weapon-origin anchor.
 for(auto a:{rpg::Action::Attack,rpg::Action::MagicMissile,rpg::Action::ScorchingRay,rpg::Action::Fireball}){auto g=hero(0);assert(!rpg::act(g,a));ViewState v;v.effect=effectFor(g,a);v.combatResult=heroCombatResult(g,a);v.effectProgress=v.effectElapsedMs=v.effectFrame=0;auto pose=dungeonHeroPose(0,v.combatResult,0,0);Canvas charge;charge.fillScreen(sentinel);CombatCanvas<Canvas> clip{charge,34,172,0,0};assert(heroActionEffect(clip,g,v,true));assert(charge.pixels[pose.originY*240+pose.originX-3]!=sentinel&&charge.pixels[(pose.originY-3)*240+pose.originX]!=sentinel);}
 // Similar legacy effect IDs cannot erase the distinction between actual actions.
 for(bool dungeon:{false,true}){
  auto snapshot=[&](unsigned cls,rpg::Action a){auto g=hero(cls);unsigned hp=g.p.hp;assert(!rpg::act(g,a));ViewState v;v.effect=effectFor(g,a);v.combatResult=heroCombatResult(g,a,hp);v.effectOnHero=onHero(a);v.effectProgress=550;v.effectElapsedMs=605;v.effectFrame=4;Canvas out;out.fillScreen(sentinel);CombatCanvas<Canvas> clip{out,dungeon?34:86,dungeon?172:192,0,0};assert(heroActionEffect(clip,g,v,dungeon));assert(changed(out));return out;};
  auto cone=snapshot(0,rpg::Action::BurningHands),rays=snapshot(0,rpg::Action::ScorchingRay);assert(differs(cone,rays));
  auto heal=snapshot(2,rpg::Action::SecondWind),guard=snapshot(2,rpg::Action::Defensive),surge=snapshot(2,rpg::Action::ActionSurge);assert(differs(heal,guard)&&differs(heal,surge)&&differs(guard,surge));
 }

 assert(rpg::crc(artMemory,ART_BYTES)==ART_CRC);free(artMemory);artMemory=nullptr;
 assert(!thresholdViolations);
 printf("PASS: hero77 %u legal action presentations, %u battle/%u dungeon phases, %u exact personalized sprite frames; stable HUD/touch boundaries, distinct fire/heal/guard/surge, clipping/anchors. Foreground maximum %.2f%% combat band (240x138), %.4f%% near enemy silhouette, %.4f%% far enemy silhouette.\n",actionCases,battleFrames,dungeonFrames,personalFrames,maxPixels*100.0/(240*138),maxNearPct,maxFarPct);
}
