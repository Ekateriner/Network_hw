cmake_minimum_required(VERSION 3.21)
project(Client)

set(CMAKE_CXX_STANDARD 20)

CPMAddPackage(
        NAME enet
        GITHUB_REPOSITORY lsalzman/enet
        GIT_TAG v1.3.17
)

CPMAddPackage(
        NAME allegro
        GITHUB_REPOSITORY liballeg/allegro5
        GIT_TAG 5.2.7.0
        OPTIONS
        # Ah, yes, I want my build process to take 10 minutes by default, thank you!
        "WANT_DOCS off"
        "WANT_EXAMPLES off"
        "WANT_DEMO off"
        "WANT_TESTS off"
        "WANT_VIDEO off"
        "WANT_PHYSFS off"
        "WANT_IMAGE_WEBP off"
        "WANT_AUDIO off"
)

if(allegro_ADDED)
    # allegro's authors have no idea how to write proper cmake files
    target_include_directories(allegro INTERFACE
            "${allegro_SOURCE_DIR}/include"
            "${allegro_BINARY_DIR}/include"
            )

    foreach(ADDON font image color primitives)
        target_include_directories("allegro_${ADDON}" INTERFACE
                "${allegro_SOURCE_DIR}/addons/${ADDON}"
                )
    endforeach()
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(network_client main.cpp Client.cpp EnetClient.cpp GameClient.cpp)
target_link_libraries(network_client PRIVATE Threads::Threads enet allegro allegro_font allegro_primitives)
target_include_directories(network_client
        PUBLIC
        $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>)