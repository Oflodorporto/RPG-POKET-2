#pragma once
#include <pthread.h>
#include <cassert>
#include <atomic>
constexpr int pdTRUE=1,pdFALSE=0;
// Host-only SMP lock stand-in. Static storage, no C++ heap allocation. The
// optional post-unlock hook lets two real threads force callback/close races.
struct portMUX_TYPE {pthread_mutex_t mutex;};
#define portMUX_INITIALIZER_UNLOCKED {PTHREAD_MUTEX_INITIALIZER}
inline thread_local unsigned fakeCriticalDepth=0;
inline std::atomic<unsigned> fakeCriticalEntries{0},fakeCriticalExits{0};
inline void (*fakeAfterCritical)()=nullptr;
inline void portENTER_CRITICAL(portMUX_TYPE* mux){assert(!fakeCriticalDepth);assert(!pthread_mutex_lock(&mux->mutex));++fakeCriticalDepth;++fakeCriticalEntries;}
inline void portEXIT_CRITICAL(portMUX_TYPE* mux){assert(fakeCriticalDepth==1);--fakeCriticalDepth;++fakeCriticalExits;assert(!pthread_mutex_unlock(&mux->mutex));if(fakeAfterCritical)fakeAfterCritical();}
