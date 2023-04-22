cmake_minimum_required(VERSION 3.22)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ../utils
        ../tools
        ../client
        ../init)

add_library(Market
        market.cpp
        quote.cpp)

target_link_directories(Market PUBLIC
        "../utils"
        "../tools"
        "../client"
        "../init")

set_property(TARGET Market PROPERTY CXX_STANDARD 20)