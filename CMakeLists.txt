cmake_minimum_required(VERSION 3.21)
project(Mipt_network)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src/client)
add_subdirectory(src/server)