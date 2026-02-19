# English version ReadMe

- [English version ReadMe](#english-version-readme)
  - [What is this project?](#what-is-this-project)
  - [How to Use](#how-to-use)
    - [Use Steps](#use-steps)
  - [Attention](#Attention)
  - [Benchmarking](#Benchmarking)
  - [Implementation Details](#implementation-details)
  - [Build](#build)
  - [Announcement](#announcement)
  - [让我们说中文](#中文版请读我)

## What is this project?

**easyServerCore** is a TCP socket server based on a straightforward **Multi-threaded epoll + Coroutines** architecture. You can use the framework and interfaces provided by this project to build simple network applications at the top level.

**Note:** Only Linux is supported. Testing was completed on Debian 6.12.63-1 x86_64.

## How to Use

Include the header files`easysv/Coro_scheduler.h`, and then:   

1) **Define your logic:** Use`easysv::task coro_func_name(easysv::Coro_scheduler& sched, int fd)`，to define a coroutine function.    
2) **Handle Events:** Use`co_await easysv::Awaitable{sched, fd, next_epoll_event}` to specify which events your coroutine should wait for.  
3) **Finish:** Use`co_return` to signal the completion of a coroutine.    

**The user does not need to worry about the lifecycle management of coroutine handles.**  

### Use Steps

1) **Create a server instance:**
```cpp
#include <easysv/Server.h>

easysv::Server s(port, listen_sock_queue_size);
```
2) **Configure parameters:**
```cpp
#include <easysv/config.h>

struct Setting userset
{
    //the size of listen sock queue
    int LISTENQ;
    //each time thread get nums of fd when socket accept
    size_t EACH_ACCEPT_NUM;
    //the max nums of fd epoll wait return
    size_t EventArraySize;
    //epoll mod -- LT(0) or ET(EPOLLET)
    uint32_t EPOLLMOD;
};
```  
3) **Define task information:**
```cpp
#include <easysv/Types.h>

easysv::Task_type your_task
{
    //your app corotinue_function
    .task_template = coro_func_name,
    //your first event to wait
    .initial_care_event = EPOLL_EVENTS
};
```
4) **Initialize and Run:**
```cpp
s.init(
    thread_nums,
    &your_task,
    &userset
); 

s.run();
```
>example at /app/mainWebServer.cc

## Attention

1) In the `EPOLLMOD setting of struct Setting`, use `0` to specify LT mode and `EPOLLET` for ET mode.
2) This framework does not guarantee correctness under `epoll ET mode`; users must pay attention to and verify this themselves.  
3) This framework is `event-driven + coroutine-based` and **does not incorporate** `zero-copy technology`.  
4) Be aware of your process file descriptor limit (which determines the concurrency capacity); the default on Linux is 1024.  
5) The framework uses `SO_REUSEPORT`. For systems that do not support it (e.g., Linux kernel versions prior to 3.9), users can use version one from the v1 branch.

## Benchmarking

（Exclude network latency / disk IO）：

under 1000 concurrency single thread (use 100% core) ：
```
kktori@kotori ~ 
❯ rewrk -d 20s -h http://192.168.100.21:19198/ -t 10 -c 1000  
Beginning round 1...
Benchmarking 1000 connections @ http://192.168.100.21:19198/ for 20 second(s)
  Latencies:
    Avg      Stdev    Min      Max      
    9.85ms   1.03ms   0.11ms   102.38ms  
  Requests:
    Total: 2021291 Req/Sec: 101382.89
  Transfer:
    Total: 248.67 MB Transfer Rate: 12.47 MB/Sec
```

under 2000 concurrency single thread：
```
kktori@kotori ~ 
❯ rewrk -d 20s -h http://192.168.100.21:19198/ -t 10 -c 2000 
Beginning round 1...
Benchmarking 2000 connections @ http://192.168.100.21:19198/ for 20 second(s)
  Latencies:
    Avg      Stdev    Min      Max      
    20.54ms  2.25ms   0.03ms   169.58ms  
  Requests:
    Total: 1930922 Req/Sec: 96998.91
  Transfer:
    Total: 237.55 MB Transfer Rate: 11.93 MB/Sec
```
For multiple thread, QPS linear growth (when under 1000 concurrency, use 1.5 core(150%), it comes to 150k QPS).

For more test data look https://study.fifseason.top/2026/02/07/easysv-test/

## Implementation Details

For an in-depth look at the implementation, please visit:  
https://study.fifseason.top/2026/01/22/easysv-log/

## Build

Run the following commands in the project root directory:

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```
if you want to install it:
```bash
sudo make install
```
unistall:
```bash
sudo xargs rm -v < build/install_manifest.txt
```

## Announcement

This library can be used as a reference for learning or to conveniently build small projects.  
There are still many shortcomings in the code.   
Do not use this project in actual large-scale projects.

---

# 中文版请读我！

1. [本项目是什么?](#本项目是什么)
2. [如何应用](#如何应用)
3. [注意事项](#注意事项)
4. [基准测试](#基准测试)
5. [具体实现](#具体实现)
6. [构建](#构建)
7. [免责声明](#免责声明)
8. [let's speak English](#english-version-readme)


## 本项目是什么？

easyServerCore是一个TCP套接字服务器，基于较简单的**多线程epoll+协程**架构，可以使用本项目提供的框架或接口，在上层编写简单的网络应用。

仅支持linux。在 Debian 6.12.63-1 x86_64 上完成测试。

## 如何应用

引入头文件`easysv/Coro_scheduler.h`。  

使用`easysv::task coro_func_name(easysv::Coro_scheduler& sched, int fd)`，定义一个协程函数，并编写你的应用逻辑。  
使用`co_await easysv::Awaitable{sched, fd, next_epoll_event}`，指出你的协程需要等待什么事件。  
使用`co_return`表示完成一个协程。  

**用户不需要考虑协程句柄的生命周期管理。**  

创建一个服务器实例：
```cpp
#include <easysv/Server.h>

easysv::Server s(port, listen_sock_queue_size);
```
创建一个参数结构体并设置值：
```cpp
#include <easysv/config.h>

struct Setting userset
{
    //the size of listen sock queue
    int LISTENQ;
    //each time thread get nums of fd when accept
    size_t EACH_ACCEPT_NUM;
    //the max nums of fd epoll wait return
    size_t EventArraySize;
    //epoll mod -- LT(0) or ET(EPOLLET)
    uint32_t EPOLLMOD;
};
```  
创建一个任务信息结构体：  
```cpp
#include <easysv/Types.h>

easysv::Task_type your_task
{
    //your app corotinue_function
    .task_template = coro_func_name,
    //your first event to wait
    .initial_care_event = EPOLL_EVENTS
};
```
初始化服务器：
```cpp
s.init(
    thread_nums,
    &your_task,
    &userset
)  
```
运行服务器：
```cpp
s.run();
```
>参考 /app/mainWebServer.cc

## 注意事项

1) 在`struct Setting`的`EPOLLMOD`设置中，使用`0`指定LT模式，`EPOLLET`指定ET模式。
2) 该框架不负责对 `epoll ET 模式`下的正确性保证，使用者须注意检查。  
3) 该框架为事件驱动 + 协同例程，不包含零拷贝技术。  
4) 注意你的进程文件描述符限制（关系到能承载多少并发），linux默认1024。  
5) 框架使用 `SO_REUSEPORT`, 对于系统不支持的使用者（linux内核版本早于3.9的）可使用分支v1中的版本一。

## 基准测试

（排除网络延迟/磁盘IO时）：

1000并发下（用满1核）：
```
kktori@kotori ~ 
❯ rewrk -d 20s -h http://192.168.100.21:19198/ -t 10 -c 1000  
Beginning round 1...
Benchmarking 1000 connections @ http://192.168.100.21:19198/ for 20 second(s)
  Latencies:
    Avg      Stdev    Min      Max      
    9.85ms   1.03ms   0.11ms   102.38ms  
  Requests:
    Total: 2021291 Req/Sec: 101382.89
  Transfer:
    Total: 248.67 MB Transfer Rate: 12.47 MB/Sec

```

2000并发下：
```
kktori@kotori ~ 
❯ rewrk -d 20s -h http://192.168.100.21:19198/ -t 10 -c 2000 
Beginning round 1...
Benchmarking 2000 connections @ http://192.168.100.21:19198/ for 20 second(s)
  Latencies:
    Avg      Stdev    Min      Max      
    20.54ms  2.25ms   0.03ms   169.58ms  
  Requests:
    Total: 1930922 Req/Sec: 96998.91
  Transfer:
    Total: 237.55 MB Transfer Rate: 11.93 MB/Sec

```
对于多线程，可以预计QPS在一定程度内线性增长。（1000并发下，使用1.5核达到150k QPS）。

详细数据见  https://study.fifseason.top/2026/02/07/easysv-test/

## 具体实现

参阅   https://study.fifseason.top/2026/01/22/easysv-log/

## 构建

在项目根目录：

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```
如果你想安装它：
```bash
sudo make install
```
卸载:
```bash
sudo xargs rm -v < build/install_manifest.txt
```

## 免责声明

本库可供学习参考，或便利地搭建中小型项目。代码尚有许多不足之处。请勿将本项目用于实际大工程中。
