#pragma once
#include <string>

void printLog(const std::string& log);
bool setNonblocking(int fd);

int initServer(int port);

void sendHttpResponse(const std::string& rawRequest, int client_fd);

void processNewConnections(int server_fd, int epfd);
void processNewData(int client_fd);
