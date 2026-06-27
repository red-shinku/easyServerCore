// -----------------------------------------------------------------------
// Public API: user-facing coroutine types and awaitables.
//
// Users see:
//   easysv::task             — coroutine return type
//   easysv::Coro_scheduler   — forward declaration (parameter type only)
//   easysv::Awaitable        — co_await {sched, fd, EPOLL_EVENT}
//   easysv::coro_t /         — convenience aliases
//   easysv::handle_t
//
// Internal details (FdDetail, full Coro_scheduler definition) live in
// src/Coro_scheduler_int.h and are never seen by user code.
// -----------------------------------------------------------------------
#pragma once

#include <sys/epoll.h>
#include <cstdint>
#include <coroutine>
#include <exception>

namespace easysv
{

// forward declarations so promise_type can return them
struct AwaitInit;
struct AwaitFin;

// -----------------------------------------------------------------------
// Coro_scheduler — forward declaration only.  Use the handle as a
// parameter in your coroutine signature; the full class definition
// is an internal detail.
// -----------------------------------------------------------------------
class Coro_scheduler;

// -----------------------------------------------------------------------
// task — the coroutine return type.
//
// Declare your coroutine as:
//   easysv::task my_coro(easysv::Coro_scheduler& sched, int fd);
//
// The coroutine handle lifecycle is managed by Coro_scheduler; the
// destructor intentionally does NOT destroy the handle.
// -----------------------------------------------------------------------
class task
{
public:
    struct promise_type
    {
        Coro_scheduler& sched;
        int             connfd;

        promise_type(Coro_scheduler& sched, int connfd);

        AwaitInit initial_suspend() noexcept;
        AwaitFin   final_suspend()   noexcept;

        task get_return_object();
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
    };

    std::coroutine_handle<promise_type> handle;

    ~task() { /*no destroy handle*/ }
};

using coro_t   = task;
using handle_t = std::coroutine_handle<task::promise_type>;

// -----------------------------------------------------------------------
// AwaitInit — returned by promise_type::initial_suspend().
// The await_suspend body lives in Coro_scheduler.cc.
// -----------------------------------------------------------------------
struct AwaitInit
{
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<task::promise_type> h);
    void await_resume() const noexcept {}
};

// -----------------------------------------------------------------------
// AwaitFin — returned by promise_type::final_suspend().
// The await_suspend body lives in Coro_scheduler.cc.
// -----------------------------------------------------------------------
struct AwaitFin
{
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<task::promise_type> h) noexcept;
    void await_resume() const noexcept {}
};

// -----------------------------------------------------------------------
// Awaitable — the main user-facing co_await helper.
//
// Usage:
//   co_await Awaitable{sched, fd, EPOLLIN};
//   co_await Awaitable{sched, fd, EPOLLOUT};
// -----------------------------------------------------------------------
struct Awaitable
{
    Coro_scheduler& sched;
    int             fd;
    EPOLL_EVENTS    care_event;

    Awaitable(Coro_scheduler& sched, int fd, EPOLL_EVENTS care_event);

    bool await_ready() const noexcept { return false; }
    void await_suspend(handle_t coro_han);
    void await_resume() noexcept {}
};

} // namespace easysv
