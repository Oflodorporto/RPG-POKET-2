#ifndef D20_CAMP77_SCENE_HEADER
#define D20_CAMP77_SCENE_HEADER "SceneCanvas.h"
#endif
#include D20_CAMP77_SCENE_HEADER
#include <filesystem>
int main(int argc,char**argv){assert(argc==2);std::filesystem::create_directories(argv[1]);std::vector<uint8_t> data(ART_BYTES);artMemory=data.data();std::ifstream f("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(f);f.seekg(16);f.read((char*)artMemory,ART_BYTES);assert(f.gcount()==ART_BYTES);menu=MenuState{};menu.art=ArtStatus::Ready;menu.clockValid=true;menu.dayCycle=true;
 auto g=rpg::create(0,17);g.race=2;g.shirt=5;g.trousers=7;g.city=0;g.rations=6;g.sleepKit=true;assert(!rpg::prepareDailyTrip(g,3));ViewState v;Canvas c;v.page=Page::TravelRoll;menu.rollStarted=1000;g.tripRoll=17;g.tripTotal=22;g.tripDifficulty=13;
 for(unsigned i=0;i<16;++i){menu.renderNow=1000+i*120;menu.rollReady=i==15;render(c,g,v,i);c.save(std::string(argv[1])+"/roll-"+(i<10?"0":"")+std::to_string(i)+".ppm");}
 for(unsigned result=1;result<=20;++result){menu.rollReady=true;g.tripRoll=result;g.tripTotal=result+5;render(c,g,v,0);c.save(std::string(argv[1])+"/face-"+std::to_string(result)+".ppm");}
 g.daily.phase=rpg::DailyPhase::NightRoll;g.daily.roll=20;g.daily.total=25;g.daily.difficulty=14;v.page=Page::CampRoll;render(c,g,v,0);c.save(std::string(argv[1])+"/night-result.ppm");
 g.daily.phase=rpg::DailyPhase::Tent;g.daily.day=1;v.page=Page::Travel;render(c,g,v,0);c.save(std::string(argv[1])+"/tent-road.ppm");
 g.daily.day=2;render(c,g,v,0);c.save(std::string(argv[1])+"/tent-city.ppm");
 g=rpg::create(0,42);g.campRoll=17;g.campRation=true;g.campKit=true;v.page=Page::CampRoll;render(c,g,v,0);c.save(std::string(argv[1])+"/camp-result.ppm");
}
