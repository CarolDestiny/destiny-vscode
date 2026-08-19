cmake_minimum_required(VERSION 3.20)
project(destiny LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../output")

find_package(Threads REQUIRED)
add_subdirectory(basicType)
add_subdirectory(core)
add_subdirectory(define)

add_executable(destiny main.cpp)
target_link_libraries(
    destiny
    PRIVATE
    Threads::Threads
    destiny_basicType
    destiny_core
    destiny_define
)
