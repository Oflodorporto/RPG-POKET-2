#define main priorControllerRegression
#include "title_controller.cpp"
#undef main
static void setup(bool modern,unsigned cls){
 nvs=Store{};backend.slot=0;assert(journal.load(game)==rpg::Load::Empty);game=modern?rpg::create(cls,42):rpg::testHero(cls,42);game.tutorial=true;assert(rpg::begin(game,0));assert(journal.save(game));view=ViewState{};view.page=Page::Skills;menu=MenuState{};combatFx=CombatFx{};now=100;
}
int main(){unsigned cases=0;const auto r=panelUi::skillsGuide;
 for(bool modern:{false,true})for(unsigned cls=0;cls<4;++cls){
  for(int x:{r.x,r.x+r.w-1})for(int y:{r.y,r.y+r.h-1}){setup(modern,cls);auto before=game;auto blobs=nvs.blobs;tapped(x,y);assert(view.page==Page::Guide&&view.guideReturn==Page::Skills&&view.guideIndex==1);assert(!memcmp(&game,&before,sizeof(game))&&nvs.blobs==blobs);tapped(120,290);assert(view.page==Page::Skills);++cases;}
  for(auto xy:{std::pair<int,int>{r.x-1,r.y}, {r.x+r.w,r.y},{r.x,r.y-1},{r.x,r.y+r.h},{38,160}}){setup(modern,cls);auto before=game;tapped(xy.first,xy.second);assert(view.page==Page::Skills&&!memcmp(&game,&before,sizeof(game)));++cases;}
  setup(modern,cls);combatFx.start(Effect::Slash,false,now,{CombatFeedback::Damage,2,false,false});auto before=game;tapped(r.x+2,r.y+2);assert(view.page==Page::Skills&&!memcmp(&game,&before,sizeof(game)));++cases;
  for(int x:{42,197})for(int y:{24,85}){setup(modern,cls);view.page=Page::Battle;auto before=game;auto blobs=nvs.blobs;tapped(x,y);assert(view.page==Page::EnemyInfo&&view.enemyReturn==Page::Battle);assert(!memcmp(&game,&before,sizeof(game))&&nvs.blobs==blobs);tapped(120,295);assert(view.page==Page::Battle);++cases;}
  setup(modern,cls);view.page=Page::Battle;before=game;tapped(120,90);assert(view.page==Page::Battle&&!memcmp(&game,&before,sizeof(game)));++cases;
  setup(modern,cls);view.page=Page::Battle;combatFx.start(Effect::Slash,false,now,{CombatFeedback::Damage,2,false,false});before=game;tapped(120,75);assert(view.page==Page::Battle&&!memcmp(&game,&before,sizeof(game)));++cases;
  // The nearby Skills button remains an action-page transition, never EnemyInfo.
  setup(modern,cls);view.page=Page::Battle;tapped(175,255);assert(view.page==Page::Skills);++cases;
 }
 printf("PASS: %u Guide/Ficha boundary, back, neighboring-action and animation-gating cases; Game and NVS unchanged by information navigation.\n",cases);
}
