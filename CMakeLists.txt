cmake_minimum_required(VERSION 3.29)
project(cache_benchmark)

set(CMAKE_CXX_STANDARD 20)

add_executable(cache_benchmark main.cpp
        page_traverser.cpp
        mem_utils.cpp)

target_compile_definitions(cache_benchmark PUBLIC -DVERBOSE)