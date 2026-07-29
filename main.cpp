#include "http_request.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

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

	std::cout << "Server has started working successfully\n";

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

	std::cout << "Server is listening\n";

	while (true){
		sockaddr_in client{};
        	socklen_t len = sizeof(client);
        	int client_fd = accept(server_fd, (struct sockaddr*)& client, &len);
        	if (client_fd < 0){
			std::cerr << "Error of connection client\n";
                	continue;
        	}

        	std::cout << "Client connecting\n";

		char buffer[2048] = {0};
        	ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);
        	if (read_bytes > 0){
                	std::string rawRequest(buffer, read_bytes);
			HttpRequest req = parseHttpRequest(rawRequest);

			std::cout << "method = " << req.method << " | path = " << req.path
			<< " | version = " << req.version << "\n";

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
			write(client_fd, httpResponse.c_str(), httpResponse.size());
        	}
		close(client_fd);
	}

	close(server_fd);
	std::cout << "Server has shut down\n";
	return 0;
}
