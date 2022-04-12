cmake_minimum_required(VERSION 3.21)
project(Server)

set(CMAKE_CXX_STANDARD 20)

CPMAddPackage(
        NAME enet
        GITHUB_REPOSITORY lsalzman/enet
        GIT_TAG v1.3.17
)
add_executable(network_server main.cpp Server.cpp EnetServer.cpp GameServer.cpp)
target_link_libraries(network_server enet)
target_include_directories(network_server
        PUBLIC
        $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>)