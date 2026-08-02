#pragma once

#include <atomic>
#include <string>
#include "thread_pool.h"

extern std::atomic<bool> g_stop_server;

class Server{
public:
    explicit Server(int port, int cntThreads = std::thread::hardware_concurrency());

    void run();

    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

private:
    int server_fd{-1};
    int epfd{-1};
    int port;
    ThreadPool pool;

    void printLog(const std::string& log);
    bool setNonblocking(int fd);
    void initServer();

    bool sendHttpResponse(const std::string& rawRequest, int client_fd);

    void reactivateSocket(int client_fd);
    void processNewConnections();
    void processNewData(int client_fd);

    void closeClientSocket(int client_fd);
};
