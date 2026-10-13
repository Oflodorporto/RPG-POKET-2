#define main originalRenderTests
#include "render.cpp"
#undef main
#include <climits>
struct VerifyCanvas:Canvas {
 struct Label {std::string text;int x,y,size;};std::vector<Label> labels;
 void print(const char* p){labels.push_back({p,tx,ty,size});Canvas::print(p);}
 bool has(const char* p)const{for(const auto& l:labels)if(l.text==p)return true;return false;}
};
int main(int argc,char** argv){
 assert(argc==2);VerifyCanvas c;auto g=rpg::create(0,77);ViewState v;v.page=Page::Updates;unsigned count=0;
 for(bool busy:{false,true})for(bool resume:{false,true})for(auto stage:{storageDiag::Stage::Boot,storageDiag::Stage::Downloaded,storageDiag::Stage::Installed})for(auto file:{storageDiag::File::GenericArt,storageDiag::File::ArtPart,storageDiag::File::FirmwarePart,storageDiag::File::VersionedArt})for(unsigned issue=1;issue<=unsigned(storageDiag::Issue::HashApi);++issue)for(int code:{0,INT_MIN,INT_MAX}){
  updateInfo={};updateInfo.state=busy?updater::State::Verifying:resume?updater::State::Error:updater::State::Idle;updateInfo.busy=busy;updateInfo.canResume=resume;updateInfo.progress=80;strcpy(updateInfo.message,"Falha conferir SD / veja detalhe");auto& d=updateInfo.lastVerify;d.stage=stage;d.file=file;d.issue=storageDiag::Issue(issue);d.offset=d.expected=d.actual=UINT32_MAX;d.code=code;
  uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,17,before);c.labels.clear();render(c,g,v,0);rpg::encode(g,17,after);assert(!memcmp(before,after,sizeof(before))&&!artMemory);
  if(!busy){int first=resume?132:119;unsigned rows=0;for(const auto& l:c.labels)if(l.y>=first&&l.y<=first+41){assert(l.y==first+int(rows/2)*11+(rows%2?1:2)&&l.text.size()<=36&&l.size==1);++rows;}assert(rows==8);assert(c.has(resume?"Continuar":"Verificar versao"));}++count;
 }
 // New recovery/identity/promotion rows, including maximum first-error values.
 for(auto stage:{storageDiag::Stage::Identity,storageDiag::Stage::Installing,storageDiag::Stage::Promotion})for(auto file:{storageDiag::File::BackupArt,storageDiag::File::FirmwareTag,storageDiag::File::ArtTag})for(auto issue:{storageDiag::Issue::Read,storageDiag::Issue::Rename,storageDiag::Issue::BackupExists}){
  updateInfo={};updateInfo.state=updater::State::Error;updateInfo.canResume=updateInfo.stagingRetained=true;strcpy(updateInfo.message,"Falha conferir SD / arquivo guardado");auto& f=updateInfo.lastVerify;f.stage=stage;f.file=file;storageDiag::set(&f,issue,UINT32_MAX,4096,0,EIO);updateInfo.recovery.attempts=1;updateInfo.recovery.first=f;updateInfo.recovery.first.code=INT_MIN;
  c.labels.clear();render(c,g,v,0);for(auto& l:c.labels)if(l.y>=119&&l.y<=209)assert(l.text.size()<=36&&l.x>=10&&l.x+int(l.text.size())*6<=230);assert(c.has("Continuar: validar dados salvos")&&c.has("Continuar"));++count;
 }
 // RAM detail remains accessible but must not cover a newer unrelated error/offer.
 updateInfo.busy=false;updateInfo.canResume=false;updateInfo.state=updater::State::Error;strcpy(updateInfo.message,"HTTP 500 / -60dBm");c.labels.clear();render(c,g,v,0);assert(c.has("HTTP 500 / -60dBm"));updateInfo.state=updater::State::Available;strcpy(updateInfo.version,"2026.10.12-visual77");strcpy(updateInfo.message,"Confirmar para baixar e instalar");c.labels.clear();render(c,g,v,0);assert(c.has("Instalar a nova versao e artes?"));
 updateInfo={};updateInfo.state=updater::State::Error;updateInfo.canResume=true;strcpy(updateInfo.message,"Falha conferir SD / veja detalhe");auto& d=updateInfo.lastVerify;d.stage=storageDiag::Stage::Downloaded;d.file=storageDiag::File::ArtPart;storageDiag::set(&d,storageDiag::Issue::Read,4812800,4096,0,EIO);updateInfo.stagingRetained=true;updateInfo.recovery.attempts=1;updateInfo.recovery.first=d;render(c,g,v,0);c.save(std::string(argv[1])+"/ota-verify-read-SIMULATED.ppm");
 updateInfo.state=updater::State::Idle;updateInfo.canResume=false;d.stage=storageDiag::Stage::Boot;d.file=storageDiag::File::GenericArt;storageDiag::set(&d,storageDiag::Issue::Catalog,12,636,488,0);render(c,g,v,0);c.save(std::string(argv[1])+"/ota-boot-catalog-SIMULATED.ppm");
 printf("PASS: %u verification diagnostic frames; four nonoverlapping rows, extreme values, unrelated errors/offers and navigation visible, no artwork or Save28 mutation.\n",count);
}
