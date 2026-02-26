#include "../include/Server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

Server::Server(const char* ip, int port):
tpool(NULL)
{
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");
    //FIXME: 开启文件日志
    bzero(&servaddr, sizeof(servaddr));
    if(inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0)
    {
        perror("invailed IP address!!!");
        exit(-1);
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //handle all ip on this os
    servaddr.sin_port = htons(port);
}

Server::~Server()
{
    delete tpool;
    delete main_epoll;
    close(signal_fd);
}

void Server::init(int thread_num, easysv::Task_type* APP, struct Setting* userset)
{
    try
    {
        g_config = *userset;

        main_epoll = new Epoll();
        main_epoll->init();

        tpool = new Tpool(thread_num, *APP, &servaddr);

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
        main_epoll->register_fd(signal_fd, EPOLLIN, 0);
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
    bool sv_stop = false;
    while (! sv_stop) 
    {
        try
        {
            auto [fds, num] = main_epoll->wait();
            for(int i = 0; i < num; ++i)
            {
                if(fds[i].first == signal_fd)
                {
                    struct signalfd_siginfo fdsi;
                    ssize_t s = read(signal_fd, &fdsi, sizeof(fdsi));
                    if (s == sizeof(fdsi)) 
                    {
                        sv_stop = true;
                        tpool->shutdown();
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
