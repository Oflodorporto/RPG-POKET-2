#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
struct StaticQueue_t {uint8_t* bytes=nullptr;unsigned capacity=0,itemSize=0,head=0,count=0;};
using QueueHandle_t=StaticQueue_t*;
inline QueueHandle_t xQueueCreateStatic(unsigned capacity,unsigned itemSize,uint8_t* bytes,StaticQueue_t* queue){*queue={bytes,capacity,itemSize,0,0};return queue;}
inline int xQueueSend(QueueHandle_t q,const void* entry,unsigned){if(q->count==q->capacity)return pdFALSE;std::memcpy(q->bytes+((q->head+q->count)%q->capacity)*q->itemSize,entry,q->itemSize);++q->count;return pdTRUE;}
inline int xQueueReceive(QueueHandle_t q,void* entry,unsigned){if(!q->count)return pdFALSE;std::memcpy(entry,q->bytes+q->head*q->itemSize,q->itemSize);q->head=(q->head+1)%q->capacity;--q->count;return pdTRUE;}
