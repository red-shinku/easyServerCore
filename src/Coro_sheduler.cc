#include "Coro_sheduler.h"

#include <iostream>
#include <coroutine>
#include <spdlog/spdlog.h>

using namespace easysv;

//TODO: 初始化map
Coro_sheduler::Coro_sheduler():
fdlist(), coros{}, ready_num(0)
{ }

Coro_sheduler::~Coro_sheduler()
{
    for(auto &coro: coros)
    {
        deletecoro(coro.first);
    }
}

coro_t Coro_sheduler::run()
{
    try
    {
        for(auto& connfd: fdlist)
        {
            auto &fddetail = coros.at(connfd);
            if(fddetail.care_event != fddetail.state)
                continue;
            co_await *fddetail.coro;       
        }
    }
    catch(const std::out_of_range& e)
    {
        //FIXME: fd列表是单回合有效的，ET下发生异常将导致未处理的fd死去
        //错误处理包在循环内？
        spdlog::error("run sheduler: Fd not found in coros map");
        std::cerr << e.what() << '\n';
    }
}

void Coro_sheduler::ready(Epoll::fdarray_t&& fdlist, int rdynum)
{
    this->fdlist = fdlist;
    ready_num = rdynum;
}

Epoll::fdarray_t Coro_sheduler::get_fdlist()
{
    return std::move(fdlist);
}

void Coro_sheduler::addcoro(int connfd, uint32_t care_event, coro_t* coro)
{
    try
    {
        if(coros.find(connfd) == coros.end())
        {
            FdDetail fddetail = {
                coro, care_event, EASYSV_WAIT
            };
            coros.emplace(connfd, fddetail);
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

void Coro_sheduler::deletecoro(int connfd)
{
    if(coros.find(connfd) != coros.end())
    {
        delete coros.at(connfd).coro;
        coros.erase(connfd);
        close(connfd); //FIXME: 处理close出错
    }
    else
    {
        spdlog::error("Delete an unexisting Fd in coros map");
        //FIXME: 不处理删除不存在的键会导致问题么
    }
}

