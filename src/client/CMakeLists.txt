cmake_minimum_required(VERSION 3.22)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ../utils
        ../tools
        /usr/local/include
        /usr/local/include/hv)


add_library(Client
        binanceClient.cpp
        cryptoClient.cpp
        OKX_Client.cpp)

target_link_directories(Client PUBLIC
        "../utils"
        "../tools"
        "/usr/local/include"
        "/usr/local/include/hv")

set_property(TARGET Client PROPERTY CXX_STANDARD 20)