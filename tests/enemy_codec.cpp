#include "../firmware/RPG_POKET_2/EnemyArt.h"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdio>
#include <algorithm>
#include <cstring>
static std::vector<uint8_t> read(const std::string& path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
 const std::string root="build/enemy_art_source/exports";
 auto pack=read("cartao/RPGPOKET/artes.pak");assert(pack.size()==ART_BYTES+16);
 std::vector<uint8_t> memory(ART_BYTES+64,0x5a);artMemory=memory.data()+32;memcpy(artMemory,pack.data()+16,ART_BYTES);
 std::vector<uint16_t> buffer(enemyArt::WORKSPACE_PIXELS+2,0x1234);auto out=buffer.data()+1;enemyArt::workspace=out;
 unsigned frames=0,ids=0;const char* poses[]={"idle","anticipation","attack","hurt"};
 auto compare=[&](const std::string& path,enemyArt::Frame frame){
  auto data=read(path+".idx4"),expected=read(path+".rgb565le");
  auto p=std::filesystem::path(path).parent_path();auto palette=read((p/"palette.rgb565le").string());
  assert(data.size()==frame.bytes&&expected.size()==frame.w*frame.h*2u);
  assert(indexed4::decode(data.data(),data.size(),palette.data(),palette.size(),frame.w,frame.h,frame.stride,out,enemyArt::WORKSPACE_PIXELS));
  for(unsigned i=0;i<frame.w*frame.h;++i)assert(out[i]==indexed4::little16(expected.data()+2*i));
  auto image=enemyArt::decode(frame);assert(image.pixels==out&&image.w==frame.w&&image.h==frame.h);
  for(unsigned i=0;i<frame.w*frame.h;++i)assert(out[i]==indexed4::little16(expected.data()+2*i));
  assert(buffer.front()==0x1234&&buffer.back()==0x1234);
  auto saved=buffer;
  for(unsigned bad=0;bad<8;++bad){auto pal=palette;if(bad==7)pal[0]^=1;
   assert(!indexed4::decode(bad==0?nullptr:data.data(),data.size()-(bad==1),pal.data(),bad==2?31:pal.size(),bad==3?81:frame.w,frame.h,bad==4?frame.stride-1:frame.stride,bad==5?nullptr:out,bad==6?frame.w*frame.h-1:enemyArt::WORKSPACE_PIXELS));assert(buffer==saved);
  }
  std::fill(data.begin(),data.end(),0);assert(indexed4::decode(data.data(),data.size(),palette.data(),palette.size(),frame.w,frame.h,frame.stride,out,enemyArt::WORKSPACE_PIXELS));
  for(unsigned i=0;i<frame.w*frame.h;++i)assert(out[i]==0xf81f);
  ++frames;
 };
 for(auto& entry:std::filesystem::directory_iterator(root+"/enemies")){
  auto name=entry.path().filename().string();unsigned id=std::stoi(name.substr(0,2));assert(id<29);++ids;
  for(unsigned pose=0;pose<4;++pose)compare(entry.path().string()+"/"+poses[pose],enemyArt::body(id,enemyArt::Pose(pose)));
 }
 assert(ids==29);
 const char* families[]={"slash","bite","charge","claw","magic","pulse","crush"};
 for(unsigned family=0;family<7;++family)for(unsigned frame=0;frame<8;++frame){char number[4];snprintf(number,sizeof(number),"%02u",frame);compare(root+"/effects/"+families[family]+"/"+number,enemyArt::effect(enemyArt::Family(family),frame));}
 assert(frames==172);
 for(unsigned id=0;id<29;++id)for(auto area:{std::pair<int,int>{75,80},{46,49},{34,31},{48,64},{64,80},{8,11}}){
  auto f=enemyArt::body(id,enemyArt::Pose::Idle);auto place=enemyArt::fit(f,3,7,area.first,area.second);
  assert(place.x>=3&&place.y>=7&&place.x+place.w<=3+area.first&&place.y+place.h<=7+area.second);
  auto world=enemyArt::fitWorld(f,126,172,area.first,area.second);
  assert(world.w==place.w&&world.h==place.h&&world.anchorX==126);
  assert(world.x+f.anchorX*world.w/f.w==126);
  if(f.anchor==enemyArt::Anchor::Ground)assert(world.anchorY==172);
  else assert(world.y+world.h==172&&world.anchorY==172-world.h+f.anchorY*world.h/f.h);
  assert(abs(place.w*86-place.h*80)<=86); // At most one output-pixel rounding.
  for(unsigned pose=1;pose<4;++pose){auto q=enemyArt::fit(enemyArt::body(id,enemyArt::Pose(pose)),3,7,area.first,area.second);assert(!memcmp(&q,&place,sizeof(q)));}
 }
 assert(!enemyArt::decode(enemyArt::body(29,enemyArt::Pose::Idle)).pixels);
 auto bad=enemyArt::body(0,enemyArt::Pose::Idle);bad.palette=92;assert(!enemyArt::decode(bad).pixels);bad=enemyArt::body(0,enemyArt::Pose::Idle);bad.bytes++;assert(!enemyArt::decode(bad).pixels);
 enemyArt::workspace=nullptr;assert(!enemyArt::decode(enemyArt::body(0,enemyArt::Pose::Idle)).pixels);enemyArt::workspace=out;
 makeFallback();
 for(unsigned id=0;id<29;++id)for(unsigned pose=0;pose<4;++pose){auto image=enemyArt::decode(enemyArt::body(id,enemyArt::Pose(pose)));assert(image.pixels);for(unsigned i=0;i<6880;++i)assert(image.pixels[i]==0xf81f);}
 for(unsigned f=0;f<7;++f)for(unsigned p=0;p<8;++p){auto image=enemyArt::decode(enemyArt::effect(enemyArt::Family(f),p));assert(image.pixels);for(unsigned i=0;i<1024;++i)assert(image.pixels[i]==0xf81f);}
 for(unsigned i=0;i<32;++i)assert(memory[i]==0x5a&&memory[memory.size()-1-i]==0x5a);
 assert(buffer.front()==0x1234&&buffer.back()==0x1234);
 // Explicit high-nibble-first, odd-width, row-stride and palette byte order proof.
 uint8_t palette[32]={31,248};for(unsigned i=1;i<16;++i){palette[i*2]=i;palette[i*2+1]=i;}
 uint8_t data[]={0x12,0x30,0x45,0x60};uint16_t sample[6];assert(indexed4::decode(data,4,palette,32,3,2,2,sample,6));
 for(unsigned i=0;i<6;++i)assert(sample[i]==(i+1)*0x101);
 artMemory=nullptr;enemyArt::workspace=nullptr;
 printf("PASS: %u typed compact frames pixel-exact; all29 IDs, uniform fit/anchors, high-nibble RGB565LE, alpha, descriptor validation, fallback and workspace/art canaries.\n",frames);
}
