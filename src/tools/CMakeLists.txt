cmake_minimum_required(VERSION 3.22)

if(CMAKE_OPTIMIZE MATCHES "O3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS "-O3")
elseif(CMAKE_OPTIMIZE MATCHES "O2")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    set(CMAKE_CXX_FLAGS "-O2")
elseif(CMAKE_OPTIMIZE MATCHES "O1")
    set(CMAKE_CXX_FLAGS_RELEASE "-O1")
    set(CMAKE_CXX_FLAGS "-O1")
elseif(CMAKE_OPTIMIZE MATCHES "None")
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ../utils)

add_library(Tools OBJECT
        config.cpp
        log_file.cpp
        logg.cpp)

target_link_directories(Tools PUBLIC
        "../utils")

set_property(TARGET Tools PROPERTY CXX_STANDARD 20)