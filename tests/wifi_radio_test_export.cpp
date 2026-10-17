// Host-only transactional CSV/error-path tests. No SD card or hardware claim.
#include "WifiRadioTestExport.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <array>
using namespace wifi_radio_test;
struct Storage {
 enum Fault {None,Mount,Open,WriteHeader,WriteRow,Flush,Verify,Corrupt};
 Fault fault=None;std::map<std::string,std::string> files;std::string current;bool mounted=false,opened=false,race=false;
 unsigned mounts=0,unmounts=0,opens=0,writes=0,closes=0,verifies=0;
 bool mount(){++mounts;mounted=fault!=Mount;return mounted;}
 void unmount(){assert(mounted);mounted=false;++unmounts;}
 bool exists(const char*path){return files.count(path)!=0;}
 bool openNew(const char*path){assert(mounted);++opens;if(race){files[path]="concurrent original";return false;}if(fault==Open||files.count(path))return false;current=path;files[current]="";opened=true;return true;}
 size_t write(const uint8_t*data,size_t n){assert(opened);++writes;if((fault==WriteHeader&&writes==1)||(fault==WriteRow&&writes==2)){files[current].append(reinterpret_cast<const char*>(data),n/2);return n/2;}files[current].append(reinterpret_cast<const char*>(data),n);return n;}
 void close(){assert(opened);opened=false;++closes;}
 bool flushClose(){close();if(fault==Corrupt&&!files[current].empty())files[current][0]^=1;return fault!=Flush;}
 bool verify(const char*path,uint32_t bytes,uint32_t crc){++verifies;assert(!opened);if(fault==Verify)return false;const auto&data=files.at(path);return data.size()==bytes&&~csvCrc(~0u,reinterpret_cast<const uint8_t*>(data.data()),data.size())==crc;}
};
static std::vector<std::vector<std::string>> parse(const std::string&csv){
 std::vector<std::vector<std::string>> rows;std::vector<std::string> row;std::string value;bool quoted=false;
 for(size_t i=0;i<csv.size();++i){char c=csv[i];if(c=='"'){if(quoted&&i+1<csv.size()&&csv[i+1]=='"'){value+='"';++i;}else quoted=!quoted;}else if(!quoted&&(c==','||c=='\n')){row.push_back(value);value.clear();if(c=='\n'){rows.push_back(row);row.clear();}}else value+=c;}
 assert(!quoted&&row.empty()&&value.empty());return rows;
}
static std::string field(const std::vector<std::vector<std::string>>&rows,const char*key,size_t row=1){for(size_t i=0;i<rows[0].size();++i)if(rows[0][i]==key)return rows[row][i];assert(false);return {};}
static Report report(){Report r;r.session=1;r.run=7;r.stage=1;r.distance=Distance::TwoPalms;r.level=Level::Baseline;r.localPower={80,80,true,0};r.local.clockDomain=123;r.local.txAttempts=30;r.local.apiAccepted=29;r.local.apiErrors=1;r.local.uniqueRx=23;r.localStarted=true;r.local.startedAt=100;r.local.endedAt=22800;r.local.timingValid=true;r.localRestoreAttempted=true;r.localRestore={80,80,true,0};r.localBaselineRaw=80;r.localMac[0]=2;r.localMac[5]=1;return r;}
int main(){
 const char*check="123456789";assert(~csvCrc(~0u,reinterpret_cast<const uint8_t*>(check),9)==0xcbf43926u);
 Report r=report();const Report* reports[]={&r};CsvContext context{"v1,\"quoted\"\nsecond line",2026101701,3,1,0};std::array<unsigned char,sizeof(Report)> original{};std::memcpy(original.data(),&r,sizeof(r));
 Storage okay;auto result=exportReports(okay,reports,1,context,0x1234);assert(result.error==ExportError::None&&result.bytes>0&&result.crc);assert(!okay.opened&&!okay.mounted&&okay.mounts==1&&okay.unmounts==1&&okay.closes==1&&okay.verifies==1);assert(std::string(result.path)=="/RPGPOKET/radio-00001234.csv");auto parsed=parse(okay.files.at(result.path));assert(parsed.size()==2&&parsed[0].size()==107&&parsed[1].size()==107);assert(field(parsed,"local_recorder_firmware_version")==context.firmwareVersion);assert(field(parsed,"local_recorder_build_number")=="2026101701");assert(field(parsed,"local_recorder_usb_phy_compiled")=="1"&&field(parsed,"local_recorder_reduce_tx_power_compiled")=="0");assert(field(parsed,"remote_probe_api_accepted")=="-1"&&field(parsed,"remote_unique_rx")=="-1"&&field(parsed,"remote_actual_raw")=="-1");assert(field(parsed,"local_to_remote_loss_permille")=="-1"&&field(parsed,"comparison")=="not_measured");assert(field(parsed,"local_mac_success")=="-1"&&field(parsed,"local_mac_telemetry_valid")=="0");assert(!std::memcmp(original.data(),&r,sizeof(r)));
 auto first=okay.files;auto second=exportReports(okay,reports,1,context,0x1234);assert(second.error==ExportError::None&&std::string(second.path)=="/RPGPOKET/radio-00001235.csv"&&okay.files.begin()->second==first.begin()->second);
 const Storage::Fault faults[]={Storage::Mount,Storage::Open,Storage::WriteHeader,Storage::WriteRow,Storage::Flush,Storage::Verify,Storage::Corrupt};const ExportError errors[]={ExportError::Mount,ExportError::Open,ExportError::Write,ExportError::Write,ExportError::Flush,ExportError::Readback,ExportError::Readback};
 for(unsigned i=0;i<7;++i){Storage s;s.fault=faults[i];s.files["/RPGPOKET/keep.csv"]="original";auto e=exportReports(s,reports,1,context,5);assert(e.error==errors[i]&&!s.opened&&!s.mounted);assert(s.files.at("/RPGPOKET/keep.csv")=="original");assert(!std::memcmp(original.data(),&r,sizeof(r)));assert(s.unmounts==(i?1u:0u));}
 Storage occupied;for(unsigned i=0;i<16;++i){char path[48];std::snprintf(path,sizeof(path),"/RPGPOKET/radio-%08x.csv",i);occupied.files[path]="keep";}auto kept=occupied.files;assert(exportReports(occupied,reports,1,context,0).error==ExportError::Names&&occupied.files==kept&&occupied.opens==0);
 Storage raced;raced.race=true;assert(exportReports(raced,reports,1,context,77).error==ExportError::Open);assert(raced.files.begin()->second=="concurrent original");
 Storage empty;assert(exportReports(empty,reports,0,context,1).error==ExportError::Empty&&empty.mounts==0);assert(exportReports(empty,reports,kMaxReports+2,context,1).error==ExportError::Empty&&empty.mounts==0);const Report* nullReport[]={nullptr};assert(exportReports(empty,nullReport,1,context,1).error==ExportError::Encode&&!empty.opened&&!empty.mounted);
 std::string oversized(8192,'x');CsvContext longContext=context;longContext.firmwareVersion=oversized.c_str();Storage tooLong;assert(exportReports(tooLong,reports,1,longContext,1).error==ExportError::Encode&&!tooLong.opened&&!tooLong.mounted);assert(!std::memcmp(original.data(),&r,sizeof(r)));
 const Report* maximum[kMaxReports+1];for(auto& p:maximum)p=&r;Storage bounded;assert(exportReports(bounded,maximum,kMaxReports+1,context,2).error==ExportError::None);auto maxRows=parse(bounded.files.begin()->second);assert(maxRows.size()==kMaxReports+2);
 uint32_t fingerprint=reportFingerprint(reports,1,context);assert(fingerprint&&fingerprint==reportFingerprint(reports,1,context));r.local.apiAccepted--;r.local.apiErrors++;assert(fingerprint!=reportFingerprint(reports,1,context));std::memcpy(&r,original.data(),sizeof(r));CsvContext changed=context;changed.resetReason++;assert(fingerprint!=reportFingerprint(reports,1,changed));assert(reportFingerprint(nullReport,1,context)==0);
 Report near;near.reason=Reason::PairTimeout;const Report* preflight[]={&near};Storage pf;assert(exportReports(pf,preflight,1,CsvContext{},8).error==ExportError::None);assert(field(parse(pf.files.begin()->second),"distance")=="pareamento_perto");
 std::puts("wifi_radio_test_export PASS: mount/open/partialwrite/flush/readback/CRC/collision/nooverwrite faults;107 columns; quotedmetadata; unknownremote/MAC;RAM retained;25rows bounded");
}
