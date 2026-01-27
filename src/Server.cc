#include "../include/Server.h"

#include <sys/signalfd.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <spdlog/spdlog.h>

#include "../src/Tpool.h"
#include "../src/Epoll.h"

using namespace easysv;

Setting g_config;

//TODO: 调整日志输出

Server::Server(int port, int listen_queue_size):
LISTENQ(listen_queue_size), tpool(NULL)
{
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //handle all ip on this os
    servaddr.sin_port = htons(port);
}

Server::~Server()
{
    delete tpool;
    close(listen_sock_fd);
    close(signal_fd);
}

void Server::tcpsv_socket()
{
    if((listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        spdlog::error("failed to create sock");        
        throw std::runtime_error("socket error");
    }
}

void Server::tcpsv_bind()
{
    if((bind(listen_sock_fd, reinterpret_cast<struct sockaddr*>(&servaddr), sizeof(servaddr))) < 0)
    {
        spdlog::error("failed to bind sock with servaddr");
        throw std::runtime_error("bind error");
    }
}

void Server::tcpsv_listen()
{
    if((listen(listen_sock_fd, LISTENQ)) < 0)
    {
        spdlog::error("failed to set listen sock");
        throw std::runtime_error("listen error");
    }
    //set non-block listen_fd
    int flags = fcntl(listen_sock_fd, F_GETFL, 0);
    if(flags == -1 || fcntl(listen_sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        spdlog::error("Server::tcpsv_accept: fcntl error");
        throw std::system_error(errno, std::system_category(), "fcntl error");
    }
}

int Server::tcpsv_accept()
{
    int connfd = 0;
    static struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    while (true) 
    {
        connfd = accept(listen_sock_fd, (struct sockaddr*)&client_addr, &client_len);
        if (connfd >= 0) break;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -1; // no more
        if (errno == EINTR) continue; // retry
        spdlog::error("accept failed: {}", strerror(errno));
        return -1;
    }
    //set non-block I/O
    int flags = fcntl(connfd, F_GETFL, 0);
    if(flags == -1 || fcntl(connfd, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        spdlog::error("Server::tcpsv_accept: fcntl error");
        throw std::system_error(errno, std::system_category(), "fcntl error");
        close(connfd);
        return -1;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    spdlog::info("connection: {}, new sock: {}", client_ip, connfd);
    return connfd; 
}

void Server::init(int thread_num, easysv::Task_type APP, struct Setting* userset)
{
    try
    {
        g_config = *userset;

        listen_epoll.init();

        int opt = 1;
        setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
        setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif

        tcpsv_socket();
        tcpsv_bind();
        tcpsv_listen();
        tpool = new Tpool(thread_num, APP);

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);

        if(sigprocmask(SIG_BLOCK, &mask, NULL) == -1) 
        {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }
        signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
        if(signal_fd == -1) {
            perror("signalfd");
            exit(EXIT_FAILURE);
        }
        listen_epoll.register_fd(signal_fd, EPOLLIN);
        listen_epoll.register_fd(listen_sock_fd, EPOLLIN);
    }
    catch(const std::system_error& e)
    {
        spdlog::error("In create Tpoll");
        std::cerr << e.what() << '\n';
    }
    catch(const std::bad_alloc& e)
    {
        spdlog::error("In create Tpool");
        std::cerr << e.what() << '\n';
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Server::run()
{
    // signal(SIGINT, when_sig_stop);
    // signal(SIGTERM, when_sig_stop);
    bool sv_stop = false;
    while (! sv_stop) 
    {
        try
        {
            auto [fds, num] = listen_epoll.wait();
            for(int i = 0; i < num; ++i)
            {
                if(fds[i].first == listen_sock_fd)
                {
                    int connfd;
                    while ((connfd = tcpsv_accept()) > 0)
                        tpool->accept_and_notice_thread(connfd);
                }
                else if(fds[i].first == signal_fd)
                {
                    struct signalfd_siginfo fdsi;
                    ssize_t s = read(signal_fd, &fdsi, sizeof(fdsi));
                    if (s == sizeof(fdsi)) 
                    {
                        sv_stop = true;
                    }
                }
            }
        }
        catch(const std::system_error& e)
        {
            std::cerr << e.what() << '\n';
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}
