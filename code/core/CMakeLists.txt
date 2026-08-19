add_subdirectory(diagnostic)
add_subdirectory(log)
add_subdirectory(memory)
add_subdirectory(thread)

add_library(destiny_core INTERFACE)
target_link_libraries(
    destiny_core
    INTERFACE
    destiny_core_diagnostic
    destiny_core_log
    destiny_core_memory
    destiny_core_thread
)
