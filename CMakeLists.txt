# LibPolycall V2 - Top-Level CMake
# Delegates to core/ for the main library build
cmake_minimum_required(VERSION 3.15)

file(READ "VERSION" POLYCALL_VERSION)
string(STRIP "${POLYCALL_VERSION}" POLYCALL_VERSION)

project(libpolycall-v2
    VERSION ${POLYCALL_VERSION}
    LANGUAGES C
    DESCRIPTION "Polymorphic Core Library - Unified FFI Bridge"
)

# Build core library
add_subdirectory(core)
