# examples/CMakeLists.txt
add_executable(gatogi_multi_polygon
    gatogi_multi_polygon.c
)

target_link_libraries(gatogi_multi_polygon
    PRIVATE
        gatogi_algorithms
        agha_dozie_core
)

add_executable(odts_integration_demo
    odts_integration_demo.c
)

target_link_libraries(odts_integration_demo
    PRIVATE
        agha_dozie_core
        $<$<BOOL:${ENABLE_ODTS_INTEGRATION}>:${ODTS_LIBRARIES}>
)

if(ENABLE_ODTS_INTEGRATION)
    target_compile_definitions(odts_integration_demo PRIVATE ENABLE_ODTS_INTEGRATION)
endif()
