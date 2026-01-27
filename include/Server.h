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
    //queue size of listen_sock
    int LISTENQ;
    int listen_sock_fd;
    int signal_fd;
    //thread pool to handle connected sock
    easysv::Tpool* tpool;
    //to tell listen_sock and some system signal
    easysv::Epoll listen_epoll;

    void tcpsv_socket();
    void tcpsv_bind();
    void tcpsv_listen();
    int tcpsv_accept();

public:
    explicit Server(int port, int listen_queue_size);
    ~Server() noexcept;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void init(int thread_num, easysv::Task_type APP, struct Setting*);
    void run();

};

}

