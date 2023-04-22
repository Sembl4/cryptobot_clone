cmake_minimum_required(VERSION 3.22)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ../utils
        ../tools
        ../markets
        /usr/local/include
        /usr/local/include/hv
        ${OPENSSL_INCLUDE_DIR})

add_library(Reader
        stream_reader.cpp
        binance_reader.cpp
        okx_read.cpp)

target_link_directories(Reader PUBLIC
        "../utils"
        "../tools"
        "../markets")

set_property(TARGET Reader PROPERTY CXX_STANDARD 20)