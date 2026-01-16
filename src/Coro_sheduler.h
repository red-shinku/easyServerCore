// ----------------------------------------------------------
// a coroutine sheduler for one thread.
// It also responsible for the management of coro.
// ----------------------------------------------------------
#pragma once

#include <cstdint>
#include <unordered_map>
#include <coroutine>
#include <cppcoro/task.hpp>
#include "Epoll.h"

namespace easysv 
{
using coro_t = cppcoro::task<void>;
using handle_t = std::coroutine_handle<>;

enum ConnfdState
{
    EASYSV_READING = 0x001,
    EASYSV_WRITING = 0x004,
    // EASYSV_WAIT = 0x002
};

typedef struct FdDetail
{
    //coros to be sheduler
    handle_t* coro_handle;
    uint32_t care_event;
    ConnfdState state; 
}FdDetail;

struct AwaitReadable;
struct AwaitWriteable;

class Coro_sheduler
{
    friend struct AwaitReadable;
    friend struct AwaitWriteable;

private:
    Epoll::fdarray_t fdlist; //{fd, care-event}
    std::unordered_map<int, FdDetail> coros;
    int ready_num;

    //method for coro to call in Await, when it suspend
    void wait_read(int fd, handle_t);
    void wait_write(int fd, handle_t);

public:
    explicit Coro_sheduler(/* args */);
    ~Coro_sheduler() noexcept;
    Coro_sheduler(const Coro_sheduler&) = delete;
    Coro_sheduler& operator=(const Coro_sheduler&) = delete;
    
    Coro_sheduler(Coro_sheduler&&) = default;
    Coro_sheduler& operator=(Coro_sheduler&&) = default;

    //the thread's main coro
    void run();

    Epoll::fdarray_t return_fdlist();
    void ready(Epoll::fdarray_t&& fdlist, int rdynum);
    void register_coro(int connfd, uint32_t care_event, handle_t* coro);
    //FIXME: 何时关闭一个fd？
    void delete_coro(int connfd) noexcept;
};

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

}