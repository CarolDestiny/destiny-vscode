add_library(
    destiny_core_thread
    STATIC
    thread.hpp
    share.hpp
    onload.cpp
    unload.cpp
    thread.cpp
)

target_include_directories(
    destiny_core_memory
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
