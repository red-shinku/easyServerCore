#include "WorkT.h"

#include <iostream>
#include <spdlog/spdlog.h>

using namespace easysv;

WorkT::WorkT(std::function<int()> callbackfunc, Task_type& taskt):
taskt(taskt), getfd(callbackfunc), coro_sheduler(taskt.initial_care_event)
{ 
    worker = std::thread([this] { work(); });
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
}

void WorkT::handle_publicq()
{
    int fd = -1;
    while ((fd = getfd()) != -1)
    {
        register_coro(fd);
    }
}

void WorkT::work()
{
    //FIXME: 退出方案
    while (true)
    {
        coro_sheduler.run();
        handle_publicq();
        coro_sheduler.ready_next_run();
    }
}
