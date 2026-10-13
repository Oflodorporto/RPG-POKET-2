#define main previousTitleController
#include "title_controller.cpp"
#undef main
static void setup(unsigned city,unsigned slot,bool modern,unsigned cls){nvs=Store{};backend.slot=slot;assert(journal.load(game)==rpg::Load::Empty);game=modern?rpg::create(cls,42):rpg::testHero(cls,42);game.city=city;game.tutorial=true;assert(journal.save(game));view=ViewState{};view.page=Page::Village;menu=MenuState{};menu.sessionStarted=true;combatFx=CombatFx{};now=100;}
static void tap(scenicUi::Rect r){tapped(r.x+r.w/2,r.y+r.h/2);}
static void same(const rpg::Game& g,const std::map<std::string,std::vector<uint8_t>>& b){assert(!memcmp(&game,&g,sizeof(g))&&nvs.blobs==b);}
int main(){unsigned cases=0;const Page targets[]={Page::People,Page::Market,Page::Inventory,Page::Character,Page::Explore,Page::Guild,Page::CampSetup,Page::Map,Page::Menu};
 for(unsigned city:{2u,3u})for(unsigned slot=0;slot<3;++slot)for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned page=0;page<2;++page){
 for(unsigned i=0;i<5;++i){auto r=i==4?carvalhoUi::menu:carvalhoUi::actions[i];unsigned id=i==4?8:carvalhoUi::actionIds[page][i];
 for(int x:{r.x,r.x+r.w-1})for(int y:{r.y,r.y+r.h-1}){setup(city,slot,modern,cls);view.cityPage=page;auto before=game;auto blobs=nvs.blobs;nvs.fail=true;tapped(x,y);assert(view.page==targets[id]);same(before,blobs);++cases;}
 setup(city,slot,modern,cls);view.cityPage=page;auto before=game;auto blobs=nvs.blobs;tap(r);
 if(id==0||id==5)tapped(120,290);else if(id==1||id==2||id==4)tapped(175,290);else if(id==3)tapped(120,289);else if(id==6){tapped(60,290);assert(view.page==Page::Explore);tapped(175,290);}else{if(id==8)tapped(scenicUi::menuX+20,scenicUi::menuYs[0]+10);tap(scenicUi::mapButtons[0]);}
 assert(view.page==Page::Village&&view.cityPage==(id>=7?0:page));same(before,blobs);++cases;
 }
 setup(city,slot,modern,cls);view.cityPage=page;auto before=game;auto blobs=nvs.blobs;tap(carvalhoUi::mission);assert(view.page==Page::Campaign);tapped(175,290);assert(view.page==Page::Village&&view.cityPage==page);tap(carvalhoUi::pager);assert(view.cityPage==1-page);same(before,blobs);combatFx.start(Effect::Slash,false,now,{});tap(carvalhoUi::pager);tap(carvalhoUi::menu);assert(view.page==Page::Village&&view.cityPage==1-page);same(before,blobs);++cases;
 }
 // Current preflight cancellation/rejection never spends or rolls. Full accepted
 // daily Save29 flow is exercised by scenes29_controller.
 for(unsigned city=0;city<4;++city)for(unsigned dest=0;dest<4;++dest)if(city!=dest){setup(city,0,true,0);game.rations=6;game.charts=game.charms=2;view.page=Page::TravelConfirm;view.tripDestination=dest;auto before=game;auto blobs=nvs.blobs;tap(launchUi::cancel);assert(view.page==Page::Map);same(before,blobs);view.page=Page::TravelConfirm;game.rations=0;before=game;tap(launchUi::confirm);assert(view.page==Page::TravelConfirm);same(before,blobs);++cases;}
 printf("PASS: %u Mares/Aurora/controller cases; all destinations/back/pages, state/slot isolation, animation lock, real trip cancel/confirm once.\n",cases);
}
