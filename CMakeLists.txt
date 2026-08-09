cmake_minimum_required(VERSION 3.15)
project(NetConnect VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)

# Shared object/network/utility code used by both the server and client.
add_library(netconnect_core STATIC
    src/Socket.cpp
    src/Logger.cpp
    src/Utils.cpp
    src/Config.cpp
)
target_include_directories(netconnect_core PUBLIC include)
target_link_libraries(netconnect_core PUBLIC Threads::Threads)

add_executable(netconnect_server
    src/main_server.cpp
    src/Server.cpp
)
target_link_libraries(netconnect_server PRIVATE netconnect_core)

add_executable(netconnect_client
    src/main_client.cpp
    src/Client.cpp
)
target_link_libraries(netconnect_client PRIVATE netconnect_core)

# Copy the default config next to the built binaries for convenience.
configure_file(${CMAKE_SOURCE_DIR}/config.txt ${CMAKE_BINARY_DIR}/config.txt COPYONLY)

install(TARGETS netconnect_server netconnect_client RUNTIME DESTINATION bin)

enable_testing()
if(EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
