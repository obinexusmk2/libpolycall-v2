# CMakeLists.txt addition
find_package(ODTS REQUIRED)
find_package(AghaDozie REQUIRED)

# Gatogi algorithm library
add_library(gatogi_algorithms 
    gatogi_algorithm.c
    pattern_gating_odts.c
    fault_tolerance_integration.c
)

target_link_libraries(gatogi_algorithms 
    PRIVATE 
    agha_dozie_core
    odts_framework
)

# Example executable
add_executable(gatogi_multi_polygon examples/gatogi_multi_polygon.c)
target_link_libraries(gatogi_multi_polygon gatogi_algorithms)
