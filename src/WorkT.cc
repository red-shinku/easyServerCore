#include "WorkT.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include "../include/config.h"

using namespace easysv;

WorkT::WorkT(std::function<std::vector<int>()> cback_getfd, 
                std::function<void()> cback_sidle, Task_type& taskt, 
                int id, int efd, std::atomic<bool>& stopping_flag) noexcept:
notify_fd(efd), task_num(0), taskt(taskt), getfd(cback_getfd), 
coro_sheduler(taskt.initial_care_event, task_num, notify_fd), 
say_idle(cback_sidle), id(id), stopping(stopping_flag)
{ 
    worker = std::thread([this] { work(); });
    coro_sheduler.register_notify_fd(notify_fd);
    spdlog::info("create a thread");
}

WorkT::~WorkT()
{
    if(worker.joinable())
    {
        worker.join();
        spdlog::info("delete a thread");
    }
    else
        spdlog::warn("failed to delete thread");
}

void WorkT::register_coro(int fd)
{
    coro_sheduler.register_coro(
        fd, 
        taskt.task_template
    );
    ++task_num;
}

void WorkT::handle_publicq()
{
    int fd = -1;
    auto fds = getfd();
    for(auto& fd: fds)
    {
        register_coro(fd);
    }
}

void WorkT::work()
{
    while (! stopping.load(std::memory_order_relaxed))
    {
        coro_sheduler.ready_next_run();
        handle_publicq();
        coro_sheduler.run();

        if(task_num <= g_config.IS_IDLE_NUM && is_idle_now == false) 
        { //FIXME: 修改空闲定义
            is_idle_now = true;
            say_idle();
        }
        else
            is_idle_now = false;
    }
}
