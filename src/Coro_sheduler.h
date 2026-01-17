// ----------------------------------------------------------
// a coroutine sheduler for one thread.
// It also responsible for the management of coro.
// ----------------------------------------------------------
#pragma once

#include <cstdint>
#include <unordered_map>
#include <functional>
#include <coroutine>
#include <cppcoro/task.hpp>
#include "Epoll.h"

namespace easysv 
{
using coro_t = cppcoro::task<void>;
using handle_t = std::coroutine_handle<>;

// enum ConnfdState
// {
    // EASYSV_READING = 0x001,
    // EASYSV_WRITING = 0x004,
// };

typedef struct FdDetail
{
    //coros to be sheduler
    handle_t coro_handle;
    uint32_t state;
    // ConnfdState state; 
}FdDetail;

struct AwaitReadable;
struct AwaitWriteable;

class Coro_sheduler
{
    friend struct AwaitReadable;
    friend struct AwaitWriteable;
    friend struct AwaitCall;

private:
    Epoll epoll;
    Epoll::fdarray_t fdlist; //{fd, care-event}
    std::unordered_map<int, FdDetail> coros;
    int ready_num;

    //method for coro to call in Await, when it suspend
    void wait_read(int fd, handle_t);
    void wait_write(int fd, handle_t);
    //a coro call it first in AwaitCall, to register
    void register_wait_(int fd, handle_t, EPOLL_EVENTS initial_care_event);

public:
    explicit Coro_sheduler(/* args */);
    ~Coro_sheduler() noexcept;
    Coro_sheduler(const Coro_sheduler&) = delete;
    Coro_sheduler& operator=(const Coro_sheduler&) = delete;
    
    Coro_sheduler(Coro_sheduler&&) = default;
    Coro_sheduler& operator=(Coro_sheduler&&) = default;

    //the thread's main coro
    void run();
    void ready_next_run();

    void register_coro(int connfd, callable_coro_t coro);
    //FIXME: 何时关闭一个fd？
    void delete_coro(int connfd);
};

//FIXME: 是不是要shared_task？或者一个工厂函数
using callable_coro_t = std::function<coro_t(Coro_sheduler&, int/*fd*/)>;

struct AwaitReadable
{
    Coro_sheduler& sched;
    int fd;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> coro_han)
    { sched.wait_read(fd, coro_han); }
    void await_resume() noexcept { }
};

struct AwaitWriteable
{
    Coro_sheduler& sched;
    int fd;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> coro_han)
    { sched.wait_write(fd, coro_han); }
    void await_resume() noexcept { }
};

struct AwaitCall
{
    AwaitCall(Coro_sheduler& sched, int fd, EPOLL_EVENTS care_event):
    sched(sched), fd(fd), care_event(care_event) {}

    Coro_sheduler& sched;
    EPOLL_EVENTS care_event;
    int fd;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> coro_han)
    { sched.register_wait_(fd, coro_han, care_event); }
    void await_resume() noexcept { }
};

}