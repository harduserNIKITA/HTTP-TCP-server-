cmake_minimum_required(VERSION 3.14)

project(httpServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(СMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -g)

find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(server
	main.cpp
	http_request.cpp
    server.cpp
    thread_pool.cpp
)

target_link_libraries(server PRIVATE Threads::Threads)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(run_tests
    tests/test_main.cpp
    http_request.cpp
    thread_pool.cpp
)

target_link_libraries(run_tests PRIVATE gtest_main Threads::Threads)

include(GoogleTest)
gtest_discover_tests(run_tests)
