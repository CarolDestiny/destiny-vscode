add_subdirectory(queueBoundMPMC)
add_subdirectory(queueBoundSPSC)

add_library(destiny_basicType_queue INTERFACE)
target_link_libraries(
    destiny_basicType_queue
    INTERFACE
    destiny_basicType_queue_queueBoundMPMC
    destiny_basicType_queue_queueBoundSPSC
)
