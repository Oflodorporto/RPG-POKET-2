#pragma once
#include <time.h>
#ifdef _WIN32
// Host-only POSIX spelling for the CRT's thread-safe UTC conversion.
inline tm* gmtime_r(const time_t* value,tm* out){return gmtime_s(out,value)==0?out:nullptr;}
#endif
