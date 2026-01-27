#pragma once

#include <mutex>
#include <atomic>
#include <vector>
#include <memory>
#include "cqueue.hpp"
#include "WorkT.h"

namespace easysv
{

class Tpool
{
private:
    easysv::queue<int> pub_fd_queue;
    //a template of coroutine
    easysv::Task_type thread_taskt;
    //threads
    std::vector<std::unique_ptr<easysv::WorkT>> wthreads; 
    //save idle thread's id(the index of wthreads)
    easysv::queue<int> idle_threads_id;

    std::mutex pub_que_mtx;
    std::mutex idle_que_mtx;
    // std::condition_variable idle_que_cv;

    std::vector<int> callback_getfds();
    void callback_say_idle(int id);

    std::atomic<bool> stopping;

    //wake up and close all thread
    void shutdown();

public:
    explicit Tpool(int thread_num, easysv::Task_type ttaskt);
    ~Tpool() noexcept;
    Tpool(const Tpool&) = delete;
    Tpool& operator=(const Tpool&) = delete;

    void accept_and_notice_thread(int connfd);
};


} // namespace eaasysv
