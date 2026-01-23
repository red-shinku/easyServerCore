#include "WorkT.h"

#include <iostream>
#include <spdlog/spdlog.h>

using namespace easysv;

WorkT::WorkT(std::function<std::vector<int>()> cback_getfd, std::function<void()> cback_sidle, 
                Task_type& taskt, int id, int efd):
task_num(0), taskt(taskt), getfd(cback_getfd), say_idle(cback_sidle), 
coro_sheduler(taskt.initial_care_event, task_num, notify_fd), 
id(id), notify_fd(efd)
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
    //FIXME: 线程如何安全退出
    while (true)
    {
        coro_sheduler.ready_next_run();
        handle_publicq();
        coro_sheduler.run();

        if(task_num <= IS_IDLE_NUM && is_idle_now == false) 
        {
            is_idle_now == true;
            say_idle();
        }
        else
            is_idle_now == false;
    }
}
