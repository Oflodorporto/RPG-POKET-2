#define main previousSettingsRegression
#include "settings_adapter.cpp"
#undef main
int main(){
 artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);std::ifstream f("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(f);std::vector<uint8_t> original{std::istreambuf_iterator<char>(f),{}};
 assert(original.size()==ART_BYTES+16);using storageDiag::Issue;unsigned count=0;
 for(unsigned fault=0;fault<11;++fault){
  packBytes=original;extraFiles.clear();fileExists=cardAvailable=true;badRead=-1;currentFiles=currentDirs=0;
  if(fault==1)cardAvailable=false;if(fault==2)fileExists=false;if(fault==3)packBytes.pop_back();
  if(fault>=4&&fault<=7)packBytes[(fault-4)*4]^=1;
  if(fault==8)packBytes[12345]^=1;if(fault==9)badRead=0;if(fault==10)badRead=2;
  const auto status=loadSdArt();const auto detail=storageDiag::bootArtFailure;
  const Issue expected[]={Issue::None,Issue::Mount,Issue::Open,Issue::Size,Issue::Magic,Issue::Length,Issue::HeaderCrc,Issue::Catalog,Issue::Crc,Issue::Read,Issue::Read};
  assert(detail.issue==expected[fault]&&detail.stage==storageDiag::Stage::Boot&&detail.file==storageDiag::File::GenericArt&&!currentFiles&&!currentDirs);
  if(!fault)assert(status==ArtStatus::Ready&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);else assert(status!=ArtStatus::Ready);
  if(fault==10)assert(detail.offset==4112&&detail.expected==4096&&detail.actual==0);++count;
 }
 packBytes=original;badRead=-1;cardAvailable=fileExists=true;char named[48];updater::artPath(named,sizeof(named),ART_CRC);extraFiles[named]=original;extraFiles[named][0]^=1;auto before=extraFiles;
 assert(loadSdArt()==ArtStatus::Ready&&!storageDiag::bootArtFailure.valid()&&extraFiles==before&&fileExists);++count;
 fileExists=false;assert(loadSdArt()==ArtStatus::Invalid&&storageDiag::bootArtFailure.file==storageDiag::File::VersionedArt&&storageDiag::bootArtFailure.issue==Issue::Magic);++count;
 free(artMemory);artMemory=nullptr;printf("PASS: %u actual boot-art loader cases; generic/versioned fallback, mount/open/size/header/catalog/read/CRC diagnostics, correct offsets, no premature cleanup or extra descriptors.\n",count);
}
