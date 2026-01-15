#include "WorkT.h"

#include <iostream>
#include <spdlog/spdlog.h>

using namespace easysv;

WorkT::WorkT(std::function<int()> callbackfunc, Task_type& taskt):
taskt(taskt), getfd(callbackfunc), worker([this]{ work(); })
{ 
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

void WorkT::register_epoll(int fd)
{
    try
    {
        epoll.addfd(fd, taskt.initial_care_event);
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void WorkT::register_coro(int fd)
{
    auto coro = new coro_t();
    *coro = taskt.task_template(fd);
    coro_sheduler.addcoro(
        fd, 
        taskt.initial_care_event, 
        coro);
}

void WorkT::handle_publicq()
{
    int fd = -1;
    while ((fd = getfd()) != -1)
    {
        register_coro(fd);
    }
}

void WorkT::del_fd_in_epoll(int fd)
{
    try
    {
        epoll.deletefd(fd);
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n'; 
    }        
}

void WorkT::work()
{
    while (true)
    {
        coro_sheduler.run();
        handle_publicq();
        //wait epoll event
        auto [fdlist, readynum] = epoll.wait(coro_sheduler.get_fdlist());
        coro_sheduler.ready(std::move(fdlist), readynum);
    }
}
