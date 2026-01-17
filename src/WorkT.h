#pragma once

#include <thread>
#include <functional>
#include "Coro_sheduler.h"

namespace easysv
{
typedef struct Task_type
{
    //a function pack, use to create coro
    easysv::callable_coro_t task_template;
    // EPOLL_EVENTS initial_care_event;   
}Task_type;

class WorkT
{
private:
    //what this thread should do
    Task_type& taskt;
    Coro_sheduler coro_sheduler;
    std::thread worker;

    //a call back function for getting fd from pool
    std::function<int()> getfd;
    void register_coro(int fd);
    //get new fd from public queue
    void handle_publicq();
    void del_fd_in_epoll(int fd);
    // function run in thread
    void work();

public:
    explicit WorkT(std::function<int()>, Task_type& taskt) noexcept;
    ~WorkT() noexcept; 
    WorkT(const WorkT&) = delete;
    WorkT& operator=(const WorkT&) = delete;

    WorkT(WorkT&&) = default;
    WorkT& operator=(WorkT&&) = default;
};

} // namespace easysv
