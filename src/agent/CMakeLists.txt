cmake_minimum_required(VERSION 3.21)
project(Lobby)

set(CMAKE_CXX_STANDARD 20)

CPMAddPackage(
        NAME enet
        GITHUB_REPOSITORY lsalzman/enet
        GIT_TAG v1.3.17
)
add_executable(network_agent main.cpp EnetAgent.cpp)
target_link_libraries(network_agent enet)
target_include_directories(network_agent
        PUBLIC
        $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>)