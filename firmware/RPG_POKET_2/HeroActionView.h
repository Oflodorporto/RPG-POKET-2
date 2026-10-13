#pragma once
#include "HeroPresentation.h"
// Local primitives only: no new image payload, framebuffer, heap or random call.
template<class C>void heroFxLine(C& c,int x0,int y0,int x1,int y1,uint16_t color,int width=1){
 int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
 for(;;){c.fillRect(x0,y0,width,width,color);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
}
template<class C>void heroFxRing(C& c,int cx,int cy,int radius,uint16_t color){
 for(int y=-radius;y<=radius;++y)for(int x=-radius;x<=radius;++x){int d=x*x+y*y;if(d<=radius*radius&&d>=(radius-1)*(radius-1))c.fillRect(cx+x,cy+y,1,1,color);}
}
template<class C>void heroFxContact(C& c,int x,int y,unsigned p,uint16_t color){
 if(p<491||p>=700)return;int radius=5+int(p-491)/25;
 for(int k=0;k<8;++k){static const int8_t dx[]={1,-1,0,0,1,-1,1,-1},dy[]={0,0,1,-1,1,1,-1,-1};c.fillRect(x+dx[k]*radius,y+dy[k]*radius,2,2,color);}
}
template<class Canvas>bool heroActionEffect(Canvas& c,const rpg::Game& g,const ViewState& v,bool dungeon,int targetX=-1,int targetY=-1){
 if(v.effect==Effect::None||v.combatResult.enemyAction||!v.combatResult.hasHeroAction)return false;
 using H=HeroChoreography;auto h=heroChoreography(g.p.cls,v.combatResult);if(h==H::None)return false;
 unsigned p=combatProgress(v.effectProgress,v.effectFrame),ms=v.effectElapsedMs==UINT32_MAX?v.effectFrame*1100/8:v.effectElapsedMs;
 if(p>=1000)return true;
 auto pose=heroPose(g.p.cls,v.combatResult,p,ms);auto hands=dungeonHeroPose(g.p.cls,v.combatResult,p,ms);
 int hx=dungeon?210:55+pose.x,hy=dungeon?145:157+pose.y,tx=targetX>=0?targetX:dungeon?120:182,ty=targetY>=0?targetY:dungeon?86:139;
 int sx=dungeon?hands.originX:103+pose.x,sy=dungeon?hands.originY:144+pose.y;
 int palmX=dungeon?hands.palmX:66+pose.x,palmY=dungeon?hands.palmY:151+pose.y;
 bool hit=v.combatResult.injury();unsigned frame=std::min(11u,p*12/1000);
 auto ball=[&](int x,int y,int size,bool blue=false){const auto* image=campArt::fireball[frame];for(int row=0;row<size;++row)for(int col=0;col<size;++col){auto color=image[(row*40/size)*40+col*40/size];if(color!=0xf81f)c.fillRect(x-size/2+col,y-size/2+row,1,1,blue?(color&0x0400?UI_WHITE:UI_BLUE):color);}};
 auto heal=[&](bool breath){if(p<200||p>=850)return;int rise=int(p-200)*24/650;if(breath){heroFxRing(c,hx,hy-rise/2,9+rise/3,UI_GREEN);for(int i=0;i<3;++i)heroFxLine(c,hx-13+i*11,hy+9-rise,hx-10+i*11,hy+3-rise,UI_WHITE);}else{for(int i=-1;i<=1;++i){int x=hx+i*14,y=hy+5-rise+abs(i)*4;c.fillRect(x-1,y-4,3,9,UI_GREEN);c.fillRect(x-4,y-1,9,3,UI_GREEN);}heroFxRing(c,hx,hy-7,14+rise/5,UI_GOLD);}};
 if(h==H::MagicDarts){
  if(dungeon)magicDartsPilot(c,v,true,hands.originX,hands.originY,tx,ty);
  else {CombatCanvas<Canvas> pilot{c,0,320,0,34};magicDartsPilot(pilot,v,false,-1,-1,targetX,targetY>=0?targetY-34:-1);}
  return true;
 }
 if(h==H::LayHands||h==H::SecondWind){heal(h==H::SecondWind);return true;}
 if(h==H::ArcaneGuard||h==H::ShieldGuard||h==H::SpearGuard||h==H::AxeGuard){
  if(p>=150&&p<850){int q=int(std::min(p-150,150u)),lift=q*5/150;
   if(h==H::ArcaneGuard){heroFxRing(c,hx,hy-12,23+lift,UI_BLUE);heroFxRing(c,hx,hy-12,19+lift,UI_WHITE);}
   else if(h==H::ShieldGuard){int x=dungeon?177:71,y=hy-30-lift;magicSprite(c,sprites_shield[std::min(7u,p*8/1000)%shield_frames],64,72,x-17,y,34,40);}
   else if(h==H::SpearGuard){heroFxLine(c,hx-20,hy+10,hx+23,hy-25,UI_WHITE,2);heroFxLine(c,hx-18,hy+13,hx+25,hy-22,UI_BLUE);}
   else {heroFxLine(c,hx-24,hy-5,hx+23,hy-5,UI_GOLD,3);heroFxLine(c,hx-16,hy-14,hx+16,hy+4,0xfd20,2);if(v.combatResult.recoveredHp)heal(true);}
  }return true;
 }
 if(h==H::ActionSurge){
  if(p>=180&&p<850){int offset=int(p-180)*13/670;for(int n=0;n<2;++n){int x=hx-12+n*15+offset;heroFxLine(c,x-5,hy-13,x+3,hy-5,UI_BLUE,2);heroFxLine(c,x+3,hy-5,x-5,hy+3,UI_WHITE,2);}}return true;
 }
 if(h==H::RageBuff){
  if(p>=180&&p<850){int r=11+int(p-180)*12/670;heroFxRing(c,hx,hy-7,r,0xfd20);for(int n=-1;n<=1;n+=2){heroFxLine(c,hx+n*(r+4),hy+9,hx+n*(r+2),hy-13,UI_GOLD,2);}}return true;
 }
 if(h==H::SacredWeapon){
  if(p>=180&&p<850){int x=dungeon?hands.originX:42,y=dungeon?hands.originY:129;heroFxLine(c,x-8,y-13,x+11,y+13,UI_GOLD,2);heroFxLine(c,x-6,y-13,x+13,y+13,UI_WHITE);heroFxRing(c,x,y,7+int(p-180)*7/670,UI_GOLD);}return true;
 }
 if(h==H::TurnUndead){
  if(p>=180&&p<780){int q=std::min(311,int(p)-180),x=palmX+(tx-palmX)*q/311,y=palmY+(ty-palmY)*q/311;int r=6+q*14/311;heroFxRing(c,x,y,r,UI_GOLD);heroFxLine(c,x,y-r+3,x,y+r-3,UI_WHITE);heroFxLine(c,x-r+3,y-2,x+r-3,y-2,UI_WHITE);}return true;
 }
 if(h==H::FlameCone){
  // A fan leaves the palm; it is not the three thin Scorching Ray tracks.
  if(p>=180&&p<600){int q=std::min(311,int(p)-180);for(int n=1;n<=4;++n){int d=q*n/4,x=palmX+(tx-palmX)*d/311,y=palmY+(ty-palmY)*d/311;int size=6+n*4;for(int ray=-1;ray<=1;++ray)ball(x,y+ray*n*3,size);}}
  if(hit)heroFxContact(c,tx,ty,p,0xfd20);return true;
 }
 if(h==H::ScorchingRays){
  if(p<200){heroFxRing(c,dungeon?sx:30+pose.x,dungeon?sy:122,3+int(p)/55,0xfd20);return true;}
  if(p<600){for(int n=0;n<3;++n){int q=std::max(0,std::min(220,int(p)-200-n*40));if(!q)continue;int ex=sx+(tx-sx)*q/220,ey=sy+(ty+(n-1)*9-sy)*q/220;heroFxLine(c,sx,sy,ex,ey,0xfd20,3);heroFxLine(c,sx,sy,ex,ey,UI_WHITE);}}
  if(hit)heroFxContact(c,tx,ty,p,UI_GOLD);return true;
 }
 if(h==H::StaffBolt||h==H::Fireball||h==H::LegacyLightning){
  if(p<200){int x=dungeon?sx:30+pose.x,y=dungeon?sy:122;heroFxRing(c,x,y,3+int(p)/40,h==H::LegacyLightning?UI_BLUE:0xfd20);}
  else if(p<491){int q=int(p)-200,x=sx+(tx-sx)*q/291,y=sy+(ty-sy)*q/291;
   if(h==H::LegacyLightning){heroFxLine(c,sx,sy,x,(sy+y)/2,UI_BLUE,2);heroFxLine(c,x,(sy+y)/2,x,y,UI_WHITE);}
   else ball(x,y,h==H::Fireball?22:12);
  }else if(hit&&p<780){
   if(h==H::LegacyLightning)magicSprite(c,campArt::lightning[frame],80,112,tx-20,ty-39,40,58);
   else ball(tx,ty,h==H::Fireball?(p<650?44:30):22);
   heroFxContact(c,tx,ty,p,h==H::LegacyLightning?UI_BLUE:UI_GOLD);
  }return true;
 }
 // Physical actions keep their weapon signature; a miss retains the swing only.
 bool spear=h==H::SpearThrust||h==H::PrecisionThrust,axe=h==H::AxeCut||h==H::RageStrike;
 bool divine=h==H::DivineSmite||v.effect==Effect::DivineSlash;
 uint16_t tint=divine?UI_GOLD:axe?0xfd20:UI_WHITE;
 if(p>=200&&p<491){int q=int(p)-200;
  if(spear){int ex=sx+(tx-sx)*q/291,ey=sy+(ty-sy)*q/291;heroFxLine(c,ex-(dungeon?-7:9),ey+2,ex,ey,tint,2);if(h==H::PrecisionThrust)heroFxLine(c,ex-(dungeon?-13:15),ey+4,ex-3,ey+2,UI_BLUE);}
  else {int cx=dungeon?120:166,cy=dungeon?90:137;int radius=axe?34:23;int begin=axe?24:14;for(int k=0;k<begin;++k){int x=cx-radius+(q*radius*2/291)-k,y=cy-radius/2+k*(axe?2:1);c.fillRect(x,y,axe?4:2,2,tint);}if(h==H::SwordPower||h==H::RageStrike)heroFxLine(c,cx-12,cy+8,cx+q*25/291,cy-12,tint,2);}
 }
 if(hit){heroFxContact(c,tx,ty,p,tint);if(divine&&p>=491&&p<650){heroFxLine(c,tx-13,ty,tx+13,ty,UI_GOLD,2);heroFxLine(c,tx,ty-16,tx,ty+16,UI_WHITE,2);}}
 return true;
}
