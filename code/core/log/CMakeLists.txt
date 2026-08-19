add_library(
    destiny_core_log
    STATIC
    log.hpp
    share.hpp
    onload.cpp
    unload.cpp
    log.cpp

    slowWrite.hpp
    slowWrite.cpp
)

target_include_directories(
    destiny_core_log
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
