#define main originalRenderTests
#include "render.cpp"
#undef main
#include <climits>
struct DiagnosticCanvas:Canvas {std::vector<std::string> labels;void print(const char* p){labels.emplace_back(p);Canvas::print(p);}bool has(const char* p){return std::find(labels.begin(),labels.end(),p)!=labels.end();}};
int main(int argc,char** argv){
 assert(argc==2);DiagnosticCanvas c;auto g=rpg::create(0,77);ViewState v;v.page=Page::Updates;unsigned cases=0;
 for(bool busy:{false,true})for(bool resume:{false,true})for(bool append:{false,true})for(bool known:{false,true})for(auto file:{updater::DownloadFile::Firmware,updater::DownloadFile::Art})for(int error:{0,28,INT_MIN,INT_MAX}){
  updateInfo=updater::Info{};updateInfo.state=busy?updater::State::Downloading:resume?updater::State::Error:updater::State::Idle;updateInfo.busy=busy;updateInfo.canResume=resume;updateInfo.progress=25;updateInfo.file=file;
  strcpy(updateInfo.message,"SD: gravacao falhou / -71dBm");strcpy(updateInfo.version,"2026.10.12-abcdefghijklmnopqrstuvwxyz12");
  auto& f=updateInfo.lastWrite;f.valid=true;f.file=file;f.append=append;f.freeKnown=known;f.initialFree=UINT64_MAX;f.offset=UINT32_MAX;f.total=UINT32_MAX;f.requested=UINT32_MAX;f.written=UINT32_MAX;f.progress=100;f.error=error;
  uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,77,before);c.labels.clear();render(c,g,v,0);rpg::encode(g,77,after);assert(!memcmp(before,after,sizeof(before)));assert(!artMemory);
  if(busy)assert(c.has(file==updater::DownloadFile::Art?"Baixando artes...":"Baixando firmware..."));
  else {assert(c.has("Pos: 4294967295/4294967295 B"));assert(c.has("Gravou 4294967295 de 4294967295 B"));assert(c.has(known?"Livre antes: 18014398509481983 KiB":"Livre antes: nao medido"));assert(c.has(resume?"Continuar":"Verificar versao"));}
  ++cases;
 }
 updateInfo=updater::Info{};updateInfo.state=updater::State::Error;updateInfo.canResume=true;strcpy(updateInfo.version,"2026.10.12-visual77");strcpy(updateInfo.message,"SD: gravacao falhou / -71dBm");auto& f=updateInfo.lastWrite;f.valid=true;f.file=updater::DownloadFile::Art;f.offset=4096;f.total=6661456;f.requested=4096;f.written=512;f.progress=25;f.error=28;f.freeKnown=true;f.initialFree=11000000;
 render(c,g,v,0);c.save(std::string(argv[1])+"/ota-sd-short-write.ppm");
 updateInfo.state=updater::State::Idle;updateInfo.canResume=false;render(c,g,v,0);c.save(std::string(argv[1])+"/ota-sd-ram-recall.ppm");
 // A retained SD record must not conceal a newer unrelated failure or offer.
 updateInfo.state=updater::State::Error;strcpy(updateInfo.message,"HTTP 500 / -60dBm");c.labels.clear();render(c,g,v,0);assert(c.has("HTTP 500 / -60dBm"));
 updateInfo.state=updater::State::Available;strcpy(updateInfo.message,"Confirmar para baixar e instalar");c.labels.clear();render(c,g,v,0);assert(c.has("Instalar a nova versao e artes?")&&c.has("2026.10.12-visual77"));
 printf("PASS: %u SD diagnostics frames plus real example/RAM recall/new-error visibility; extreme values fit 240x320, no artwork needed, Save28 unchanged.\n",cases);
}
