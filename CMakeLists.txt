cmake_minimum_required(VERSION 3.28)
project(scheduler)

set(CMAKE_CXX_STANDARD 20)

add_executable(scheduler main.cpp
        process.h
        scheduler.cpp
        scheduler.h
        util.h
        thread_pool.h
        thread_pool.cpp)
