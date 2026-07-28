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

	sockaddr_in client{};
	socklen_t len = sizeof(client);
	int client_fd = accept(server_fd, (struct sockaddr*)& client, &len);
	if (client_fd < 0){
		std::cerr << "Not connect with client\n";
		close(server_fd);
		return 1;
	}

	std::cout << "Client connecting\n";

	char buffer[1024] = {0};
	ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);
	if (read_bytes > 0){
		std::cout << "Data from client:\n";
		std::cout << "--------------------\n";
		std::cout << buffer << "\n";
		std::cout << "--------------------\n";
	}

	close(server_fd);
	close(client_fd);
	std::cout << "Server has shut down\n";
	return 0;
}
