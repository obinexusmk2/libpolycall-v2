# cmake/FindODTS.cmake
# Find ODTS (OBINexus Derivative Tracing System)
#
# This module defines:
#  ODTS_FOUND
#  ODTS_INCLUDE_DIRS
#  ODTS_LIBRARIES

find_path(ODTS_INCLUDE_DIR
    NAMES odts/odts_core.h
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/../odts/include
        /usr/local/include
        /usr/include
)

find_library(ODTS_LIBRARY
    NAMES odts_core
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/../odts/build
        /usr/local/lib
        /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ODTS
    REQUIRED_VARS ODTS_LIBRARY ODTS_INCLUDE_DIR
)

if(ODTS_FOUND)
    set(ODTS_INCLUDE_DIRS ${ODTS_INCLUDE_DIR})
    set(ODTS_LIBRARIES ${ODTS_LIBRARY})
endif()

mark_as_advanced(ODTS_INCLUDE_DIR ODTS_LIBRARY)
