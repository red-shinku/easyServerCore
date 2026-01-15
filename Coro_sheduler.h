// ----------------------------------------------------------
// a coroutine sheduler for one thread.
// It also responsible for the management of coro.
// make sure the instance of Class Coro_sheduler is build-
// on-heapmemory. 
// ----------------------------------------------------------
#pragma once

#include <cstdint>
#include <unordered_map>
#include <cppcoro/task.hpp>
#include "Epoll.h"

namespace easysv 
{
using coro_t = cppcoro::task<void>;

enum ConnfdState
{
    EASYSV_READING = 0x001,
    EASYSV_WRITING = 0x004,
    EASYSV_WAIT = 0x002
};

typedef struct FdDetail
{
    //coros to be sheduler
    coro_t* coro;
    uint32_t care_event;
    ConnfdState state; 
}FdDetail;

class Coro_sheduler
{
private:
    Epoll::fdarray_t fdlist;
    std::unordered_map<int, FdDetail> coros;
    int ready_num;

public:
    explicit Coro_sheduler(/* args */);
    ~Coro_sheduler() noexcept;
    Coro_sheduler(const Coro_sheduler&) = delete;
    Coro_sheduler& operator=(const Coro_sheduler&) = delete;
    
    Coro_sheduler(Coro_sheduler&&) = default;
    Coro_sheduler& operator=(Coro_sheduler&&) = default;

    //the thread's main coro
    coro_t run();

    void ready(Epoll::fdarray_t&& fdlist, int rdynum);
    Epoll::fdarray_t get_fdlist();
    void addcoro(int connfd, uint32_t care_event, coro_t* coro);
    //FIXME: 何时关闭一个fd？
    void deletecoro(int connfd) noexcept;
};


}