cmake_minimum_required(VERSION 3.29)
project(cache_benchmark)

set(CMAKE_CXX_STANDARD 20)

add_executable(cache_benchmark main.cpp
        page_traverser.cpp
        page_traverser.h
        clear_memory.cpp
        clear_memory.h)

target_compile_definitions(cache_benchmark PUBLIC -DVERBOSE)