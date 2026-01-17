#include "Coro_sheduler.h"

#include <iostream>
#include <cppcoro/sync_wait.hpp>
#include <spdlog/spdlog.h>

using namespace easysv;

//TODO: 初始化map
Coro_sheduler::Coro_sheduler():
epoll(), fdlist(), coros{}, ready_num(0)
{ 
    epoll.init();
}

Coro_sheduler::~Coro_sheduler()
{
    for(auto &coro: coros)
    {
        delete_coro(coro.first);
    }
}

void Coro_sheduler::wait_read(int fd, handle_t coro_handle)
{
    //如果状态变化修改epoll
    try
    {
        if(coros.at(fd).state == EPOLLOUT)
        {
            epoll.change_fd_event(fd, EPOLLIN);
            coros.at(fd).state = EPOLLIN;
        }
    }
    catch(const std::out_of_range& e)
    {
        spdlog::error("Coro_sheduler::wait_read: fd not found in coros map");
        std::cerr << e.what() << '\n';
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Coro_sheduler::wait_write(int fd, handle_t coro_handle)
{
    try
    {
        if(coros.at(fd).state == EPOLLIN)
        {
            epoll.change_fd_event(fd, EPOLLOUT);
            coros.at(fd).state = EPOLLOUT;
        }
    }
    catch(const std::out_of_range& e)
    {
        spdlog::error("Coro_sheduler::wait_write: fd not found in coros map");
        std::cerr << e.what() << '\n';
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Coro_sheduler::register_wait_(int fd, handle_t coro_handle, EPOLL_EVENTS initial_care_event)
{
    try
    {
        if(coros.find(fd) != coros.end())
        {
            coros.emplace(fd, coro_handle, initial_care_event);
            epoll.register_fd(fd, initial_care_event);
        }
        else
            spdlog::warn("Coro_sheduler::register_wait_read: The FD {} has been register", fd);
    }
    catch(const std::bad_alloc& e)
    {
        spdlog::error("Coro_sheduler::register_wait_read: Register failed");
        std::cerr << e.what() << '\n';
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Coro_sheduler::run()
{
    for(int i = 0; i < ready_num; ++i)
    {
        try
        {
            auto &fddetail = coros.at(fdlist[i].first);
            if(fddetail.state == fdlist[i].second)
            {
                //协程运行
                fddetail.coro_handle.resume();
            }
            else
                continue;
        }
        catch(const std::out_of_range& e)
        {
            spdlog::error("run sheduler: Fd not found in coros map");
            std::cerr << e.what() << '\n';
            continue;
        }   
    }
}

void Coro_sheduler::ready_next_run()
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

void Coro_sheduler::register_coro(int connfd, callable_coro_t coro)
{
    try
    {
        if(coros.find(connfd) == coros.end())
        {   //first run a coro and register
            coro(
                [this] -> Coro_sheduler& { return *this; } (),
                connfd
            ); 
        }
        else
        {
            spdlog::error("Register an existing Fd in coros map");
            //FIXME: 注册已存在键是否抛异常？
        }   
    }
    catch(const std::bad_alloc& e)
    {
        spdlog::error("out of memory: coros map");
        std::cerr << e.what() << '\n';
    }
    
}

void Coro_sheduler::delete_coro(int connfd)
{
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
    }
    else
    {
        spdlog::error("Delete an unexisting Fd in coros map");
        //FIXME: 不处理删除不存在的键会导致问题么
    }
}

