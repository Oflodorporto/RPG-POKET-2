#include "windows_compat.h"
#include <cassert>
#include <cstdio>
int main(){time_t epoch=0;tm out{};assert(gmtime_r(&epoch,&out)==&out);assert(out.tm_year==70&&out.tm_mon==0&&out.tm_mday==1&&out.tm_hour==0);std::puts("PASS: native thread-safe UTC compatibility at Unix epoch");}
