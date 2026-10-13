#define main previousTitleRegression
#include "title_controller.cpp"
#undef main
static void setup(unsigned slot=0,bool modern=true,unsigned cls=0){nvs=Store{};backend.slot=slot;assert(journal.load(game)==rpg::Load::Empty);game=modern?rpg::create(cls,42):rpg::testHero(cls,42);game.tutorial=true;game.city=0;game.p.gold=24;assert(journal.save(game));view=ViewState{};view.page=Page::Village;menu=MenuState{};menu.sessionStarted=true;combatFx=CombatFx{};now=100;}
static void tap(scenicUi::Rect r){tapped(r.x+r.w/2,r.y+r.h/2);}
static void unchanged(const rpg::Game& before,const std::map<std::string,std::vector<uint8_t>>& blobs){assert(!memcmp(&game,&before,sizeof(game))&&nvs.blobs==blobs);}

int main(){unsigned cases=0;const Page targets[]={Page::People,Page::Market,Page::TownBag,Page::Character,Page::Explore,Page::Guild,Page::CampSetup,Page::Map,Page::Menu};
 for(unsigned slot=0;slot<3;++slot)for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls)for(unsigned page=0;page<2;++page){
  for(unsigned i=0;i<5;++i){auto r=i==4?carvalhoUi::menu:carvalhoUi::actions[i];unsigned id=i==4?8:carvalhoUi::actionIds[page][i];
   for(int x:{r.x,r.x+r.w-1})for(int y:{r.y,r.y+r.h-1}){setup(slot,modern,cls);view.cityPage=page;auto before=game;auto blobs=nvs.blobs;nvs.fail=true;tapped(x,y);assert(view.page==targets[id]&&view.cityPage==page);unchanged(before,blobs);++cases;}
   setup(slot,modern,cls);view.cityPage=page;auto before=game;auto blobs=nvs.blobs;tap(r);
   if(id==0||id==5)tapped(120,290);else if(id==1)tapped(175,290);else if(id==2)tap(panelUi::bagBack);else if(id==3)tapped(120,289);else if(id==4)tapped(175,290);else if(id==6){tapped(60,290);assert(view.page==Page::Explore);tapped(175,290);}else{if(id==8){tapped(scenicUi::menuX+20,scenicUi::menuYs[0]+10);assert(view.page==Page::Map);}tap(scenicUi::mapButtons[0]);}
   assert(view.page==Page::Village&&view.cityPage==(id>=7?0:page));unchanged(before,blobs);++cases;
  }
  setup(slot,modern,cls);view.cityPage=page;auto before=game;auto blobs=nvs.blobs;tap(carvalhoUi::mission);assert(view.page==Page::Campaign&&view.objectiveReturn==Page::Village);tapped(175,290);assert(view.page==Page::Village&&view.cityPage==page);unchanged(before,blobs);++cases;
  auto r=carvalhoUi::pager;for(int x:{r.x,r.x+r.w-1})for(int y:{r.y,r.y+r.h-1}){view.cityPage=page;tapped(x,y);assert(view.page==Page::Village&&view.cityPage==1-page);unchanged(before,blobs);++cases;}
  view.cityPage=page;combatFx.start(Effect::Slash,false,now,{CombatFeedback::Damage,2,false,false});tap(carvalhoUi::pager);tap(carvalhoUi::menu);tap(carvalhoUi::mission);for(auto a:carvalhoUi::actions)tap(a);assert(view.page==Page::Village&&view.cityPage==page);unchanged(before,blobs);++cases;
  combatFx=CombatFx{};view.cityPage=1;selectSlot(slot);assert(!view.cityPage&&view.page==Page::Home);uint8_t savedBefore[rpg::SAVE_SIZE],savedAfter[rpg::SAVE_SIZE];rpg::encode(before,5,savedBefore);rpg::encode(game,5,savedAfter);assert(!memcmp(savedBefore,savedAfter,sizeof(savedBefore))&&before.fatigue==game.fatigue&&nvs.blobs==blobs);++cases;
 }
 // Exhaustive hit partition: action rectangles, mission, pager and gaps never overlap.
 for(unsigned page=0;page<2;++page)for(int y=0;y<320;++y)for(int x=0;x<240;++x){unsigned hits=carvalhoUi::mission.contains(x,y)+carvalhoUi::pager.contains(x,y)+carvalhoUi::menu.contains(x,y);int expected=carvalhoUi::menu.contains(x,y)?8:-1;for(unsigned i=0;i<4;++i)if(carvalhoUi::actions[i].contains(x,y)){++hits;expected=carvalhoUi::actionIds[page][i];}assert(hits<=1&&carvalhoUi::choice(page,x,y)==expected);}
 setup();auto before=game;auto blobs=nvs.blobs;TouchGate hold;hold.update(true,false,0);hold.update(true,false,30);unsigned flips=0;for(unsigned t=31;t<1000;++t)if(hold.update(t%7!=0,true,t)){tap(carvalhoUi::pager);++flips;}assert(flips==1&&view.cityPage==1);hold.update(true,false,1000);hold.update(true,false,1030);assert(hold.update(true,true,1031));tap(carvalhoUi::pager);assert(!view.cityPage);unchanged(before,blobs);++cases;
 setup();view.cityPage=255;assert(carvalhoUi::choice(255,10,180)==0);tap(carvalhoUi::pager);assert(view.cityPage==1);++cases;
 // Other major cities share the approved paging while preserving their destinations.
 for(unsigned city:{2u,3u}){setup();game.city=city;view.cityPage=0;tapped(170,195);assert(view.page==Page::Market&&view.cityPage==0);++cases;}
 printf("PASS: %u real paged city navigation cases;8 destinations+Menu, boundaries/back/reentry/slot/animation/held-input;153600 exhaustive hit pixels; Game/NVS unchanged.\n",cases);
}
