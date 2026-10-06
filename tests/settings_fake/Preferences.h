#pragma once
#include "WiFi.h"
#include <map>
struct Preferences {
 std::map<std::string,std::string> strings;std::map<std::string,int> values;std::string failKey;
 bool begin(const char*,bool){return true;}
 unsigned putUChar(const char* k,unsigned char v){if(failKey==k)return 0;values[k]=v;return 1;}
 unsigned char getUChar(const char* k,unsigned char d){return values.count(k)?values[k]:d;}
 unsigned putBool(const char* k,bool v){return putUChar(k,v);}
 bool getBool(const char* k,bool d){return getUChar(k,d);}
 size_t putString(const char* k,const char* v){if(failKey==k)return 0;strings[k]=v;return strlen(v);}
 String getString(const char* k,const char* d){return String(strings.count(k)?strings[k]:d);}
};
