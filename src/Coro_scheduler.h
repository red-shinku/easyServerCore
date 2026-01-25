// -----------------------------------------------------------------------
// this file define : task(coro type), Coro_scheduler and two Awaitable --
// AwaitReadable and AwaitWriteable
// 
// the coroutine sheduler is for one thread.
// It also responsible for the management of coro(save a list of handles).
// -----------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <unordered_map>
#include <functional>
#include <exception>
#include <coroutine>
#include "Epoll.h"

namespace easysv 
{

class Coro_scheduler;

class task 
{
public:
    ~task() { /*no destory handle*/ }
    struct promise_type 
    {
        promise_type(Coro_scheduler& sched, int connfd):
        sched(sched), connfd(connfd) { }

        Coro_scheduler& sched;
        int connfd;
        
        //when first run a coro, suspend at first and register it
        auto initial_suspend() noexcept;

        //when co_return, put the handle in ending_queue
        auto final_suspend() noexcept;

        task get_return_object() 
        { 
            return task{ std::coroutine_handle<promise_type>::from_promise(*this) }; 
        }

        void unhandled_exception() { std::terminate(); }
        
        void return_void() {}
    };

    std::coroutine_handle<promise_type> handle;

};

using coro_t = task;
using handle_t = std::coroutine_handle<task::promise_type>;
using callable_coro_t = std::function<coro_t(Coro_scheduler&, int/*fd*/)>;

typedef struct FdDetail
{
    //coros to be sheduler
    handle_t coro_handle;
    uint32_t state;
}FdDetail;


class Coro_scheduler
{
    friend struct Awaitable;
    friend struct task;

private:
    Epoll epoll;
    EPOLL_EVENTS initial_care_event;
    Epoll::fdarray_t fdlist; //{fd, care-event}
    std::unordered_map<int, FdDetail> coros;
    std::vector<handle_t> ending_queue;
    int ready_num;
    int& task_num;
    int notify_fd;

    //use in initial_suspend
    void __register_coro__(int fd, handle_t);
    //use in final_suspend, move the handle to ending queue
    void unregister_coro(int connfd, handle_t);
    //take handles in ending queue and destory it
    void destory_coro();

    //method for coro to call in Await, when it suspend
    void wait_event(int fd, handle_t, EPOLL_EVENTS state);

public:
    explicit Coro_scheduler(EPOLL_EVENTS initial_care_event, 
                            int& task_num, int notify_fd);
    ~Coro_scheduler() noexcept;
    Coro_scheduler(const Coro_scheduler&) = delete;
    Coro_scheduler& operator=(const Coro_scheduler&) = delete;
    
    Coro_scheduler(Coro_scheduler&&) = default;
    Coro_scheduler& operator=(Coro_scheduler&&) = default;

    //the thread's main coro
    void run();
    void ready_next_run();

    //work thread call it to register coro
    void register_coro(int connfd, callable_coro_t coro);

    void register_notify_fd(int efd);
};

struct Awaitable
{
    Awaitable(Coro_scheduler& sched, int fd, EPOLL_EVENTS care_event):
    sched(sched), fd(fd), care_event(care_event) { }

    Coro_scheduler& sched;
    int fd;
    EPOLL_EVENTS care_event;

    bool await_ready() const noexcept { return false; }
    void await_suspend(handle_t coro_han)
    { sched.wait_event(fd, coro_han, care_event); }
    void await_resume() noexcept { }
};

inline auto task::promise_type::initial_suspend() noexcept
{
    struct AwaitFirst
    {
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<promise_type> h)
        {
            h.promise().sched.__register_coro__(
                h.promise().connfd,
                h
            );
        }
        void await_resume() const noexcept {}
    };
    return AwaitFirst{};
}

inline auto task::promise_type::final_suspend() noexcept
{
    struct AwaitFinal
    {
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<promise_type> h) noexcept
        {
            h.promise().sched.unregister_coro(
                h.promise().connfd,
                h
            );
        }
        void await_resume() const noexcept {}
    };
    return AwaitFinal{};    
}

}