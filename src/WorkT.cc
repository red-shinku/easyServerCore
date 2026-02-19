#include "WorkT.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include "../include/config.h"

using namespace easysv;

WorkT::WorkT(int notify_fd, int listen_sock_fd, Task_type& taskt, 
            int id, std::atomic<bool>& stopping_flag):
notify_fd(notify_fd),
listen_sock_fd(listen_sock_fd),
taskt(taskt), 
epoll(), 
coro_sheduler(taskt.initial_care_event, listen_sock_fd, 
                [this] (int fd, EPOLL_EVENTS event, uint32_t mod) {
                    this->epoll.register_fd(fd, event, mod);
                }, 
                [this] (int fd, EPOLL_EVENTS event, uint32_t mod) {
                    this->epoll.change_fd_event(fd, event, mod);
                },
                [this] (int fd) {
                    this->epoll.deletefd(fd);
                }), 
id(id), 
stopping(stopping_flag)
{
    readylist.resize(g_config.EventArraySize);
    epoll.init();
    epoll.register_fd(listen_sock_fd, EPOLLIN, 0);
    epoll.register_fd(notify_fd, EPOLLIN, 0);
    worker = std::thread([this] { work(); });
    spdlog::info("create a thread");
}

WorkT::~WorkT()
{
    if(worker.joinable())
    {
        worker.join();
        close(listen_sock_fd);
        close(notify_fd);
        spdlog::info("delete a thread");
    }
    else
        spdlog::warn("failed to delete thread");
}

int WorkT::tcpsv_accept()
{
    int connfd = 0;
    socklen_t client_len = sizeof(client_addr);
    while (true) 
    {
        connfd = accept(listen_sock_fd, (struct sockaddr*)&client_addr, &client_len);
        if (connfd >= 0) break;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -1; // no more
        if (errno == EINTR) continue; // retry
        spdlog::error("accept failed: {}", strerror(errno));
        throw std::runtime_error("error at tcpsv_accept");
        return -1;
    }
    //set non-block I/O
    int flags = fcntl(connfd, F_GETFL, 0);
    if(flags == -1 || fcntl(connfd, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        spdlog::error("Server::tcpsv_accept: fcntl error");
        close(connfd);
        throw std::system_error(errno, std::system_category(), "fcntl error");
        return -1;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    spdlog::info("connection: {}, new sock: {}", client_ip, connfd);
    return connfd;
}

void WorkT::work()
{
    while (! stopping.load(std::memory_order_relaxed))
    {
        auto [list, rdnum] = epoll.wait(std::move(readylist));
        readylist = list;
        //优先注册新连接，并检查
        for(int i = 0; i < rdnum; ++i)
        {
            if(readylist[i].first == listen_sock_fd)
            {
                try
                {
                    for(int j = 0; j < g_config.EACH_ACCEPT_NUM; ++j)
                    {
                        int connfd;
                        if((connfd = tcpsv_accept()) > 0)
                            coro_sheduler.register_coro(connfd, taskt.task_template);
                        else
                            break;
                    }
                }
                catch(const std::system_error& e)
                {
                    std::cerr << e.what() << '\n';
                }
                catch(const std::runtime_error& e)
                {
                    std::cerr << e.what() << '\n';
                    //TODO: 过载策略：一个标志位在下一轮跳过accept
                }              
                break;
            }
            else if (readylist[i].first == notify_fd) 
            {
                uint64_t v;
                while (true) {
                    ssize_t n = read(notify_fd, &v, sizeof(v));
                    if(n == -1) {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                        if(errno == EINTR) continue;
                        spdlog::error("read notify_fd failed: {}", strerror(errno));
                        break;
                    }
                    break;
                }
            }
        }
        //调度协程
        coro_sheduler.run(&readylist, rdnum);
    }
    spdlog::info("bye bye");
}
