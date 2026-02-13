#pragma once

#include <cstdint>
#include <vector>
#include <tuple>
#include <sys/epoll.h>

namespace easysv
{
class Epoll
{
private:
    // static constexpr int EventArraySize = 50;
    using event_t = struct epoll_event;
    event_t ev_sample;
    std::vector<event_t> events;
    int epfd;

    //this array create by outside, move in to handle
    using fdarray_t = std::vector<std::pair<int, uint32_t>>;

public:
    explicit Epoll(/* args */);
    ~Epoll();
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;
    
    Epoll(Epoll&&) = delete;
    Epoll& operator=(Epoll&&) = delete;

    //epoll_create1
    void init();
    //epoll_ctl
    void register_fd(int fd, EPOLL_EVENTS care_event, 
                    uint32_t EPOLLMOD);
    void change_fd_event(int fd, EPOLL_EVENTS care_event, uint32_t EPOLLMOD);
    void deletefd(int fd);
    //epoll_wait
    std::tuple<fdarray_t, int> wait(fdarray_t&& fdlist);
    std::tuple<fdarray_t, int> wait();

};

    
} // namespace easysv
