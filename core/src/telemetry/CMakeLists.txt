set(POLYCALL_TELEMETRY_SOURCES
    guid_engine.c
    replay_engine.c
    symbolic_interpreter.c
)

add_library(polycall_telemetry STATIC ${POLYCALL_TELEMETRY_SOURCES})

target_include_directories(polycall_telemetry
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

find_package(OpenSSL REQUIRED)
target_link_libraries(polycall_telemetry
    PRIVATE
        OpenSSL::Crypto
        Threads::Threads
)

if(TARGET nlink_core_components)
    add_dependencies(nlink_core_components polycall_telemetry)
endif()

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/polycall/telemetry
    DESTINATION include/polycall
)
