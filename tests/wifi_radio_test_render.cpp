#if RPG_ENABLE_WIFI_RADIO_TEST
#include "WifiRadioRuntimeHarness.h"
#include "WifiRadioSimulation.h"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "SceneCanvas.h"
#pragma GCC diagnostic pop
#endif
#include <filesystem>
#include <cstring>
static unsigned captures=0;
#if RPG_ENABLE_WIFI_RADIO_TEST
using namespace wifi_radio_ui;
static void verifyLayout(){assert(!screen.clipped);for(unsigned i=0;i<screen.lineCount;++i){const auto& l=screen.lines[i];assert(l.y>=0&&l.y+l.scale*8<=320);for(unsigned j=0;j<screen.buttonCount;++j){const auto& b=screen.buttons[j];assert(l.y+l.scale*8<=b.y||l.y>=b.y+b.h);}for(unsigned j=i+1;j<screen.lineCount;++j){const auto& r=screen.lines[j];assert(l.y+l.scale*8<=r.y||r.y+r.scale*8<=l.y);}}for(unsigned i=0;i<screen.buttonCount;++i){const auto& b=screen.buttons[i];assert(b.x>=0&&b.y>=0&&b.x+b.w<=240&&b.y+b.h<=320);for(unsigned j=i+1;j<screen.buttonCount;++j){const auto& r=screen.buttons[j];assert(b.x+b.w<=r.x||r.x+r.w<=b.x||b.y+b.h<=r.y||r.y+r.h<=b.y);}}}
static void capture(const std::string& out,const char* name){buildScreen(fake::timeMs);verifyLayout();Canvas c;auto before=game;const auto s=screen;render(c,game,view,0);assert(!memcmp(&before,&game,sizeof game)&&!memcmp(&s,&screen,sizeof screen));c.save(out+"/"+name+".ppm");++captures;}
#endif
int main(int argc,char**argv){assert(argc==2);std::string out=argv[1];std::filesystem::create_directories(out);artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);makeFallback();
#if RPG_ENABLE_WIFI_RADIO_TEST
 resetRuntime();assert(wifiRadioTestOpen());capture(out,"lobby");fake::timeMs+=31000;wifiRadioTestTick(fake::timeMs);capture(out,"discovery-timeout");
 Rig r;r.pair();device.flow()=r.f[0];fake::timeMs=r.now;capture(out,"distance-two-palms");r.bothManual(Distance::TwoPalms,Level::N1);device.flow()=r.f[0];fake::timeMs=r.now;capture(out,"coordinating");r.running();r.advance(5000);device.flow()=r.f[0];fake::timeMs=r.now;capture(out,"running");r.report(1);device.flow()=r.f[0];fake::timeMs=r.now;capture(out,"manual-levels");manualDistance=Distance::OneMeter;capture(out,"manual-one-meter");reportOpen=true;reportIndex=0;capture(out,"report-complete");for(unsigned i=1;i<=4;++i){detailPage=i;capture(out,("details-"+std::to_string(i)).c_str());}
 Rig missing;missing.pair();missing.drop=[](const Packet&p){return p.wire.type==Type::Preliminary||p.wire.type==Type::Final;};missing.bothManual(Distance::OneMeter,Level::N2);missing.report(1);device.flow()=missing.f[0];fake::timeMs=missing.now;detailPage=0;capture(out,"report-missing-peer");for(unsigned i=1;i<=3;++i){detailPage=i;capture(out,("missing-details-"+std::to_string(i)).c_str());}
 Rig rescue;rescue.pair();rescue.dropBaseline=true;rescue.bothContinue();rescue.report(1);device.flow()=rescue.f[0];fake::timeMs=rescue.now;reportOpen=false;capture(out,"baseline-not-coordinated");rescue.bothManual(Distance::OneMeter,Level::N1);rescue.report(2);device.flow()=rescue.f[0];fake::timeMs=rescue.now;reportOpen=true;reportIndex=1;detailPage=0;capture(out,"report-recovery");reportOpen=false;fake::failures[fake::SetChannel]=1;assert(!closeDevice());assert(device.status().restoreError!=0);capture(out,"restore-error");assert(closeDevice());
 ViewState v;v.page=Page::NetworkTest;updateInfo.tested=true;updateInfo.netTrials=3;updateInfo.netSuccess=2;Canvas network;render(network,game,v,0);network.save(out+"/internet-test-preserved.ppm");++captures;
 for(bool saved:{false,true}){v.page=Page::Wifi;menu.savedNetworks=saved;menu.savedCount=1;menu.networkCount=1;strcpy(menu.network,"Rede de teste");Canvas wifi;render(wifi,game,v,0);wifi.save(out+(saved?"/wifi-saved-entry.ppm":"/wifi-browse-entry.ppm"));++captures;}
#else
 rpg::Game game;ViewState view;view.page=Page::NetworkTest;updateInfo.tested=true;updateInfo.netTrials=3;updateInfo.netSuccess=2;Canvas network;auto before=game;render(network,game,view,0);assert(!memcmp(&game,&before,sizeof game));network.save(out+"/internet-test-radio-off.ppm");++captures;
#endif
 free(artMemory);artMemory=nullptr;printf("PASS render: %u actual renderer captures, bounded nonoverlapping text/buttons, read-only game and snapshot\n",captures);}
