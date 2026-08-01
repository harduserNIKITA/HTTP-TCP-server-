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

void printLog(const std::string& log){
    std::cerr << log << std::system_error(errno, std::generic_category()).what() << '\n';
}

bool setNonblocking(int fd){
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

int initServer(int port){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        printLog("Socket-server not created");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        printLog("Can't reuse address");
        close(server_fd);
        return -1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        printLog("Can't link port with socket");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 128) < 0){
        printLog("Can't listen");
        close(server_fd);
        return -1;
    }

    if (!setNonblocking(server_fd)){
        std::cerr << "Can't set NONBLOCK\n";
        close(server_fd);
        return -1;
    }
    return server_fd;
}

void processNewConnections(int server_fd, int epfd){
    while (true){
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr*)& client, &len);
        if (client_fd < 0){
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            printLog("error of client connection");
            continue;
        }

        if (!setNonblocking(client_fd)){
            std::cerr << "Client_fd FAILED to set NONBLOCK\n";
            close(client_fd);
            continue;
        }

        epoll_event client_ev{};
        client_ev.events = EPOLLIN | EPOLLET;
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

void sendHttpResponse(const std::string& rawRequest, int client_fd){
    HttpRequest req = parseHttpRequest(rawRequest);

    std::cout << "method = " << req.method << " | path = " << req.path << " | version = " << req.version << "\n";

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

    std::string httpResponse = startLine + "\r\n" + "Content-Type: text/html; charset=UTF-8\r\n" + "Content-Length: " + std::to_string(body.size()) + "\r\n" + "Connection: close\r\n\r\n" + body;
    write(client_fd, httpResponse.c_str(), httpResponse.size());
}

void processNewData(int client_fd){
    std::string fullRawRequest;
    char buffer[256] = {0};
    bool client_closed = false;
    while (true){
        ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);
        if (read_bytes > 0){
            fullRawRequest.append(buffer, read_bytes);
        }
        else if (read_bytes == 0){
            std::cout << "[-] client disconnecting before sending data, client_fd: " << client_fd << '\n';
            close(client_fd);
            client_closed = true;
            break;
        }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            printLog("error read data");
            close(client_fd);
            client_closed = true;
            break;
        }
    }

    if (client_closed || fullRawRequest.empty()){
        return;
    }

    sendHttpResponse(fullRawRequest, client_fd);
    close(client_fd);
    std::cout << "[-] response and close client_fd: " << client_fd << '\n';
}
