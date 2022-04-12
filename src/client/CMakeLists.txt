cmake_minimum_required(VERSION 3.21)
project(Client)

set(CMAKE_CXX_STANDARD 20)

CPMAddPackage(
        NAME enet
        GITHUB_REPOSITORY lsalzman/enet
        GIT_TAG v1.3.17
)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(network_client main.cpp Client.cpp EnetClient.cpp)
target_link_libraries(network_client PRIVATE Threads::Threads enet)
target_include_directories(network_client
        PUBLIC
        $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>)