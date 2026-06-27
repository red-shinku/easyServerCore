// -----------------------------------------------------------------------
// Internal header — full Coro_scheduler definition and helper types.
//
// This header is included only by WorkT.h and Coro_scheduler.cc.
// Users of the library never see these internals.
// -----------------------------------------------------------------------
#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <sys/epoll.h>
#include <coroutine>

#include "../include/Coro_scheduler.h"

namespace easysv
{

// -----------------------------------------------------------------------
// FdDetail — state associated with one file descriptor.
// -----------------------------------------------------------------------
struct FdDetail
{
    handle_t  coro_handle;  // coroutine that owns this fd
    uint32_t  state;        // current epoll event mask
};

// -----------------------------------------------------------------------
// Coro_scheduler — per-thread coroutine scheduler.
//
// Each work thread owns exactly one instance. It maps fds to their active
// coroutines, registers/unregisters/mutates epoll interest via callbacks,
// and drives one round of coroutine dispatch on each run() call.
// -----------------------------------------------------------------------
class Coro_scheduler
{
    friend struct Awaitable;
    friend struct task;
    friend struct AwaitInit;
    friend struct AwaitFin;

private:
    using callable_coro_t = std::function<coro_t(Coro_scheduler&, int/*fd*/)>;
    using fdarray_t       = std::vector<std::pair<int, uint32_t>>;

    EPOLL_EVENTS initial_care_event;
    std::unordered_map<int, FdDetail> coros;
    std::vector<handle_t>             ending_queue;
    int                               listen_fd;

    // epoll callbacks — injected by WorkT
    std::function<void(int, EPOLL_EVENTS, uint32_t)> register_fd;
    std::function<void(int, EPOLL_EVENTS, uint32_t)> change_fd_event;
    std::function<void(int)>                         unregister_fd;

    // used by initial_suspend
    void __register_coro__(int fd, handle_t);
    // used by final_suspend     — moves handle to ending_queue
    void unregister_coro(int connfd, handle_t);
    // destroy handles in ending_queue
    void clean_coro();

    // called from Awaitable::await_suspend
    void wait_event(int fd, handle_t, EPOLL_EVENTS state);

public:
    explicit Coro_scheduler(EPOLL_EVENTS initial_care_event,
                            int listen_fd,
                            std::function<void(int, EPOLL_EVENTS, uint32_t)> ep_reg,
                            std::function<void(int, EPOLL_EVENTS, uint32_t)> ep_ctl,
                            std::function<void(int)> ep_del);
    ~Coro_scheduler() noexcept;
    Coro_scheduler(const Coro_scheduler&)            = delete;
    Coro_scheduler& operator=(const Coro_scheduler&) = delete;
    Coro_scheduler(Coro_scheduler&&)                 = delete;
    Coro_scheduler& operator=(Coro_scheduler&&)      = delete;

    // the thread's main dispatch loop — called from work()
    void run(fdarray_t* readylist, int readynum);

    // called from WorkT to start a new coroutine for a connected client
    void register_coro(int connfd, callable_coro_t coro);
};

} // namespace easysv
