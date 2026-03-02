# CMakeLists.txt for Agha-Dozie MVP with ODTS Integration
cmake_minimum_required(VERSION 3.25)

project(AghaDozieRegistry 
    VERSION 1.0.0
    LANGUAGES C CXX
    DESCRIPTION "OBINexus Registry with ODTS Integration"
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==================== Build Options ====================

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_ODTS "Enable ODTS integration" ON)

# ==================== Find Dependencies ====================

# ODTS Integration (optional if not found)
if(ENABLE_ODTS)
    # Try to find ODTS
    find_package(ODTS QUIET)
    
    if(NOT ODTS_FOUND)
        message(STATUS "ODTS not found - building without ODTS support")
        message(STATUS "To enable ODTS, install it or set ODTS_DIR to the ODTS installation directory")
        set(ENABLE_ODTS OFF)
    else()
        message(STATUS "ODTS found: ${ODTS_DIR}")
    endif()
endif()

# ==================== Core Library: Agha-Dozie ====================

set(AGHA_DOZIE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/gatogi_algorithm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pattern_gating_odts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fault_tolerance_integration.c
)

set(AGHA_DOZIE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/odts_integration.h
)

add_library(agha_dozie ${AGHA_DOZIE_SOURCES})

target_include_directories(agha_dozie
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

if(ENABLE_ODTS)
    target_link_libraries(agha_dozie PUBLIC ODTS::odts)
    target_compile_definitions(agha_dozie PUBLIC AGHA_DOZIE_WITH_ODTS)
endif()

# ==================== Registry Server Library ====================

# Note: Registry server is in Rust, this is just for C/FFI bridge
set(REGISTRY_FFI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/libpolycall_bridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/polycall.c
    ${CMAKE_CURRENT_SOURCE_DIR}/polycall_protocol.c
)

add_library(registry_ffi ${REGISTRY_FFI_SOURCES})

target_include_directories(registry_ffi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(registry_ffi PUBLIC agha_dozie)

# ==================== Examples ====================

if(BUILD_EXAMPLES)
    # Gatogi multi-polygon example
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/gatogi_mulit_polygon.c)
        add_executable(gatogi_multi_polygon 
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/gatogi_mulit_polygon.c
        )
        target_link_libraries(gatogi_multi_polygon PRIVATE agha_dozie)
    endif()
    
    # Fault tolerance example
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/fault_tolerance_integration.c)
        add_executable(fault_tolerance_demo
            ${CMAKE_CURRENT_SOURCE_DIR}/fault_tolerance_integration.c
        )
        target_link_libraries(fault_tolerance_demo PRIVATE agha_dozie)
    endif()
endif()

# ==================== Tests ====================

if(BUILD_TESTS)
    enable_testing()
    
    # Find test framework (e.g., Google Test)
    find_package(GTest QUIET)
    
    if(GTest_FOUND)
        add_executable(agha_dozie_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_gatogi.c
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_pattern_gating.c
        )
        
        target_link_libraries(agha_dozie_tests
            PRIVATE
                agha_dozie
                GTest::gtest_main
        )
        
        include(GoogleTest)
        gtest_discover_tests(agha_dozie_tests)
    else()
        message(STATUS "Google Test not found - skipping C++ tests")
    endif()
endif()

# ==================== Installation ====================

include(GNUInstallDirs)

install(TARGETS agha_dozie registry_ffi
    EXPORT AghaDozieTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${AGHA_DOZIE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/agha_dozie
)

# Install CMake package config
install(EXPORT AghaDozieTargets
    FILE AghaDozieTargets.cmake
    NAMESPACE AghaDozie::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AghaDoz ie
)

# ==================== Package Config ====================

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AghaDozieConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AghaDozieConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AghaDozie
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/AghaDozieConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/AghaDozieConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AghaDozieConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AghaDozie
)

# ==================== Summary ====================

message(STATUS "")
message(STATUS "======================================")
message(STATUS "Agha-Dozie Registry Configuration")
message(STATUS "======================================")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  C Compiler:        ${CMAKE_C_COMPILER}")
message(STATUS "  CXX Compiler:      ${CMAKE_CXX_COMPILER}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  ODTS support:      ${ENABLE_ODTS}")
message(STATUS "  Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "  Build tests:       ${BUILD_TESTS}")
message(STATUS "  Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "======================================")
message(STATUS "")
