#include "Tpool.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/eventfd.h>
#include <iostream>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include "../include/config.h"

using namespace easysv;

static int tcpsv_socket()
{
    int listen_sock_fd;
    if((listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        spdlog::error("failed to create sock");        
        throw std::runtime_error("socket error");
    }
    return listen_sock_fd;
}

static void tcpsv_bind(int fd, struct sockaddr_in* servaddr)
{
    if((bind(fd, reinterpret_cast<struct sockaddr*>(servaddr), sizeof(*servaddr))) < 0)
    {
        spdlog::error("failed to bind sock with servaddr");
        close(fd);
        throw std::runtime_error("bind error");
    }
}

static void tcpsv_listen(int listen_sock_fd)
{
    if((listen(listen_sock_fd, g_config.LISTENQ)) < 0)
    {
        spdlog::error("failed to set listen sock");
        close(listen_sock_fd);
        throw std::runtime_error("listen error");
    }
    //set non-block listen_fd
    int flags = fcntl(listen_sock_fd, F_GETFL, 0);
    if(flags == -1 || fcntl(listen_sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        spdlog::error("Server::tcpsv_accept: fcntl error");
        close(listen_sock_fd);
        throw std::system_error(errno, std::system_category(), "fcntl error");
    }
}

Tpool::Tpool(int thread_num, Task_type ttaskt, struct sockaddr_in* servaddr):
thread_taskt(ttaskt), stopping(false)
{ 
    wthreads.reserve(thread_num);
    //设置同端口、独立监听FD,以及作为通知作用的eventfd,并分发给线程
    for (int id = 0; id < thread_num; ++id)
    {
        int listen_sock_fd = tcpsv_socket();

        int opt = 1;
        setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
        setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
        tcpsv_bind(listen_sock_fd, servaddr);
        tcpsv_listen(listen_sock_fd);

        wthreads.emplace_back(std::make_unique<easysv::WorkT>(
            eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC), //设置非阻塞
            listen_sock_fd,
            thread_taskt,
            id,
            stopping
        ));
    }
    spdlog::info("main thread: thread pool ready!");  
}

Tpool::~Tpool()
{ }

void Tpool::shutdown()
{
    spdlog::info("main thread: close server...");
    stopping.store(true, std::memory_order_relaxed);
    //唤醒并关闭线程
    for(auto& t : wthreads) {
        uint64_t one = 1;
        ssize_t s;
        do {
            s = write(t->notify_fd, &one, sizeof(one));
        } while (s == -1 && errno == EINTR);
        if (s == -1) 
        {
            spdlog::error("failed to write eventfd for thread: {}", strerror(errno));
        }
    }
}
