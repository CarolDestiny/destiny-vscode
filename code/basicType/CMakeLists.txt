add_subdirectory(queue)

add_library(destiny_basicType INTERFACE)
target_link_libraries(
    destiny_basicType
    INTERFACE
    destiny_basicType_queue
)
