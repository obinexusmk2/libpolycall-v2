# CMakeLists.txt - Fixed version for agha-dozie with ODTS integration
cmake_minimum_required(VERSION 3.16)
project(agha_dozie VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTING "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_ODTS_INTEGRATION "Enable ODTS integration" ON)

# Add module path for custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find required packages
find_package(Threads REQUIRED)

# Handle ODTS dependency
if(ENABLE_ODTS_INTEGRATION)
    # First, try to find ODTS as a pre-installed package
    find_package(ODTS QUIET)
    
    if(NOT ODTS_FOUND)
        # Try to find ODTS in sibling directory
        set(ODTS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../odts")
        
        if(EXISTS "${ODTS_SOURCE_DIR}/CMakeLists.txt")
            message(STATUS "Found ODTS in sibling directory: ${ODTS_SOURCE_DIR}")
            add_subdirectory("${ODTS_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/odts")
            set(ODTS_FOUND TRUE)
            set(ODTS_INCLUDE_DIRS "${ODTS_SOURCE_DIR}/include")
            set(ODTS_LIBRARIES odts_core)
        else()
            message(WARNING "ODTS not found. ODTS integration will be disabled.")
            set(ENABLE_ODTS_INTEGRATION OFF)
        endif()
    endif()
    
    if(ODTS_FOUND)
        message(STATUS "ODTS integration enabled")
        add_definitions(-DENABLE_ODTS_INTEGRATION=1)
    endif()
endif()

# Create interface libraries for project-wide settings
add_library(agha_dozie_warnings INTERFACE)
add_library(agha_dozie_options INTERFACE)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(agha_dozie_warnings INTERFACE
        -Wall -Wextra -Wpedantic
        -Wconversion -Wsign-conversion
        -Wunused-parameter -Wunused-variable
        -Wshadow -Wnon-virtual-dtor
        -Wold-style-cast -Wcast-align
    )
elseif(MSVC)
    target_compile_options(agha_dozie_warnings INTERFACE
        /W4 /permissive-
    )
endif()

# Compiler options
target_compile_options(agha_dozie_options INTERFACE
    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# Create main library
add_library(agha_dozie_core STATIC
    src/agha_dozie.c
    src/pattern_recognition.c
    src/coherence_framework.c
    src/fault_tolerance.c
    src/odts_integration.c
)

target_include_directories(agha_dozie_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(agha_dozie_core
    PUBLIC
        agha_dozie_warnings
        agha_dozie_options
        Threads::Threads
    PRIVATE
        $<$<BOOL:${ENABLE_ODTS_INTEGRATION}>:${ODTS_LIBRARIES}>
)

# ODTS integration
if(ENABLE_ODTS_INTEGRATION AND ODTS_FOUND)
    target_include_directories(agha_dozie_core PRIVATE ${ODTS_INCLUDE_DIRS})
    target_compile_definitions(agha_dozie_core PRIVATE ENABLE_ODTS_INTEGRATION)
endif()

# Create gatogi algorithm library
add_library(gatogi_algorithms STATIC
    src/gatogi/gatogi_algorithm.c
    src/gatogi/pattern_gating_odts.c
    src/gatogi/fault_tolerance_integration.c
    src/gatogi/multi_polygon_analysis.c
)

target_include_directories(gatogi_algorithms
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gatogi_algorithms
    PUBLIC
        agha_dozie_core
    PRIVATE
        agha_dozie_warnings
        agha_dozie_options
)

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)
    if(Doxygen_FOUND)
        add_subdirectory(docs)
    endif()
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS agha_dozie_core gatogi_algorithms
    EXPORT agha_dozieTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Export targets
install(EXPORT agha_dozieTargets
    FILE agha_dozieTargets.cmake
    NAMESPACE agha_dozie::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/agha_dozie
)

# Create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/agha_dozieConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/agha_dozieConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/agha_dozie
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/agha_dozieConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/agha_dozieConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/agha_dozieConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/agha_dozie
)

# Print configuration summary
message(STATUS "")
message(STATUS "=== Agha-Dozie Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "ODTS Integration: ${ENABLE_ODTS_INTEGRATION}")
message(STATUS "Build testing: ${BUILD_TESTING}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
