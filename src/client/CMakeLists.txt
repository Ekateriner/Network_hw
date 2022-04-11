cmake_minimum_required(VERSION 3.21)
project(Client)

set(CMAKE_CXX_STANDARD 20)

add_executable(network_client main.cpp Client.cpp)
