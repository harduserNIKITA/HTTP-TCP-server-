#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cerrno>
#include <system_error>
#include "server.h"
#include "http_request.h"

constexpr int MAX_EVENTS = 64;
constexpr int BUFFER_SIZE = 4096;

Server::Server(int port, int cntThreads) : port(port), pool(cntThreads) {
    initServer();
}

Server::~Server(){
    if (epfd != -1) close(epfd);
    if (server_fd != -1) close(server_fd);
    std::cout << ".....[!][!] server shut down [!][!].....\n";
}

bool Server::setNonblocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1){
        printLog("Error with F_GETFL");
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
        printLog("Error with F_SETFL");
        return false;
    }
    return true;
}

void Server::printLog(const std::string& log){
    std::cerr << log << std::system_error(errno, std::generic_category()).what() << '\n';
}

void Server::initServer(){
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        throw std::runtime_error("Socket-server not created");
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        throw std::runtime_error("Can't reuse address");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        throw std::runtime_error("Can't link port with socket");
    }

    if (listen(server_fd, 128) < 0){
        throw std::runtime_error("Can't listen");
    }

    if (!setNonblocking(server_fd)){
        throw std::runtime_error("Can't set NONBLOCK");
    }

    epfd = epoll_create1(0);
    if (epfd < 0){
        throw std::runtime_error("Error epoll_create1:");
    }

    epoll_event server_ev{};
    server_ev.events = EPOLLIN | EPOLLET;
    server_ev.data.fd = server_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &server_ev) < 0){
        throw std::runtime_error("error add server_fd to EPOLL");
    }

    std::cout << "[!!!] server start work with epoll Event Loop on http:\x2F/localhost:8080\n";
}

void Server::reactivateSocket(int client_fd){
    epoll_event client_ev{};
    client_ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    client_ev.data.fd = client_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &client_ev) < 0){
        printLog("error reactivateSocket");
        close(client_fd);
    }
}

void Server::run(){
    epoll_event events[MAX_EVENTS] = {0};
    while (!g_stop_server){
        int nefd = epoll_wait(epfd, events, MAX_EVENTS, 1000);
        if (nefd <= 0){
            if (nefd == 0 || errno == EINTR) continue;
            printLog("error in epoll_wait");
            break;
        }
        for (size_t i = 0; i < nefd; i++){
            int current_fd = events[i].data.fd;
            if (current_fd == server_fd){
                processNewConnections();
            }
            else {
                pool.enqueue([this, current_fd]{this->processNewData(current_fd);});
            }
        }
    }
}

void Server::processNewConnections(){
    while (true){
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr*)& client, &len);
        if (client_fd < 0){
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            printLog("error of client connection");
            break;
        }

        if (!setNonblocking(client_fd)){
            std::cerr << "Client_fd FAILED to set NONBLOCK\n";
            close(client_fd);
            continue;
        }

        epoll_event client_ev{};
        client_ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        client_ev.data.fd = client_fd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev) < 0){
            printLog("error in add client_fd to EPOLL");
            close(client_fd);
        }
        else {
            std::cout << "[+] new connection\n";
        }
    }
}

void Server::processNewData(int client_fd){
    std::string fullRawRequest;
    char buffer[BUFFER_SIZE] = {0};
    bool client_closed = false;
    while (true){
        ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);
        if (read_bytes > 0){
            fullRawRequest.append(buffer, read_bytes);
        }
        else if (read_bytes == 0){
            std::cout << "[-] client disconnecting before sending data, client_fd: " << client_fd << '\n';
            client_closed = true;
            break;
        }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            if (errno == EINTR) continue;
            printLog("error read data");
            client_closed = true;
            break;
        }
    }

    if (client_closed){
        close(client_fd);
        return;
    }

    if (fullRawRequest.empty()){
        reactivateSocket(client_fd);
        std::cout << "[!] reactivate client_fd: " << client_fd << '\n';
        return;
    }

    bool shouldClose = sendHttpResponse(fullRawRequest, client_fd);
    if (shouldClose){
        close(client_fd);
        std::cout << "[-] response and close client_fd: " << client_fd << '\n';
    }
    else {
        reactivateSocket(client_fd);
        std::cout << "[!] response and reactivate client_fd: " << client_fd << '\n';
    }
}

bool Server::sendHttpResponse(const std::string& rawRequest, int client_fd){
    HttpRequest req = parseHttpRequest(rawRequest);

    std::cout << "method = " << req.method << " | path = " << req.path << " | version = " << req.version << "\n";

    bool shouldClose = false;
    auto it = req.headers.find("Connection");
    if (it != req.headers.end() && it->second == "close"){
        shouldClose = true;
    }

    std::string startLine;
    std::string body;
    if (req.path == "/"){
        startLine = "HTTP/1.1 200 OK";
        body = "<html><body><h1>Welcome to KOVSHIKOV HTTP-server!!!</h1><p>Main page</p></body></html>";
    }
    else if (req.path == "/about"){
        startLine = "HTTP/1.1 200 OK";
        body = "<html><body><h1>Page About Us</h1><p>Written on C++ within system call && epoll</p></body></html>";
    }
    else {
        startLine = "HTTP/1.1 404 Not Found";
        body = "<html><body><h1>Page Not Fout 404</h1></body></html>";
    }

    std::string connectionType = shouldClose ? "close" : "keep-alive";

    std::string httpResponse = startLine + "\r\n" + "Content-Type: text/html; charset=UTF-8\r\n" +
                               "Content-Length: " + std::to_string(body.size()) + "\r\n" +
                               "Connection: " + connectionType + "\r\n\r\n" + body;
    write(client_fd, httpResponse.c_str(), httpResponse.size());

    return shouldClose;
}
