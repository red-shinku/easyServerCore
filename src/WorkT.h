#pragma once

#include <thread>
#include <atomic>
#include "../include/Types.h"
#include "../include/Coro_scheduler.h"
#include "Epoll.h"
#include "private_types.h"

// #define IS_IDLE_NUM 5 

namespace easysv
{

class WorkT
{
public:
    //for tpool to this notify thread
    int notify_fd;

private:
    //listen fd own by this thread
    int listen_sock_fd;
    //what this thread should do
    Task_type& taskt;
    Epoll epoll;
    Coro_scheduler coro_sheduler;
    fdarray_t readylist;
    std::thread worker;
    //index of this thread in Tpool
    int id;

    struct sockaddr_in client_addr;

    void register_coro(int fd);
    void del_fd_in_epoll(int fd);

    //listen sock accept
    int tcpsv_accept();
    // function run in thread
    void work();

    std::atomic<bool>& stopping;

public:
    explicit WorkT(int notify_fd, int listen_sock_fd, 
                Task_type& taskt, int id,
                std::atomic<bool>& stopping_flag);
    ~WorkT() noexcept; 
    WorkT(const WorkT&) = delete;
    WorkT& operator=(const WorkT&) = delete;

    WorkT(WorkT&&) = delete;
    WorkT& operator=(WorkT&&) = delete;

};

} // namespace easysv
