#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include "Tpool.h"
#include "WorkT.h"

namespace easysv 
{

class Server
{
private:
    struct sockaddr_in servaddr;
    //queue size of listen_sock
    int LISTENQ;
    int listen_sock_fd;
    //thread pool to handle connected sock
    easysv::Tpool* tpool;

    void tcpsv_socket();
    void tcpsv_bind();
    void tcpsv_listen();
    int tcpsv_accept();

public:
    explicit Server(int port, int listen_queue_size);
    ~Server() noexcept;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void init(int thread_num, easysv::Task_type APP);
    void run();

};

}

