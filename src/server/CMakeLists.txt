cmake_minimum_required(VERSION 3.21)
project(Server)

set(CMAKE_CXX_STANDARD 20)

add_executable(network_server main.cpp Server.cpp)
