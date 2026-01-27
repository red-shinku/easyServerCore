#pragma once

#include <sys/epoll.h>
#include <functional>

namespace easysv
{
class Coro_scheduler;
class task;

using callable_coro_t = std::function<task(Coro_scheduler&, int/*fd*/)>;

typedef struct Task_type
{
    //a function pack, use to create coro
    easysv::callable_coro_t task_template;
    EPOLL_EVENTS initial_care_event;   
}Task_type;

} // namespace easysv
