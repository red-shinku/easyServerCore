# English version ReadMe

1. [What is this project?](#what-is-this-project)
2. [How to Use](#how-to-use)
    * [Setup Steps](#setup-steps)
3. [Implementation Details](#implementation-details)
4. [Build](#build)
5. [Announcement](#Announcement)
6. [让我们说中文](#中文版请读我)

## What is this project?

**easyServerCore** is a TCP socket server based on a straightforward **Multi-threaded epoll + Coroutines** architecture. You can use the framework and interfaces provided by this project to build simple network applications at the top level.

**Note:** Only Linux is supported. Testing was completed on Debian 6.12.63-1 x86_64.

## How to Use

Include the header files`easysv/Coro_scheduler.h`, and then:   

1) **Define your logic:** Use`easysv::task coro_func_name(easysv::Coro_scheduler& sched, int fd)`，to define a coroutine function.    
2) **Handle Events:** Use`co_await easysv::Awaitable{sched, fd, next_epoll_event}` to specify which events your coroutine should wait for.  
3) **Finish:** Use`co_return` to signal the completion of a coroutine.    

**The user does not need to worry about the lifecycle management of coroutine handles.**  

### Setup Steps

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
    //Number of connections a worker thread fetches per batch
    .EACH_FD_GET_NUM = //xx,
    //Size of the public FD queue (for pending connections)
    .PUB_FD_QUEUE_SIZE = //xx,
    // Threshold for the public FD queue to be considered "backlogged"
    .PUB_FD_QUEUE_CRITICAL = //xx,
    //Task threshold for a worker thread to be considered "idle"
    .IS_IDLE_NUM = //xx,
    //Maximum number of events returned per epoll_wait call
    .EventArraySize = //xx,
    //Epoll mode
    .EPOLLMOD = //like EPOLLET
};
```  
3) **Define task information:**
```cpp
#include <easysv/Types.h>

easysv::Task_type your_task
{
    .task_template = coro_func_name,
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
>参考 /app/mainWebServer.cc

## Implementation Details

For an in-depth look at the implementation, please visit:  
 https://study.fifseason.top/2026/01/22/%E5%A5%97%E6%8E%A5%E5%AD%97%E6%9C%8D%E5%8A%A1%E5%99%A8%E5%BC%80%E5%8F%91%E6%97%A5%E5%BF%97/

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
3. [具体实现](#具体实现)
4. [构建](#构建)
5. [声明](#声明)
6. [let's speak English](#english-version-readme)


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
    //工作线程单次获取的连接数
    .EACH_FD_GET_NUM = //xx,
    //公共fd（未开始处理）队列的大小
    .PUB_FD_QUEUE_SIZE = //xx,
    //公共fd队列进入过堆积状态的设定值（大于该数视为过堆积）
    .PUB_FD_QUEUE_CRITICAL = //xx,
    //工作线程进入空闲状态的任务数设定值（小于该数视为空闲）
    .IS_IDLE_NUM = //xx,
    //epoll wait每次返回的事件上限
    .EventArraySize = //xx,
    //epoll 模式
    .EPOLLMOD = //like EPOLLET
};
```  
创建一个任务信息结构体：  
```cpp
#include <easysv/Types.h>

easysv::Task_type your_task
{
    .task_template = coro_func_name,
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

## 具体实现

参阅   
https://study.fifseason.top/2026/01/22/%E5%A5%97%E6%8E%A5%E5%AD%97%E6%9C%8D%E5%8A%A1%E5%99%A8%E5%BC%80%E5%8F%91%E6%97%A5%E5%BF%97/

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

## 声明

本库可供学习参考，或便利地搭建小型项目。代码尚有许多不足之处。请勿将本项目用于实际大工程中。
