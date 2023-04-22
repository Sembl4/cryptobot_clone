cmake_minimum_required(VERSION 3.22)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ../utils
        ../tools
        ../markets
        ../client
        /usr/local/include
        /usr/local/include/hv)

add_library(Request OBJECT
        request_data.cpp
        # doOrdersClass.cpp
        rollBack.cpp)

target_link_directories(Request PUBLIC
        "../utils"
        "../tools"
        "../markets"
        "../client"
        "/usr/local/include"
        "/usr/local/include/hv")

set_property(TARGET Request PROPERTY CXX_STANDARD 20)