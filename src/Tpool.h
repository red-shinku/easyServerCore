#pragma once

#include <queue>
#include <mutex>
#include <vector>
#include "WorkT.h"

namespace easysv
{

class Tpool
{
private:
    //FIXME: 换为循环队列
    std::queue<int> pub_fd_queue;
    std::mutex pub_que_mtx;
    //a template of coroutine
    easysv::Task_type thread_taskt;
    std::vector<easysv::WorkT> wthreads;

    int callback_getfd() noexcept;

public:
    explicit Tpool(int thread_num, easysv::Task_type ttaskt);
    ~Tpool() noexcept;
    Tpool(const Tpool&) = delete;
    Tpool& operator=(const Tpool&) = delete;

    void update_fd_queue(int connfd);
};


} // namespace eaasysv
