#pragma once
#include "Dungeon.h"
#include "DungeonArt.h"
#include <math.h>
template<class Canvas> void drawDungeon(Canvas& c,const rpg::Game& g,const ViewState& v){
  constexpr int horizon=86,height=172;float depths[120];unsigned floor=rpg::dungeonFloor(g),heading=rpg::dungeonHeading(g);
  float px=rpg::dungeonX(g)+.5f,py=rpg::dungeonY(g)+.5f,dx=rpg::dungeonDx[heading],dy=rpg::dungeonDy[heading],planeX=-dy*.66f,planeY=dx*.66f;
  auto shade=[](uint16_t color,bool dim){return dim?uint16_t((color>>1)&0x7bef):color;};
  // Half-resolution raycasting writes directly into the existing PSRAM framebuffer.
  for(int y=0;y<height;y+=2){float distance=86.f/std::max(1,abs(y-horizon));for(int x=0;x<240;x+=2){float camera=2.f*x/240-1;float wx=px+distance*(dx+planeX*camera),wy=py+distance*(dy+planeY*camera);int tx=int(floorf(wx*32))%32,ty=int(floorf(wy*32))%32;if(tx<0)tx+=32;if(ty<0)ty+=32;c.fillRect(x,y,2,2,shade(dungeonArt::textures[floor][y<horizon?2:1][ty*32+tx],true));}}
  for(int ray=0;ray<120;++ray){float camera=2.f*(ray+.5f)/120-1,rx=dx+planeX*camera,ry=dy+planeY*camera;int mx=int(px),my=int(py);float deltaX=rx==0?1e6f:fabsf(1/rx),deltaY=ry==0?1e6f:fabsf(1/ry);int sx=rx<0?-1:1,sy=ry<0?-1:1;float sideX=(rx<0?px-mx:mx+1-px)*deltaX,sideY=(ry<0?py-my:my+1-py)*deltaY;bool side=false;
    for(int n=0;n<24;++n){if(sideX<sideY){sideX+=deltaX;mx+=sx;side=false;}else{sideY+=deltaY;my+=sy;side=true;}if(rpg::dungeonWall(g,mx,my))break;}
    float distance=std::max(.08f,side?sideY-deltaY:sideX-deltaX);depths[ray]=distance;int wallHeight=int(height/distance),top=horizon-wallHeight/2,bottom=horizon+wallHeight/2;
    float hit=side?px+distance*rx:py+distance*ry;int tx=int((hit-floorf(hit))*32)&31;const uint16_t* texture=dungeonArt::textures[floor][rpg::dungeonCell(floor,mx,my)=='D'?3:0];
    for(int y=std::max(0,top);y<std::min(height,bottom);y+=2){int ty=std::min(31,(y-top)*32/std::max(1,wallHeight));c.fillRect(ray*2,y,2,std::min(2,height-y),shade(texture[ty*32+tx],side));}
  }
  auto billboard=[&](const uint16_t* img,int iw,int ih,float wx,float wy,float scale){float vx=wx-px,vy=wy-py,depth=vx*dx+vy*dy;if(depth<.1f)return;float lateral=(-dy*vx+dx*vy)/.66f;int center=int(120*(1+lateral/depth)),h=std::min(250,int(height*scale/depth)),w=h*iw/ih,base=int(horizon+86/depth),left=center-w/2,top=base-h;if(w<1||h<1)return;
    for(int x=std::max(0,left);x<std::min(240,left+w);x+=2){if(depth>depths[x/2]+.05f)continue;int ix=(x-left)*iw/w;for(int y=std::max(0,top);y<std::min(height,base);y+=2){int iy=(y-top)*ih/h;uint16_t color=img[iy*iw+ix];if(color!=0xf81f)c.fillRect(x,y,std::min(2,240-x),std::min(2,height-y),color);}}
  };
  // Far-to-near billboards, with wall depth clipping; all art stays in flash/PSRAM.
  struct Visible{float distance;int kind,id,x,y;};Visible objects[20];unsigned count=0;
  auto add=[&](int kind,int id,int x,int y){float vx=x+.5f-px,vy=y+.5f-py;objects[count++]={vx*vx+vy*vy,kind,id,x,y};};
  for(unsigned i=0;i<7;++i){auto s=rpg::dungeonSpawns[i];if(s.floor==floor&&!(g.dungeonEnemies&(1u<<i)))add(0,i,s.x,s.y);}
  for(unsigned i=0;i<8;++i){auto s=rpg::dungeonPickups[i];if(s.floor==floor&&!(g.dungeonLoot&(1u<<i)))add(1,i,s.x,s.y);}
  for(int y=1;y<8;++y)for(int x=1;x<8;++x)if(rpg::dungeonCell(floor,x,y)=='S')add(2,floor?9:10,x,y);
  std::sort(objects,objects+count,[](const Visible&a,const Visible&b){return a.distance>b.distance;});
  for(unsigned i=0;i<count;++i){auto o=objects[i];if(o.kind==0){auto s=rpg::dungeonSpawns[o.id];unsigned type=s.id==2?0:s.id==5?1:s.id==9?2:3,pose=0;if(o.id==rpg::dungeonEnemyAhead(g)){if(v.effect!=Effect::None)pose=v.effectOnHero?(v.effectFrame<3?1:2):3;else if(g.phase==rpg::Phase::Enemy)pose=1;}billboard(dungeonArt::enemies[type][pose],48,64,o.x+.5f,o.y+.5f,.8f);}
    else {unsigned prop=o.kind==2?o.id:rpg::dungeonPickups[o.id].id;billboard(dungeonArt::props[prop],32,32,o.x+.5f,o.y+.5f,.32f);if(o.distance<.1f){const auto* image=dungeonArt::props[prop];for(int y=0;y<32;++y)for(int x=0;x<32;++x)if(image[y*32+x]!=0xf81f)c.fillRect(104+x,130+y,1,1,image[y*32+x]);}}
  }
  const auto* weapon=dungeonArt::weapons[g.p.cls][v.effect!=Effect::None&&!v.effectOnHero];for(int y=0;y<52;++y)for(int x=0;x<64;++x){uint16_t color=weapon[y*64+x];if(color!=0xf81f)c.fillRect(144+x,120+y,1,1,color);}
  if(v.effect==Effect::Lightning){for(int y=30;y<130;y+=6)c.fillRect(118+(y/6%2?5:-5),y,5,7,0xaffe);}
  if(v.effect==Effect::Slash&&v.effectOnHero)c.drawRect(0,0,240,172,UI_RED);
  if(v.effect==Effect::Shield){for(int k=0;k<3;++k)c.drawRect(k*3,k*3,240-k*6,172-k*6,UI_BLUE);}
  if(v.effect==Effect::Rage){for(int k=0;k<3;++k)c.drawRect(k*2,k*2,240-k*4,172-k*4,(v.effectFrame&1)?0xfd20:UI_GOLD);}
  c.fillRect(0,172,240,148,UI_INK);auto text=[&](int x,int y,const char* s,uint16_t color=UI_WHITE){c.setTextSize(1);c.setTextColor(color);c.setCursor(x,y);c.print(s);};char b[48];
  snprintf(b,sizeof(b),"HP %u/%u MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);text(4,176,b);static const char* directions[]={"N","L","S","O"};snprintf(b,sizeof(b),"ANDAR %u / %s / SELO %s",floor+1,directions[heading],g.dungeonLoot&8?"SIM":"NAO");text(4,186,b,UI_GOLD);
  const char* status=g.phase==rpg::Phase::Won?"Vitoria! Toque em Continuar":g.phase==rpg::Phase::Lost?"Derrota! Retorne as Ruinas":g.phase==rpg::Phase::Fled?"Recuou do inimigo":v.message;char shortText[39];snprintf(shortText,sizeof(shortText),"%.38s",status);text(4,196,shortText,UI_GREEN);
  auto button=[&](int x,int y,int w,int h,const char* label){c.fillRect(x,y,w,h,UI_PANEL);c.drawRect(x,y,w,h,UI_GOLD);if(h>30){int cx=x+w/2,cy=y+12;bool up=!strcmp(label,"Avancar"),down=!strcmp(label,"Recuar"),left=strchr(label,'<');if(up||down){c.fillRect(cx-2,cy-4,4,16,UI_GOLD);for(int k=0;k<8;++k)c.fillRect(cx-k,cy+(up?-7+k:10-k),2*k+1,1,UI_GOLD);}else{c.fillRect(cx-10,cy-2,21,4,UI_GOLD);for(int k=0;k<8;++k)c.fillRect(cx+(left?-12+k:12-k),cy-k,1,2*k+1,UI_GOLD);}text(x+(w-int(strlen(label))*6)/2,y+28,label);}else text(x+(w-int(strlen(label))*6)/2,y+(h-8)/2,label);};
  if(g.phase==rpg::Phase::Hero||g.phase==rpg::Phase::Enemy){snprintf(b,sizeof(b),"%s / HP %u",rpg::enemySpec(g.enemyId).name,g.enemyHp);c.fillRect(4,4,204,14,UI_INK);text(8,7,b,UI_GOLD);}
  button(4,208,74,39,"Girar <");button(82,208,74,39,"Avancar");button(160,208,76,39,"Girar >");button(4,252,74,39,"Lado <");button(82,252,74,39,"Recuar");button(160,252,76,39,"Lado >");
  button(4,296,152,22,g.phase==rpg::Phase::Hero?"ATACAR":g.phase==rpg::Phase::Home?"ACAO / COLETAR":g.phase==rpg::Phase::Enemy?"TURNO INIMIGO":"CONTINUAR");button(160,296,76,22,"Menu");
}
