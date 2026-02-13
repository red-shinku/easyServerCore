#!/bin/bash
echo "compling..."
g++ mainWebServer.cc ../src/Server.cc ../src/Tpool.cc ../src/WorkT.cc ../src/Coro_scheduler.cc ../src/Epoll.cc -std=c++20 -fcoroutines -lspdlog -lfmt -o webserver

echo "done."
