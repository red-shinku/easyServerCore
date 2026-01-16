#include "Coro_sheduler.h"

#include <iostream>
#include <cppcoro/sync_wait.hpp>
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
        delete_coro(coro.first);
    }
}

void Coro_sheduler::wait_read(int fd, handle_t coro_handle)
{
    //TODO: 如果状态变化修改epoll
    coros.at(fd).state = EASYSV_READING;
}

void Coro_sheduler::wait_write(int fd, handle_t coro_handle)
{
    coros.at(fd).state = EASYSV_WRITING;
}

void Coro_sheduler::run()
{
    for(int i = 0; i < ready_num; ++i)
    {
        try
        {
            auto &fddetail = coros.at(fdlist[i].first);
            if(fddetail.care_event & fdlist[i].second)
            {
                auto handle = cppcoro::sync_wait(*fddetail.coro_handle);
                //FIXME: 保存返回的句柄，协程每次返回下一步状态，需要设置
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

void Coro_sheduler::ready(Epoll::fdarray_t&& fdlist, int rdynum)
{
    this->fdlist = fdlist;
    ready_num = rdynum;
}

Epoll::fdarray_t Coro_sheduler::return_fdlist()
{
    return std::move(fdlist);
}

void Coro_sheduler::register_coro(int connfd, uint32_t care_event, handle_t* coro)
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

void Coro_sheduler::delete_coro(int connfd)
{
    if(coros.find(connfd) != coros.end())
    {
        delete coros.at(connfd).coro_handle;
        coros.erase(connfd);
        close(connfd); //FIXME: 处理close出错
    }
    else
    {
        spdlog::error("Delete an unexisting Fd in coros map");
        //FIXME: 不处理删除不存在的键会导致问题么
    }
}

