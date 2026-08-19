add_library(
    destiny_core_diagnostic
    STATIC
    diagnostic.hpp
    share.hpp
    onload.cpp
    unload.cpp
    diagnostic.cpp
)

target_include_directories(
    destiny_core_diagnostic
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

