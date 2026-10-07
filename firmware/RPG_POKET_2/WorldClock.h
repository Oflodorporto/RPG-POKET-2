#pragma once
#include <stdint.h>
#include <time.h>
namespace worldClock {
enum class Period:uint8_t { Dawn,Day,Dusk,Night };
inline Period period(unsigned h){return h<5||h>=20?Period::Night:h<7?Period::Dawn:h<18?Period::Day:Period::Dusk;}
inline const char* name(Period p){return p==Period::Dawn?"Amanhecer":p==Period::Day?"Dia":p==Period::Dusk?"Crepusculo":"Noite";}
inline bool leap(int y){return y%4==0&&(y%100!=0||y%400==0);}
inline unsigned days(int y,unsigned m){static const unsigned d[]={31,28,31,30,31,30,31,31,30,31,30,31};return m>=1&&m<=12?d[m-1]+(m==2&&leap(y)):0;}
inline bool civil(int y,unsigned m,unsigned d,unsigned h,unsigned min){return y>=2026&&y<=2099&&m>=1&&m<=12&&d>=1&&d<=days(y,m)&&h<24&&min<60;}
// Calendar conversion independent of the host timezone and daylight saving rules.
inline int64_t epoch(int y,unsigned m,unsigned d,unsigned h,unsigned min,int offset){if(!civil(y,m,d,h,min)||offset<-12||offset>14)return 0;int64_t n=0;for(int a=1970;a<y;++a)n+=365+leap(a);for(unsigned a=1;a<m;++a)n+=days(y,a);return (n+d-1)*86400+int64_t(h)*3600+min*60-offset*3600;}
struct Clock {bool manual=false;int64_t stamp=0;uint32_t tick=0;
 void set(int64_t s,uint32_t now){manual=true;stamp=s;tick=now;}
 int64_t read(int64_t automatic,uint32_t now){if(!manual)return automatic;uint32_t seconds=uint32_t(now-tick)/1000;stamp+=seconds;tick+=seconds*1000;return stamp;}
 bool valid(int64_t s)const{return s>=1767225600LL&&s<4102444800LL;}
};
inline uint16_t shade(uint16_t c,Period p){unsigned r=(c>>11)&31,g=(c>>5)&63,b=c&31;if(p==Period::Day)return c;
 if(p==Period::Night){r=r*48/100;g=g*55/100;b=b*72/100;}else if(p==Period::Dusk){r=r*90/100;g=g*68/100;b=b*56/100;}else {r=r*95/100;g=g*83/100;b=b*68/100;}return uint16_t(r<<11|g<<5|b);}
}
