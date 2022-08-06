cmake_minimum_required(VERSION 3.2.0)

project(wordle)

add_executable(main "main.cpp")

if (UNIX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(main Threads::Threads )
endif (UNIX)
