cmake_minimum_required(VERSION 3.22)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ../utils
        ../tools
        ../client)

add_library(Init
        init.cpp)

target_link_directories(Init PUBLIC
        "../utils"
        "../tools"
        "../client")

set_property(TARGET Init PROPERTY CXX_STANDARD 20)