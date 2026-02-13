#include <unistd.h>
#include <errno.h>
#include <string>
#include <cstring>
#include <iostream>
#include <sys/socket.h>

#include "../include/Server.h"
#include "../include/config.h"
#include "../include/Coro_scheduler.h"
#include "../include/Types.h"

using namespace easysv;

static std::string make_http_response(const std::string& body, bool keep_alive)
{
    std::string resp;
    resp += "HTTP/1.1 200 OK\r\n";
    resp += "Content-Type: text/plain\r\n";
    resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    if (keep_alive)
        resp += "Connection: keep-alive\r\n";
    else
        resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;
    return resp;
}

easysv::coro_t http_coro(easysv::Coro_scheduler& sched, int fd)
{
    constexpr size_t BUF_SIZE = 4096;
    char buf[BUF_SIZE];

    std::string inbuf;
    std::string outbuf;

    bool keep_alive = true;

    while (true)
    {
        while (true)
        {
            ssize_t n = ::read(fd, buf, BUF_SIZE);
            if (n > 0)
            {
                inbuf.append(buf, n);

                // HTTP 头部结束
                if (inbuf.find("\r\n\r\n") != std::string::npos)
                    break;
            }
            else if (n == 0)
            {
                // 客户端关闭连接
                co_return;
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // 等待读事件
                    co_await Awaitable(sched, fd, EPOLLIN);
                    continue;
                }
                else
                {
                    // socket 错误
                    co_return;
                }
            }
        }

        // 简单http解析
        if (inbuf.find("Connection: close") != std::string::npos ||
            inbuf.find("connection: close") != std::string::npos)
        {
            keep_alive = false;
        }

        // 处理请求行
        std::string body = "Hello from easysv coroutine web server!\n";
        outbuf = make_http_response(body, keep_alive);

        inbuf.clear();

        //写
        size_t written = 0;
        while (written < outbuf.size())
        {
            ssize_t n = ::write(fd, outbuf.data() + written, outbuf.size() - written);
            if (n > 0)
            {
                written += n;
            }
            else if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    co_await Awaitable(sched, fd, EPOLLOUT);
                    continue;
                }
                else
                {
                    // socket 错误
                    co_return;
                }
            }
        }

        outbuf.clear();

        if (!keep_alive)
        {
            co_return;
        }

        // HTTP keep-alive
    }
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cout << "usage: webserver [port]" << '\n';
        exit(-1);
    }
    int port = atoi(argv[1]);

    Server server(port);

    Task_type web_task;
    web_task.task_template = http_coro;
    web_task.initial_care_event = EPOLLIN;

    //just a test , may not a good config set
    struct Setting set 
    {
        .LISTENQ = 40,
        .EACH_ACCEPT_NUM = 5,
        .EventArraySize = 50,
        .EPOLLMOD = EPOLLET
    };
    server.init(3, &web_task, &set);
    server.run();

}