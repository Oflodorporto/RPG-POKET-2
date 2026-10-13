#pragma once
#include <cassert>
#include <vector>
constexpr int OUTPUT=1,HIGH=1;inline std::vector<int> sequence;inline bool sdHigh=false,lcdHigh=false;
inline void pinMode(int p,int m){assert((p==41||p==45)&&m==OUTPUT);sequence.push_back(p);}
inline void digitalWrite(int p,int v){assert((p==41||p==45)&&v==HIGH);(p==41?sdHigh:lcdHigh)=true;sequence.push_back(100+p);}
