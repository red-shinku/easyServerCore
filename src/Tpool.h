#pragma once

#include <atomic>
#include <vector>
#include <memory>
#include "WorkT.h"

namespace easysv
{

class Tpool
{
private:
    //a template of coroutine
    easysv::Task_type thread_taskt;
    //threads
    std::vector<std::unique_ptr<easysv::WorkT>> wthreads; 
    //save idle thread's id(the index of wthreads)
    // easysv::queue<int> idle_threads_id;

    std::atomic<bool> stopping;

public:
    explicit Tpool(int thread_num, 
                    easysv::Task_type ttaskt, 
                    struct sockaddr_in* servaddr);
    ~Tpool() noexcept;
    Tpool(const Tpool&) = delete;
    Tpool& operator=(const Tpool&) = delete;

    //wake up and close all thread
    void shutdown();
};


} // namespace eaasysv
