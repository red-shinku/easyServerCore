#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include "config.h"

namespace easysv 
{
class Tpool;
class Epoll;
class Task_type;

class Server
{
private:
    struct sockaddr_in servaddr;
    int signal_fd;
    //thread pool to handle connected sock
    easysv::Tpool* tpool;
    //to tell listen_sock and some system signal
    easysv::Epoll* main_epoll;

public:
    explicit Server(const char* ip, int port);
    ~Server() noexcept;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void init(int thread_num, easysv::Task_type* APP, struct Setting*);
    void run();

};

}

