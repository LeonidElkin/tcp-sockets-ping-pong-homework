cmake_minimum_required(VERSION 3.20)
project(tcp-sockets-homework LANGUAGES CXX)

if (NOT UNIX)
    message(FATAL_ERROR "This project is intended to be built on UNIX-like systems (Linux / WSL).")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
endif()

add_executable(tcp-sockets-homework main.cpp)
