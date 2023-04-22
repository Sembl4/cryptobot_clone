cmake_minimum_required(VERSION 3.22)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ../utils
        ../tools
        ../client
        ../init
        ../markets
        ../requests
        /usr/local/include
        /usr/local/include/hv)

add_library(Binance
        algorithms.cpp
        triangle.cpp
        doOrdersClass.cpp
        )

target_link_directories(Binance PUBLIC
        "../utils"
        "../tools"
        "../client"
        "../init"
        "../markets"
        "../requests"
        "/usr/local/include"
        "/usr/local/include/hv")

set_property(TARGET Binance PROPERTY CXX_STANDARD 20)