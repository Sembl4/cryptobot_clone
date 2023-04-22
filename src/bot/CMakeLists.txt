cmake_minimum_required(VERSION 3.22)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ../utils
        ../tools
        ../client
        ../init
        ../markets
        ../readers
        ../requests
        ../binance
        /usr/local/include
        /usr/local/include/hv)

add_library(Bot
        cryptobot.cpp)

target_link_directories(Bot PUBLIC
        "../utils"
        "../tools"
        "../client"
        "../init"
        "../markets"
        "../readers"
        "../requests"
        "../binance"
        "/usr/local/include"
        "/usr/local/include/hv")

set_property(TARGET Bot PROPERTY CXX_STANDARD 20)