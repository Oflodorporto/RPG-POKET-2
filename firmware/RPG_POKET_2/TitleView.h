#pragma once
#include "TitleArt.h"
#include "TitleLogo.h"
// Static indexed art in flash. Animation is deterministic, read-only and SD-free.
template<class Canvas> void drawTitle(Canvas& c,unsigned frame){
 uint16_t row[240];for(int y=0;y<320;++y){for(int x=0;x<240;++x)row[x]=titleArt::palette[titleArt::pixels[y*240+x]];c.draw16bitRGBBitmap(0,y,row,240,1);}
 c.setTextWrap(false);
 auto label=[&](int x,int y,const char* s,int scale,uint16_t color){c.setTextSize(scale);c.setTextColor(color);c.setCursor(x,y);c.print(s);};
 auto center=[&](int y,const char* s,int scale,uint16_t color){label((240-int(strlen(s))*6*scale)/2,y,s,scale,color);};
 // A few moving leaf highlights suggest a breeze without shaking the whole screen.
 for(unsigned i=0;i<4;++i){int x=15+int(i*14)+sway,y=13+int(i*9);c.fillRect(x,y,3,1,0x52c5);}
 unsigned wind=frame%100;c.fillRect(8+int(wind/3),154+int(wind/12),2,1,0xa4a6);
 // Transparent beveled wordmark recreated from the user's reference.
 for(int y=0;y<112;++y){int x=0;while(x<190){while(x<190&&!titleLogo::pixels[y*190+x])++x;int begin=x;while(x<190&&titleLogo::pixels[y*190+x]){row[x]=titleLogo::palette[titleLogo::pixels[y*190+x]];++x;}if(x>begin)c.draw16bitRGBBitmap(25+begin,y,row+begin,x-begin,1);}}
 center(116,"AS CINZAS DA PRIMEIRA AURORA",1,UI_WHITE);
 center(133,"Valdaria / Aeldra",1,UI_GOLD);
 // A distant hippogriff travels slowly near the setting sun, with four wing poses.
 unsigned cycle=frame%320;int bx=207-int(cycle*54/320),by=139+int((cycle/40)%3)-1;
 const uint16_t* bird=hippogriffArt::frames[(frame/6)%4];
 for(int y=0;y<17;++y)for(int x=0;x<16;++x){uint16_t color=bird[(y*86/17)*80+x*80/16];if(color!=SPRITE_KEY)c.fillRect(bx+x,by+y,1,1,color);}
 // Slow fire tips and rising embers follow the illustration's hearth.
 unsigned breathe=(frame/3)%8;int sway=breathe<4?int(breathe)-2:5-int(breathe);
 c.fillRect(52+sway,252-int(breathe%3),2,9,0xfba0);c.fillRect(54,258,3,5,0xffab);
 for(unsigned i=0;i<3;++i){unsigned age=(frame/3+i*11)%28;c.fillRect(50+int(i*4)+int(age/9),255-int(age),1,age<15?2:1,age<14?0xfdc4:0xb240);}
 auto button=[&](int y,const char* s,bool enabled,bool selected){
  c.fillRect(82,y,150,40,0x0842);c.fillRect(85,y+3,144,34,enabled?0x1926:0x2104);
  c.drawRect(82,y,150,40,enabled?UI_GOLD:0x528a);c.drawRect(84,y+2,146,36,enabled?0x8c51:0x3186);
  c.fillRect(87,y+4,140,1,enabled?0x6b6c:0x3186);int scale=strlen(s)*12<144?2:1;
  label(82+(150-int(strlen(s))*6*scale)/2,y+(40-8*scale)/2,s,scale,enabled?UI_WHITE:0x630c);
  if(selected){for(int i=0;i<6;++i)c.fillRect(69+i,y+14+i,1,12-2*i,UI_GOLD);}
 };
 button(170,"Continuar",menu.hasContinue,menu.hasContinue);
 button(216,"Novo jogo",true,!menu.hasContinue);
 button(262,"Configuracoes",true,false);
 if(*menu.notice){c.fillRect(2,150,236,14,UI_INK);center(153,menu.notice,1,UI_RED);}
 center(309,"RPG POKET 2 / titulo1",1,UI_MUTED);
}
