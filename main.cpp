#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <thread>
#include <chrono>
#include <system_error>
#include "http_request.h"

constexpr int MAX_EVENTS = 64;

bool setNonblocking(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1){
		perror("Error with F_GETFL");
		return false;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
		perror("Error with F_GETFL");
		return false;
	}
	return true;
}

int main(){
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0){
		std::cerr << "Socket-server not created\n";
		return 1;
	}

	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
		std::cerr << "Can't reuse address\n";
		close(server_fd);
		return 1;
	}

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8080);
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
		std::cerr << "Can't link port with socket\n";
		close(server_fd);
		return 1;
	}

	if (listen(server_fd, 128) < 0){
		std::cerr << "Can't listen\n";
		close(server_fd);
		return 1;
	}

	if (!setNonblocking(server_fd)){
		std::cerr << "Server_fd FAILED to set NONBLOCK\n";
		close(server_fd);
		return 1;
	}

	int epfd = epoll_create1(0);
	if (epfd < 0){
		std::cerr << "error epoll_create1: " << std::system_error(errno, std::generic_category()).what() << '\n';
		close(server_fd);
		return 1;
	}

	epoll_event ev{};
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) < 0){
		std::cerr << "error add server_fd in EPOLL: " << std::system_error(errno, std::generic_category()).what() << '\n';
		close(epfd);
		close(server_fd);
		return 1;
	}

	std::cout << "[!!!] server start work with epoll Event Loop on http://localhost:8080\n";

	epoll_event events[MAX_EVENTS] = {0};
	while (true){
		int nefd = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if (nefd < 0){
			std::cerr << "error in epoll_wait" << std::system_error(errno, std::generic_category()).what() << '\n';
			break;
		}
		for (size_t i = 0; i < nefd; i++){
			int current_fd = events[i].data.fd;
			if (current_fd == server_fd){
				sockaddr_in client{};
                socklen_t len = sizeof(client);
                int client_fd = accept(server_fd, (struct sockaddr*)& client, &len);
                if (client_fd < 0){
					if (errno != EAGAIN && errno != EWOULDBLOCK){
						std::cerr << "error of client connection" << std::system_error(errno, std::generic_category()).what() << '\n';
					}
					continue;
                }

                if (!setNonblocking(client_fd)){
                    std::cerr << "Client_fd FAILED to set NONBLOCK\n";
                    close(client_fd);
                    continue;
                }

				epoll_event client_ev{};
				client_ev.events = EPOLLIN;
				client_ev.data.fd = client_fd;
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev) < 0){
					std::cerr << "error in add client_fd to EPOLL" << std::system_error(errno, std::generic_category()).what() << '\n';
					close(client_fd);
				}
				else {
					std::cout << "[+] new connection\n";
				}
			}
			else {
				char buffer[1024] = {0};
                ssize_t read_bytes = read(current_fd, buffer, sizeof(buffer) - 1);
                if (read_bytes < 0){
                    if (errno != EAGAIN && errno == EWOULDBLOCK){
                        std::cerr << "error read data" << std::system_error(errno, std::generic_category()).what() << '\n';
						close(current_fd);
                    }
                }
                else if (read_bytes == 0){
                    std::cout << "[-] client disconnecting before sending data, client_fd: " << current_fd << '\n';
					close(current_fd);
                }
                else {
                    std::string rawRequest(buffer, read_bytes);
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

                    std::string httpResponse = startLine + "\r\n" + "Content-Type: text/html; charset=UTF-8\r\n" + "Content-Length: " +
                    std::to_string(body.size()) + "\r\n" + "Connection: close\r\n\r\n" + body;
                    write(current_fd, httpResponse.c_str(), httpResponse.size());

					close(current_fd);
					std::cout << "[-] response and close client_fd: " << current_fd << '\n';
				}
			}
		}
	}

	close(epfd);
	close(server_fd);
	std::cout << "Server has shut down\n";
	return 0;
}
