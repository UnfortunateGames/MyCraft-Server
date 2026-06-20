cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD          17 )
set(CMAKE_C_STANDARD_REQUIRED ON )
set(CMAKE_C_EXTENSIONS        OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(MyCraft VERSION 0.1.0 LANGUAGES C)

add_executable(myc-server)

add_library(craftserv)

find_package(ZLIB REQUIRED)
#find_library(MATH_LIBRARY m REQUIRED)

target_sources(
    myc-server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
)
target_sources(
    craftserv
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internals/debug/logger.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internals/server/packets.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internals/server/sockets.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internals/server/data.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internals/server/play.c
)

target_include_directories(
    craftserv
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/internals
)

target_link_libraries(
    myc-server
    PRIVATE
        craftserv
        ZLIB::ZLIB
        m
)

target_compile_options(
    myc-server
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        #-Werror

        -O3

        -g
        -fsanitize=address,undefined
)

target_link_options(
    myc-server
    PRIVATE
        -fsanitize=address,undefined
)
