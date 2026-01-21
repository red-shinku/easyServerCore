#include "Epoll.h"
#include <errno.h>
#include <stdexcept>
#include <spdlog/spdlog.h>

#define EPOLLMOD EPOLLET

using namespace easysv;

Epoll::Epoll():
ev_sample{0, 0}, epfd(0)
{ }

void Epoll::init()
{
    if((epfd = epoll_create1(0)) == -1)
    {
        spdlog::error("failed to create epoll");  
        throw std::runtime_error("epoll_create1 error");
    }
}

void Epoll::register_fd(int connfd, EPOLL_EVENTS care_event)
{
    ev_sample.data.fd = connfd;
    ev_sample.events = care_event | EPOLLMOD;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev_sample) == -1)
    {
        spdlog::error("failed to add epoll");
        throw std::system_error(
            errno,                     
            std::system_category(),
            "Epoll.addfd failed"   
        );
    }
}

void Epoll::change_fd_event(int connfd, EPOLL_EVENTS care_event)
{
    ev_sample.data.fd = connfd;
    ev_sample.events = care_event | EPOLLMOD;
    if(epoll_ctl(epfd, EPOLL_CTL_MOD, connfd, &ev_sample) == -1)
    {
        spdlog::error("failed to change epoll event");
        throw std::system_error(
            errno,                     
            std::system_category(),
            "Epoll.addfd failed"   
        );
    }
}

void Epoll::deletefd(int connfd)
{
    if(epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL) == -1)
    {
        spdlog::error("failed to del epoll");
        throw std::system_error(
            errno,                     
            std::system_category(),
            "Epoll.deletefd failed"   
        );
    }
}

std::tuple<Epoll::fdarray_t, int> Epoll::wait(fdarray_t&& fdlist)
{
    int n = 0;
    if((n = epoll_wait(epfd, events, Epoll::EventArraySize, 500)) == -1)
    {
        spdlog::error("failed from epoll_wait()");
        throw std::system_error(
            errno,                     
            std::system_category(),
            "Epoll.wait failed"   
        );
    }
    for(int i =0; i< n; ++i)
    {
        fdlist[i].first = events[i].data.fd;
        fdlist[i].second = events[i].events;
    }
    return {std::move(fdlist), n};
}

    
