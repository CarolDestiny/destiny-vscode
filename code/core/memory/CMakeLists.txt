add_library(
    destiny_core_memory
    STATIC
    memory.hpp
    share.hpp
    onload.cpp
    unload.cpp
    memory.cpp
)

target_include_directories(
    destiny_core_memory
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
