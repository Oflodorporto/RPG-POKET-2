#include "../firmware/RPG_POKET_2/Save.h"
#include "../firmware/RPG_POKET_2/AssetCatalog.h"
#include "update_fake/FakeDevice.h"
enum class ArtStatus {Ready,Missing};struct {bool scanning=false,connecting=false;ArtStatus art=ArtStatus::Ready;} menu;
#include "../firmware/RPG_POKET_2/UpdateDevice.h"
#include <fstream>
#include <sstream>
std::string digest(const std::vector<uint8_t>& b){mbedtls_sha256_context c;mbedtls_sha256_init(&c);mbedtls_sha256_update(&c,b.data(),b.size());uint8_t d[32];mbedtls_sha256_finish(&c,d);char out[65];for(unsigned i=0;i<32;++i)snprintf(out+2*i,3,"%02x",d[i]);return out;}
void tag(const char* path,const updater::Package& p){disk[std::string(path)+".tag"]={p.sha,p.sha+64};}
std::vector<uint8_t> firmware,art,oldArt;
void reset(){disk.clear();disk["/RPGPOKET/artes.pak"]=oldArt;disk["/sentinel-save"]={1,2,3,4};deleted.clear();ranges.clear();urls.clear();replies.clear();card=true;writeFault=renameFault=readFault=beginFault=otaWriteFault=endFault=bootFault=false;beginCalls=bootCalls=abortCalls=endCalls=0;openFiles=0;otaOpen=false;ESP.restarted=false;SD.freeBytes=1ull<<30;updateWorker=updater::Info{};updateInfo=updater::Info{};WiFi.state=WL_CONNECTED;maxFiles=1;}
void preserved(){assert(disk["/RPGPOKET/artes.pak"]==oldArt&&disk["/sentinel-save"]==std::vector<uint8_t>({1,2,3,4})&&!openFiles&&!otaOpen);}
void plan(){updatePlan=updater::Plan{};strcpy(updatePlan.version,"2026.10.10-audit71");updatePlan.build=FW_BUILD+1;updatePlan.saveFormat=7;updatePlan.writesSaveFormat=28;updatePlan.readsCurrentSaves=true;updatePlan.assets=1;updatePlan.artCrc=rpg::crc(art.data()+16,art.size()-16);for(auto pair:{std::pair<updater::Package*,std::vector<uint8_t>*>(&updatePlan.firmware,&firmware),{&updatePlan.art,&art}}){auto& p=*pair.first;p.bytes=pair.second->size();strcpy(p.sha,digest(*pair.second).c_str());strcpy(p.url,"https://github.com/Oflodorporto/RPG-POKET-2/releases/download/vtest/file.bin");}}
void cached(){disk["/RPGPOKET/firmware.part"]=firmware;tag("/RPGPOKET/firmware.part",updatePlan.firmware);disk["/RPGPOKET/artes.part"]=art;tag("/RPGPOKET/artes.part",updatePlan.art);}
int main(){
 initUpdater();firmware.resize(10000);for(unsigned i=0;i<firmware.size();++i)firmware[i]=i*7;art.resize(128,0x35);memcpy(art.data(),"PKA1",4);rpg::put32(art.data(),4,112);rpg::put32(art.data(),8,rpg::crc(art.data()+16,112));rpg::put32(art.data(),12,1);oldArt={9,8,7,6};plan();
 // Real cJSON parses published shape, migration coverage, malformed and huge/fractional numbers.
 std::ifstream input("outputs/RPG_POKET_2_0_Waveshare/releases/2026.10.10-eventos8/manifest.json");if(!input)input.open("tests/update_fake/manifest.json");assert(input);std::stringstream ss;ss<<input.rdbuf();auto raw=ss.str();updater::Plan parsed;assert(parseUpdate(raw.c_str(),parsed)&&parsed.readsCurrentSaves&&parsed.writesSaveFormat==28);parsed.build=FW_BUILD+1;assert(updater::valid(parsed,target.size,ESP.getPsramSize()));
 auto j=cJSON_Parse(raw.c_str());cJSON_DeleteItemFromObjectCaseSensitive(j,"reads_save_formats");char* text=cJSON_PrintUnformatted(j);assert(!parseUpdate(text,parsed));cJSON_free(text);cJSON_Delete(j);
 j=cJSON_Parse(raw.c_str());auto reads=cJSON_GetObjectItem(j,"reads_save_formats");cJSON_DeleteItemFromArray(reads,27);text=cJSON_PrintUnformatted(j);assert(!parseUpdate(text,parsed));cJSON_free(text);cJSON_Delete(j);
 j=cJSON_Parse(raw.c_str());cJSON_ReplaceItemInObject(j,"build",cJSON_CreateNumber(1e100));text=cJSON_PrintUnformatted(j);assert(!parseUpdate(text,parsed));cJSON_free(text);cJSON_Delete(j);assert(!parseUpdate("{broken",parsed));auto p=updatePlan;p.writesSaveFormat=27;assert(!updater::valid(p,target.size,ESP.getPsramSize()));p=updatePlan;p.firmware.bytes=target.size+1;assert(!updater::valid(p,target.size,ESP.getPsramSize()));p=updatePlan;p.art.bytes=ESP.getPsramSize();assert(!updater::valid(p,target.size,ESP.getPsramSize()));
 // A truncated response leaves complete blocks and resumes at exactly their offset.
 reset();replies.push_back({200,firmware,-2,"","",5000});std::vector<uint8_t> rest(firmware.begin()+4096,firmware.end());replies.push_back({206,rest,-2,"bytes 4096-9999/10000"});assert(downloadUpdate(updatePlan.firmware,"/RPGPOKET/firmware.part",0,25));assert(disk["/RPGPOKET/firmware.part"]==firmware&&ranges.size()==2&&ranges[1]=="bytes=4096-");preserved();
 // Exhausting retries preserves a resumable prefix; a later installation completes it.
 reset();replies.push_back({200,firmware,-2,"","",5000});for(unsigned i=0;i<4;++i)replies.push_back({206,rest,-2,"bytes 4096-9999/10000","",1000});assert(!installUpdate(false)&&updateWorker.canResume&&!ESP.restarted&&!bootCalls&&disk["/RPGPOKET/firmware.part"].size()==4096);preserved();
 replies.push_back({206,rest,-2,"bytes 4096-9999/10000"});replies.push_back({200,art});assert(installUpdate(false)&&ESP.restarted&&flashed==firmware);preserved();
 // Wrong Content-Range is rejected; a server ignoring Range safely restarts from zero.
 reset();disk["/RPGPOKET/firmware.part"]={firmware.begin(),firmware.begin()+4096};tag("/RPGPOKET/firmware.part",updatePlan.firmware);replies.push_back({206,rest,-2,"bytes 0-9999/10000"});replies.push_back({200,firmware});assert(downloadUpdate(updatePlan.firmware,"/RPGPOKET/firmware.part",0,25)&&disk["/RPGPOKET/firmware.part"]==firmware);preserved();
 reset();for(unsigned i=0;i<5;++i)replies.push_back({500,{}});assert(!installUpdate(false)&&!bootCalls&&!ESP.restarted);preserved();
 for(unsigned fault=0;fault<8;++fault){reset();cached();if(fault==0)disk["/RPGPOKET/firmware.part"][3]^=1;if(fault==1)disk["/RPGPOKET/artes.part"][20]^=1;if(fault==2)renameFault=true;if(fault==3)beginFault=true;if(fault==4)otaWriteFault=true;if(fault==5)endFault=true;if(fault==6)bootFault=true;if(fault==7){readFault=true;replies.push_back({200,firmware});replies.push_back({200,art});}
  assert(!installUpdate(false)&&!ESP.restarted);assert(bootCalls==unsigned(fault==6));assert(abortCalls==unsigned(fault==4));assert(endCalls==unsigned(fault==5||fault==6));preserved();}
 reset();SD.freeBytes=1;assert(!installUpdate(false)&&!beginCalls);preserved();reset();card=false;assert(!installUpdate(false)&&!beginCalls);preserved();
 reset();cached();assert(installUpdate(false)&&ESP.restarted&&bootCalls==1&&flashed==firmware);preserved();assert(!disk.count("/RPGPOKET/firmware.part"));
 reset();cached();assert(installUpdate(true)&&ESP.restarted&&!bootCalls&&!beginCalls);preserved();
 // Changed identity discards stale partial data. Write failure cannot retain a misleading tag.
 reset();disk["/RPGPOKET/firmware.part"]={1,2,3};disk["/RPGPOKET/firmware.part.tag"]=std::vector<uint8_t>(64,'f');assert(partialUpdate(updatePlan.firmware,"/RPGPOKET/firmware.part")==0);writeFault=true;assert(!downloadUpdate(updatePlan.firmware,"/RPGPOKET/firmware.part",0,25));preserved();
 puts("PASS: actual OTA adapter/real JSON; save compatibility/partition/PSRAM; dropped HTTP resumes4096, invalid ranges/200 restart; HTTP/SD/hash/rename/OTA begin/write/end/boot faults preserve active arts and sentinel saves; successful/art-only installation; resources closed");
}
