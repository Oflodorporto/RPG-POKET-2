#pragma once
#include "LegacyEventOffer.h"
#include "TestHero.h"
#include "../firmware/RPG_POKET_2/View.h"
#include "../firmware/RPG_POKET_2/src/GFX/font/glcdfont.h"
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <cstdlib>
// One fixed test workspace mirrors the one-time device PSRAM allocation.
static uint16_t enemyTestWorkspace[enemyArt::WORKSPACE_PIXELS];
struct BindEnemyWorkspace {BindEnemyWorkspace(){enemyArt::workspace=enemyTestWorkspace;}} bindEnemyWorkspace;
struct Canvas {
  uint16_t pixels[240*320]{};int tx=0,ty=0,size=1;uint16_t color=UI_WHITE;
  void pixel(int x,int y,uint16_t c){if(x>=0&&x<240&&y>=0&&y<320)pixels[y*240+x]=c;}
  void fillRect(int x,int y,int w,int h,uint16_t c){for(int j=0;j<h;++j)for(int i=0;i<w;++i)pixel(x+i,y+j,c);}
  void drawRect(int x,int y,int w,int h,uint16_t c){fillRect(x,y,w,1,c);fillRect(x,y+h-1,w,1,c);fillRect(x,y,1,h,c);fillRect(x+w-1,y,1,h,c);}
  void fillScreen(uint16_t c){fillRect(0,0,240,320,c);}
  void setTextWrap(bool){}
  void setTextColor(uint16_t c){color=c;}
  void setTextSize(int s){size=s;}
  void setCursor(int x,int y){tx=x;ty=y;}
  void print(const char* s){
    if(!(tx>=0&&ty>=0&&tx+int(strlen(s))*6*size<=240&&ty+8*size<=320))fprintf(stderr,"Text overflow: x=%d y=%d size=%d text=%s\n",tx,ty,size,s);
    assert(tx>=0&&ty>=0&&tx+int(strlen(s))*6*size<=240&&ty+8*size<=320);
    while(*s){unsigned ch=(unsigned char)*s++;for(int i=0;i<5;++i)for(int j=0;j<8;++j)if(font[ch*5+i]&(1<<j))fillRect(tx+i*size,ty+j*size,size,size,color);tx+=6*size;}
  }
  void draw16bitRGBBitmap(int x,int y,uint16_t* b,int w,int h){for(int j=0;j<h;++j)for(int i=0;i<w;++i)pixel(x+i,y+j,b[j*w+i]);}
  void save(const std::string& path){std::ofstream f(path,std::ios::binary);f<<"P6\n240 320\n255\n";for(auto p:pixels){char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};f.write(rgb,3);}}
};

