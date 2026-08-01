#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <cerrno>
#include "server.h"

constexpr int PORT = 8080;
constexpr int MAX_EVENTS = 64;

int main(){
    int server_fd = initServer(PORT);
    if (server_fd == -1){
        std::cerr << "Error of initialize server\n";
        return 1;
    }

    int epfd = epoll_create1(0);
    if (epfd < 0){
        std::cerr << "error epoll_create1: " << std::system_error(errno, std::generic_category()).what() << '\n';
        close(server_fd);
        return 1;
    }

    epoll_event server_ev{};
    server_ev.events = EPOLLIN | EPOLLET;
    server_ev.data.fd = server_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &server_ev) < 0){
        printLog("error add server_fd to EPOLL");
        close(epfd);
        close(server_fd);
        return 1;
    }

    std::cout << "[!!!] server start work with epoll Event Loop on http:\x2F/localhost:8080\n";

    epoll_event events[MAX_EVENTS] = {0};
    while (true){
        int nefd = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nefd < 0){
            if (errno == EINTR) continue;
            printLog("error in epoll_wait");
            break;
        }
        for (size_t i = 0; i < nefd; i++){
            int current_fd = events[i].data.fd;
            if (current_fd == server_fd){
                processNewConnections(server_fd, epfd);
            }
            else {
                processNewData(current_fd);
            }
        }
    }

    close(epfd);
    close(server_fd);
    return 0;
}
