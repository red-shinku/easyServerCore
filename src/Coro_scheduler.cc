#include "Coro_scheduler.h"

#include <iostream>
#include <spdlog/spdlog.h>

using namespace easysv;

Coro_scheduler::Coro_scheduler(EPOLL_EVENTS initial_care_event, 
                            int& task_num, int& notify_fd):
epoll(), initial_care_event(initial_care_event), fdlist(), coros{}, 
ending_queue(), ready_num(0), task_num(task_num), notify_fd(notify_fd)
{
    epoll.init();
}

Coro_scheduler::~Coro_scheduler()
{
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
            epoll.change_fd_event(fd, state);
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
            epoll.register_fd(fd, initial_care_event);
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
            epoll.deletefd(connfd);
            coros.erase(connfd);
            close(connfd); //FIXME: 处理close出错
        }
        catch(const std::system_error& e)
        {
            std::cerr << e.what() << '\n';
        }
        --task_num;
    }
    else
    {
        spdlog::warn("Delete an unexisting Fd in coros map");
    }
}

void Coro_scheduler::destory_coro()
{
    for(auto& handle: ending_queue)
    {
        handle.destroy();
    }
    ending_queue.clear();
}

void Coro_scheduler::run()
{
    for(int i = 0; i < ready_num; ++i) {
        int fd = fdlist[i].first;
        //遇到eventfd跳过
        if(fd == notify_fd)
        {
            continue;
        }
        auto it = coros.find(fd);
        if (it == coros.end()) continue; //has been unregister

        auto &fddetail = it->second;
        if( fdlist[i].second & (fddetail.state | EPOLLHUP | EPOLLRDHUP | EPOLLERR) ) {
            fddetail.coro_handle.resume();
        }
    }
    destory_coro();
}


void Coro_scheduler::ready_next_run()
{   //wait epoll
    try
    {
        auto [rfdlist, rdynum] = epoll.wait(std::move(fdlist));
        this->fdlist = rfdlist;
        ready_num = rdynum;
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Coro_scheduler::register_coro(int connfd, callable_coro_t coro)
{
    try
    {
        if(coros.find(connfd) == coros.end())
        {   //first run a coro and register
            coro(
                [this] -> Coro_scheduler& { return *this; } (),
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

void Coro_scheduler::register_notify_fd(int efd)
{
    epoll.register_fd(efd, EPOLLIN);
}

