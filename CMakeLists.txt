cmake_minimum_required(VERSION 3.10)

project(httpServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(СMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(server
	main.cpp
	http_request.cpp
    server.cpp
    thread_pool.cpp
)

target_link_libraries(server PRIVATE Threads::Threads)
