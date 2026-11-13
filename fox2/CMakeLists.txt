cmake_minimum_required(VERSION 3.16)

# Version of the code (keep in sync with configure.ac)
project(FOX VERSION 1.7.95 LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not supported; use: cmake -S . -B <builddir>")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
include(FOXFeatures)

set(FOX_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set(FOX_MINOR_VERSION ${PROJECT_VERSION_MINOR})
set(FOX_PATCH_LEVEL   ${PROJECT_VERSION_PATCH})
configure_file(include/fxver.h.in include/fxver.h @ONLY)

# Executables and the DLL go to one directory, so tests find the DLL on Windows
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory(lib)

if(FOX_BUILD_TESTS)
  add_subdirectory(tools)
  add_subdirectory(tests)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fox-1.7
        FILES_MATCHING PATTERN "*.h")
install(FILES "${CMAKE_BINARY_DIR}/include/fxver.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fox-1.7)
