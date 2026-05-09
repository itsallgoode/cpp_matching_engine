cmake_minimum_required(VERSION 3.20)

project(cpp_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cpp_engine
    src/main.cpp
    src/order_book.cpp
    src/matching_engine.cpp
)

target_include_directories(cpp_engine PRIVATE include)

target_compile_options(cpp_engine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)