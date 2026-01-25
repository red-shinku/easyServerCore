#pragma once

#include <cstdint>
#include <array>
#include <tuple>
#include <sys/epoll.h>

namespace easysv
{
class Epoll
{
private:
    static constexpr int EventArraySize = 50;
    using event_t = struct epoll_event;
    event_t ev_sample;
    event_t events[EventArraySize];
    int epfd;

public:
    explicit Epoll(/* args */);
    ~Epoll();
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;
    
    Epoll(Epoll&&) = default;
    Epoll& operator=(Epoll&&) = default;

    //this array create by outside, move in to handle
    using fdarray_t = std::array<std::pair<int, uint32_t>, EventArraySize>;

    //epoll_create1
    void init();
    //epoll_ctl
    void register_fd(int connfd, EPOLL_EVENTS care_event);
    void change_fd_event(int connfd, EPOLL_EVENTS care_event);
    void deletefd(int connfd);
    //epoll_wait
    std::tuple<fdarray_t, int> wait(fdarray_t&& fdlist);
};

    
} // namespace easysv
