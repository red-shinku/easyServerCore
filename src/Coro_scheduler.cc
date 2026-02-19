#include "../include/Coro_scheduler.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include "../include/config.h"
#include "Epoll.h"

using namespace easysv;

Coro_scheduler::Coro_scheduler(EPOLL_EVENTS initial_care_event, 
                            int listen_fd,
                            std::function<void(int, EPOLL_EVENTS, uint32_t)> ep_reg, 
                            std::function<void(int, EPOLL_EVENTS, uint32_t)> ep_ctl, 
                            std::function<void(int)> ep_del):
initial_care_event(initial_care_event), coros{}, 
ending_queue(), listen_fd(listen_fd), 
register_fd(ep_reg), change_fd_event(ep_ctl), unregister_fd(ep_del)
{ }

Coro_scheduler::~Coro_scheduler()
{
    //destroy all corotinues
    for(auto &coro: coros)
    {
        coro.second.coro_handle.destroy();
    }
}

void Coro_scheduler::wait_event(int fd, handle_t coro_handle, EPOLL_EVENTS state)
{
    //如果状态变化修改epoll
    try
    {
        if(coros.at(fd).state != state)
        {
            change_fd_event(fd, state, 0);
            coros.at(fd).state = state;
        }
    }
    catch(const std::out_of_range& e)
    {
        spdlog::error("Coro_scheduler::wait_read: fd not found in coros map");
        std::cerr << e.what() << '\n';
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Coro_scheduler::__register_coro__(int fd, handle_t coro_handle)
{
    try
    {
        if(coros.find(fd) == coros.end())
        {
            FdDetail fddetail{coro_handle, initial_care_event};
            coros.emplace(fd, std::move(fddetail));
            register_fd(fd, initial_care_event, g_config.EPOLLMOD);
        }
        else
            spdlog::warn("Coro_scheduler::register_wait_read: The FD {} has been register", fd);
    }
    catch(const std::bad_alloc& e)
    {
        spdlog::error("Coro_scheduler::register_wait_read: Register failed");
        std::cerr << e.what() << '\n';
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Coro_scheduler::unregister_coro(int connfd, handle_t handle)
{
    ending_queue.push_back(handle);
    if(coros.find(connfd) != coros.end())
    {
        try
        {
            unregister_fd(connfd);
            coros.erase(connfd);
            close(connfd);
            spdlog::info("finish sock {}", connfd);
        }
        catch(const std::system_error& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    else
    {
        spdlog::warn("Delete an unexisting Fd in coros map");
    }
}

void Coro_scheduler::clean_coro()
{
    for(auto& handle: ending_queue)
    {
        handle.destroy();
    }
    ending_queue.clear();
}

void Coro_scheduler::run(fdarray_t* readylist, int readynum)
{
    for(int i = 0; i < readynum; ++i) {
        int fd = (*readylist)[i].first;
        auto it = coros.find(fd);
        if (it == coros.end()) continue; //has been unregister or other fd

        auto &fddetail = it->second;
        if( (*readylist)[i].second & (fddetail.state | EPOLLHUP | EPOLLRDHUP | EPOLLERR) ) {
            fddetail.coro_handle.resume();
        }
    }
    clean_coro();
}

void Coro_scheduler::register_coro(int connfd, callable_coro_t coro)
{
    try
    {
        if(coros.find(connfd) == coros.end())
        {   //first run a coro and register
            coro(
                [this]() -> Coro_scheduler& { return *this; } (),
                connfd
            ); 
        }
        else
        {
            spdlog::warn("Register an existing Fd in coros map");
        }   
    }
    catch(const std::bad_alloc& e)
    {
        spdlog::error("out of memory: coros map");
        std::cerr << e.what() << '\n';
    }
    
}
