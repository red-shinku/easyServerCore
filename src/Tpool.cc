#include "Tpool.h"

#include <iostream>
#include <stdexcept>
#include <spdlog/spdlog.h>

using namespace easysv;

Tpool::Tpool(int thread_num, Task_type ttaskt):
thread_taskt(ttaskt)
{ 
    wthreads.reserve(thread_num);
    for (int i = 0; i < thread_num; ++i)
    {
        wthreads.emplace_back(
            [this]{ return callback_getfd(); },
            thread_taskt 
        );
    }
    spdlog::info("create thread poll!");  
}

int Tpool::callback_getfd()
{
    std::unique_lock<std::mutex> lock(pub_que_mtx);
    if( ! pub_fd_queue.empty())
    {
        int fd = pub_fd_queue.front();
        pub_fd_queue.pop();
        return fd;
    }
    return -1;
}

void Tpool::update_fd_queue(int connfd)
{
    std::unique_lock<std::mutex> lock(pub_que_mtx);
    pub_fd_queue.push(connfd);
}
