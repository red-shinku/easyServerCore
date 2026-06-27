#include "Coro_scheduler_int.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include "../include/config.h"

using namespace easysv;

// ======================================================================
// task::promise_type
// ======================================================================

task::promise_type::promise_type(Coro_scheduler& sched, int connfd):
sched(sched), connfd(connfd) { }

task task::promise_type::get_return_object()
{
    return task{ std::coroutine_handle<promise_type>::from_promise(*this) };
}

AwaitInit task::promise_type::initial_suspend() noexcept
{
    return AwaitInit{};
}

AwaitFin task::promise_type::final_suspend() noexcept
{
    return AwaitFin{};
}

// ======================================================================
// AwaitInit — registers the fd in Coro_scheduler when coroutine starts.
// ======================================================================

void AwaitInit::await_suspend(std::coroutine_handle<task::promise_type> h)
{
    h.promise().sched.__register_coro__(
        h.promise().connfd,
        h
    );
}

// ======================================================================
// AwaitFin  — moves the handle into the ending queue when coroutine
//             completes (co_return / co_await with EPOLLERR / EPOLLHUP).
// ======================================================================

void AwaitFin::await_suspend(std::coroutine_handle<task::promise_type> h) noexcept
{
    h.promise().sched.unregister_coro(
        h.promise().connfd,
        h
    );
}

// ======================================================================
// Awaitable
// ======================================================================

Awaitable::Awaitable(Coro_scheduler& sched, int fd, EPOLL_EVENTS care_event):
sched(sched), fd(fd), care_event(care_event) { }

void Awaitable::await_suspend(handle_t coro_han)
{
    sched.wait_event(fd, coro_han, care_event);
}

// ======================================================================
// Coro_scheduler
// ======================================================================

Coro_scheduler::Coro_scheduler(EPOLL_EVENTS initial_care_event,
                            int listen_fd,
                            std::function<void(int, EPOLL_EVENTS, uint32_t)> ep_reg,
                            std::function<void(int, EPOLL_EVENTS, uint32_t)> ep_ctl,
                            std::function<void(int)> ep_del):
initial_care_event(initial_care_event),
listen_fd(listen_fd),
register_fd(std::move(ep_reg)),
change_fd_event(std::move(ep_ctl)),
unregister_fd(std::move(ep_del))
{ }

Coro_scheduler::~Coro_scheduler()
{
    for(auto &coro: coros)
    {
        coro.second.coro_handle.destroy();
    }
}

void Coro_scheduler::wait_event(int fd, handle_t coro_handle, EPOLL_EVENTS state)
{
    try
    {
        if(coros.at(fd).state != state)
        {
            change_fd_event(fd, state, 0);
            coros.at(fd).state = state;
        }
    }
    catch(const std::out_of_range& e)
    {
        spdlog::error("Coro_scheduler::wait_read: fd not found in coros map");
        std::cerr << e.what() << '\n';
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Coro_scheduler::__register_coro__(int fd, handle_t coro_handle)
{
    try
    {
        if(coros.find(fd) == coros.end())
        {
            FdDetail fddetail{coro_handle, initial_care_event};
            coros.emplace(fd, std::move(fddetail));
            register_fd(fd, initial_care_event, g_config.EPOLLMOD);
        }
        else
            spdlog::warn("Coro_scheduler::register_wait_read: The FD {} has been register", fd);
    }
    catch(const std::bad_alloc& e)
    {
        spdlog::error("Coro_scheduler::register_wait_read: Register failed");
        std::cerr << e.what() << '\n';
    }
    catch(const std::system_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Coro_scheduler::unregister_coro(int connfd, handle_t handle)
{
    ending_queue.push_back(handle);
    if(coros.find(connfd) != coros.end())
    {
        try
        {
            unregister_fd(connfd);
            coros.erase(connfd);
            close(connfd);
            spdlog::info("finish sock {}", connfd);
        }
        catch(const std::system_error& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    else
    {
        spdlog::warn("Delete an unexisting Fd in coros map");
    }
}

void Coro_scheduler::clean_coro()
{
    for(auto& handle: ending_queue)
    {
        handle.destroy();
    }
    ending_queue.clear();
}

void Coro_scheduler::run(fdarray_t* readylist, int readynum)
{
    for(int i = 0; i < readynum; ++i) {
        int fd = (*readylist)[i].first;
        auto it = coros.find(fd);
        if (it == coros.end()) continue; //has been unregister or other fd

        auto &fddetail = it->second;
        if( (*readylist)[i].second & (fddetail.state | EPOLLHUP | EPOLLRDHUP | EPOLLERR) ) {
            fddetail.coro_handle.resume();
        }
    }
    clean_coro();
}

void Coro_scheduler::register_coro(int connfd, callable_coro_t coro)
{
    try
    {
        if(coros.find(connfd) == coros.end())
        {   //first run a coro and register
            coro(
                [this]() -> Coro_scheduler& { return *this; } (),
                connfd
            );
        }
        else
        {
            spdlog::warn("Register an existing Fd in coros map");
        }
    }
    catch(const std::bad_alloc& e)
    {
        spdlog::error("out of memory: coros map");
        std::cerr << e.what() << '\n';
    }

}
