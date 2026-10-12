#define main previousOtaRegression
#include "update_adapter.cpp"
#undef main
static std::vector<uint8_t> packFile(const char* path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
 initUpdater();firmware.resize(10000);for(unsigned i=0;i<firmware.size();++i)firmware[i]=i*7;
 art=packFile("cartao/RPGPOKET/artes.pak");oldArt=packFile("build/enemy_transition/old.pak");assert(art.size()==ART_BYTES+16&&rpg::get32(art.data(),12)==ART_COUNT&&oldArt.size()!=art.size());
 plan();strcpy(updatePlan.version,FW_VERSION);updatePlan.build=FW_BUILD;updatePlan.assets=ART_COUNT;assert(updatePlan.artCrc==ART_CRC);assert(updater::valid(updatePlan,target.size,ESP.getPsramSize()));
 char named[48];updater::artPath(named,sizeof(named),ART_CRC);unsigned cases=0;
 for(unsigned fault=0;fault<7;++fault){reset();cached();for(unsigned retry=0;retry<5;++retry)replies.push_back({500,{}});
  if(fault<3){unsigned asset=fault==0?92:unsigned(fault==1?544:600);disk["/RPGPOKET/artes.part"][16+assetMeta[asset].offset]^=1;}
  if(fault==3)beginFault=true;if(fault==4)otaWriteFault=true;if(fault==5)endFault=true;if(fault==6)bootFault=true;
  assert(!installUpdate(false)&&!ESP.restarted);preserved();assert(bootCalls==unsigned(fault==6));++cases;
 }
 // Actual old pack cannot satisfy the paired new hash/count/length/CRC. The active old generic remains.
 reset();cached();disk["/RPGPOKET/artes.part"]=oldArt;for(unsigned retry=0;retry<5;++retry)replies.push_back({500,{}});assert(!installUpdate(false)&&!bootCalls&&!ESP.restarted);preserved();++cases;
 // Resumed candidate download replaces only its temporary file and publishes new CRC identity.
 reset();disk["/RPGPOKET/artes.part"]={art.begin(),art.begin()+4096};tag("/RPGPOKET/artes.part",updatePlan.art);
 std::vector<uint8_t> rest(art.begin()+4096,art.end());std::string range="bytes 4096-"+std::to_string(art.size()-1)+"/"+std::to_string(art.size());replies.push_back({206,rest,-2,range});assert(downloadUpdate(updatePlan.art,"/RPGPOKET/artes.part",25,70));assert(disk["/RPGPOKET/artes.part"]==art&&ranges.back()=="bytes=4096-");preserved();++cases;
 reset();cached();assert(installUpdate(false)&&ESP.restarted&&bootCalls==1&&flashed==firmware&&disk[named]==art);preserved();++cases;
 reset();cached();assert(installUpdate(true)&&ESP.restarted&&!bootCalls&&!beginCalls&&disk[named]==art);preserved();++cases;
 printf("PASS: %u actual old/newpack OTA gates; body/effect/palette corruption, wrongpack, inactive write/begin/end/boot failures, resume, paired publish and same-build art repair preserve old generic and Save28 sentinel.\n",cases);
}
