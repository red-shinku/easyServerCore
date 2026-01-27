#pragma once

#include <cstddef>

struct Setting
{
    //each time thread get nums of fd from pub_queue
    size_t EACH_FD_GET_NUM;
    //size of pub fd queue
    size_t PUB_FD_QUEUE_SIZE;
    //the nums when pub fd queue turn to busy state 
    size_t PUB_FD_QUEUE_CRITICAL;
    //the nums when thread turn to busy state
    size_t IS_IDLE_NUM;
    //the max nums of fd epoll wait return
    size_t EventArraySize;
    //epoll mod -- LT or ET
    uint32_t EPOLLMOD;
};

extern Setting g_config;
