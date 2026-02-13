#pragma once

#include <cstddef>

struct Setting
{
    //the size of listen sock queue
    int LISTENQ;
    //each time thread get nums of fd when accept
    size_t EACH_ACCEPT_NUM;
    //the max nums of fd epoll wait return
    size_t EventArraySize;
    //epoll mod -- LT or ET
    uint32_t EPOLLMOD;
};

extern Setting g_config;
