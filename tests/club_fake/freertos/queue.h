#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>
struct StaticQueue_t {unsigned capacity=0,itemSize=0;std::deque<std::vector<uint8_t>> items;};
using QueueHandle_t=StaticQueue_t*;
inline QueueHandle_t xQueueCreateStatic(unsigned capacity,unsigned itemSize,uint8_t*,StaticQueue_t* queue){queue->capacity=capacity;queue->itemSize=itemSize;queue->items.clear();return queue;}
inline int xQueueSend(QueueHandle_t queue,const void* entry,unsigned){if(queue->items.size()==queue->capacity)return pdFALSE;auto first=static_cast<const uint8_t*>(entry);queue->items.emplace_back(first,first+queue->itemSize);return pdTRUE;}
inline int xQueueReceive(QueueHandle_t queue,void* entry,unsigned){if(queue->items.empty())return pdFALSE;memcpy(entry,queue->items.front().data(),queue->itemSize);queue->items.pop_front();return pdTRUE;}
