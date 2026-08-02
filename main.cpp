#include <iostream>
#include <atomic>
#include <csignal>
#include "server.h"

std::atomic<bool> g_stop_server{false};

void signalHandler(int signal){
    if (signal == SIGINT || signal == SIGTERM){
        g_stop_server = true;
    }
}

int main(){
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    try {
        Server server(8080);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
