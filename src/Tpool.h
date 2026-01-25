#pragma once

#include <mutex>
#include <atomic>
#include <vector>
#include <condition_variable>
#include "cqueue.hpp"
#include "WorkT.h"

#define EACH_FD_GET_NUM 5
#define PUB_FD_QUEUE_SIZE 300 
#define PUB_FD_QUEUE_CRITICAL 10

namespace easysv
{

class Tpool
{
private:
    easysv::queue<int> pub_fd_queue;
    //a template of coroutine
    easysv::Task_type thread_taskt;
    //threads
    std::vector<easysv::WorkT> wthreads;
    //save idle thread's id(the index of wthreads)
    easysv::queue<int> idle_threads_id;

    std::mutex pub_que_mtx;
    std::mutex idle_que_mtx;
    // std::condition_variable idle_que_cv;

    std::vector<int> callback_getfds() noexcept;
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
