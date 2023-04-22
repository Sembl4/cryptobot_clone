cmake_minimum_required(VERSION 3.12)
project(my_project)


set(CMAKE_CXX_STANDARD 20)

# Flags
if(CMAKE_BUILD_TYPE MATCHES "Debug")
	add_definitions(-DDEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
	add_definitions(-DRELEASE)
endif()

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

add_definitions(-DTIMESTAMP)

add_subdirectory(src/tools)
add_subdirectory(src/init)
add_subdirectory(src/client)
add_subdirectory(src/markets)
add_subdirectory(src/readers)
add_subdirectory(src/requests)
add_subdirectory(src/binance)
add_subdirectory(src/bot)


find_package(OpenSSL REQUIRED)


include_directories(
		/usr/local/include
		/usr/local/include/hv
		/usr/local/include/boost/include
		/usr/local/include/boost/lib
		${OPENSSL_INCLUDE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/src/utils
		${PROJECT_SOURCE_DIR}/src/tools
		${PROJECT_SOURCE_DIR}/src/client
		${PROJECT_SOURCE_DIR}/src/init
		${PROJECT_SOURCE_DIR}/src/markets
		${PROJECT_SOURCE_DIR}/src/readers
		${PROJECT_SOURCE_DIR}/src/requests
		${PROJECT_SOURCE_DIR}/src/binance
		${PROJECT_SOURCE_DIR}/src/bot)


add_executable(${PROJECT_NAME} src/main.cpp)


target_link_libraries(${PROJECT_NAME} PRIVATE
		Bot
		Binance
		Request
		Market
		Reader
		Init
		Client
		Tools
		/usr/local/lib/libhv.a
		${OPENSSL_LIBRARIES})


target_link_directories(${PROJECT_NAME} PUBLIC
		"${PROJECT_BINARY_DIR}"
		"${PROJECT_SOURCE_DIR}/src/tools"
		"${PROJECT_SOURCE_DIR}/src/markets"
		"${PROJECT_SOURCE_DIR}/src/readers"
		"${PROJECT_SOURCE_DIR}/src/requests"
		"${PROJECT_SOURCE_DIR}/src/binance"
		"${PROJECT_SOURCE_DIR}/src/bot"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/client"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/init"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/markets"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/readers"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/requests"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/binance"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/bot"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/utils")
