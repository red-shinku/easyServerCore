#include "Tpool.h"

#include <sys/eventfd.h>
#include <iostream>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include "../include/config.h"

using namespace easysv;

Tpool::Tpool(int thread_num, Task_type ttaskt):
pub_fd_queue(g_config.PUB_FD_QUEUE_SIZE), idle_threads_id(thread_num), 
thread_taskt(ttaskt), stopping(false)
{ 
    wthreads.reserve(thread_num);
    for (int id = 0; id < thread_num; ++id)
    {
        wthreads.emplace_back(std::make_unique<easysv::WorkT>(
            [this] { return callback_getfds(); }, 
            [this, id] { return callback_say_idle(id); }, 
            thread_taskt,
            id,
            eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC), //设置非阻塞
            stopping
        ));
        idle_threads_id.push(id);
    }
    spdlog::info("create thread pool!");  
}

Tpool::~Tpool()
{
    shutdown();
}

std::vector<int> Tpool::callback_getfds()
{
    std::unique_lock<std::mutex> lock(pub_que_mtx);
    //每次取 EACH_FD_GET_NUM 个，提高锁效率
    std::vector<int> fds;
    for(int i = 0; i < g_config.EACH_FD_GET_NUM && !pub_fd_queue.empty(); ++i)
    {
        int fd = pub_fd_queue.front();
        pub_fd_queue.pop();
        fds.push_back(fd);
    }
    return std::move(fds);
}

void Tpool::callback_say_idle(int id)
{
    std::unique_lock<std::mutex> lock(idle_que_mtx);
    idle_threads_id.push(id);
}

void Tpool::shutdown()
{
    stopping.store(true, std::memory_order_relaxed);

    for(auto& t : wthreads) {
        uint64_t one = 1;
        write(t->notify_fd, &one, sizeof(one));
    }
}

void Tpool::accept_and_notice_thread(int connfd)
{
    //一整个操作的原子性质
    std::unique_lock<std::mutex> lock_pub_q(pub_que_mtx);
    std::unique_lock<std::mutex> lock_idle_q(idle_que_mtx);

    // while(pub_fd_queue.size() == PUB_FD_QUEUE_SIZE - 1 && idle_threads_id.empty())
    // {
        // idle_que_cv.wait(lock_pub_q);
    // }
    pub_fd_queue.push(connfd);

    if(! idle_threads_id.empty())
    {
        auto idle_id = idle_threads_id.front();
        idle_threads_id.pop();
        // lock_idle_q.unlock();
        //notice the thread
        uint64_t one = 1;
        write(wthreads[idle_id]->notify_fd, &one, sizeof(one));
    }
    else if(pub_fd_queue.size() < g_config.PUB_FD_QUEUE_CRITICAL)
    {
        return; //无人有空且新连接堆积较少
    }
    else
    {
        //方案B: 没人有空且新连接堆积较多，轮转处理，该方案与条件变量方案冲突
        static int next = 0;
        uint64_t one = 1;
        write(wthreads[next++ % wthreads.size()]->notify_fd, &one, sizeof(one));
    }
}
