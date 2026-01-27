#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include "../include/Types.h"
#include "../include/Coro_scheduler.h"

// #define IS_IDLE_NUM 5 

namespace easysv
{

class WorkT
{
public:
    /*eventfd for this thread, which be registered into 
    epoll, Tpool write it to wake up thread when accept newfd*/
    int notify_fd;

private:
    int task_num;
    //what this thread should do
    Task_type& taskt;
    Coro_scheduler coro_sheduler;
    std::thread worker;
    //index of this thread in Tpool
    int id;
    //防止重复加入空闲队列
    bool is_idle_now = true; 

    //a call back function for getting fd from pool
    std::function<std::vector<int>()> getfd;
    //a call back function to push thread ifself to Tpoll idle_q
    std::function<void()> say_idle;

    void register_coro(int fd);
    //get new fd from public queue
    void handle_publicq();
    void del_fd_in_epoll(int fd);
    // function run in thread
    void work();

    std::atomic<bool>& stopping;

public:
    explicit WorkT(std::function<std::vector<int>()>, std::function<void()>, 
                    Task_type& taskt, int id, int efd, 
                    std::atomic<bool>& stopping_flag) noexcept;
    ~WorkT() noexcept; 
    WorkT(const WorkT&) = delete;
    WorkT& operator=(const WorkT&) = delete;

    WorkT(WorkT&&) = delete;
    WorkT& operator=(WorkT&&) = delete;

};

} // namespace easysv
