#include "Server.h"

#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <spdlog/spdlog.h>

using namespace easysv;

static bool sv_stop = false;

static void when_sig_stop(int)
{
    sv_stop = true;
}

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
}

int Server::tcpsv_accept()
{
    int connfd = 0;
    if((connfd = accept(listen_sock_fd, NULL, NULL)) < 0)
    {
        spdlog::error("failed to accept connect sock");
        throw std::runtime_error("accept error");
    }
    //set non-block I/O
    int flags = fcntl(connfd, F_GETFL, 0);
    if(flags == -1 || fcntl(connfd, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        spdlog::error("Server::tcpsv_accept: fcntl error");
        throw std::system_error(errno, std::system_category(), "fcntl error");
    }
    spdlog::info("accept new sock: {}", connfd);
    return connfd; 
}

void Server::init(int thread_num, easysv::Task_type APP)
{
    try
    {
        tcpsv_socket();
        tcpsv_bind();
        tcpsv_listen();
        tpool = new Tpool(thread_num, APP);
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
    signal(SIGINT, when_sig_stop);
    signal(SIGTERM, when_sig_stop);

    while (! sv_stop) 
    {
        try
        {
            tpool->accept_and_notice_thread(tcpsv_accept());
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << e.what() << '\n';
        }
        catch(const std::system_error& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}
