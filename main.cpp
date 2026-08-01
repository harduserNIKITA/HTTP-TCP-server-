#include <iostream>
#include "server.h"

int main(){
    try {
        Server server(8080);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
