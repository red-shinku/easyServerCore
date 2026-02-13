#include "Epoll.h"
#include <errno.h>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include "../include/config.h"

using namespace easysv;

Epoll::Epoll():
ev_sample{0, 0}, epfd(0)
{ }

Epoll::~Epoll()
{
    close(epfd);
}

void Epoll::init()
{
    if((epfd = epoll_create1(0)) == -1)
    {
        spdlog::error("failed to create epoll");  
        throw std::runtime_error("epoll_create1 error");
    }
    events.resize(g_config.EventArraySize);
}

void Epoll::register_fd(int fd, EPOLL_EVENTS care_event, uint32_t EPOLLMOD)
{
    ev_sample.data.fd = fd;
    ev_sample.events = care_event | EPOLLMOD;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev_sample) == -1)
    {
        spdlog::error("failed to add epoll");
        throw std::system_error(
            errno,                     
            std::system_category(),
            "Epoll.addfd failed"   
        );
    }
}

void Epoll::change_fd_event(int fd, EPOLL_EVENTS care_event, uint32_t EPOLLMOD)
{
    ev_sample.data.fd = fd;
    ev_sample.events = care_event | EPOLLMOD;
    if(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev_sample) == -1)
    {
        spdlog::error("failed to change epoll event");
        throw std::system_error(
            errno,                     
            std::system_category(),
            "Epoll.addfd failed"   
        );
    }
}

void Epoll::deletefd(int fd)
{
    if(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
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
    do
    {
        n = epoll_wait(epfd, events.data(), g_config.EventArraySize, -1);
    } while (n == -1 && errno == EINTR);
    
    if(n == -1 && errno != EINTR)
    {
        spdlog::error("failed from epoll_wait()");
        throw std::system_error(
            errno,                     
            std::generic_category(),
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

std::tuple<Epoll::fdarray_t, int> Epoll::wait()
{
    Epoll::fdarray_t fds(g_config.EventArraySize);
    if (fds.size() < static_cast<size_t>(g_config.EventArraySize))
        fds.resize(g_config.EventArraySize);
    return wait(std::move(fds));
}

    
