cmake_minimum_required(VERSION 3.8)
project(SableTextConverter LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard whose features are requested to build this target.")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Boolean describing whether the value of CXX_STANDARD is a requirement.")
set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "Boolean specifying whether compiler specific extensions are requested.")

option(SABLE_BUILD_TESTS "Build tests." OFF)
option(SABLE_BUILD_MAIN "Build tests." ON)

set(SABLE_BINARY_PATH "${CMAKE_CURRENT_BINARY_DIR}")

find_package(Git)
if(NOT Git_FOUND)
  message(FATAL_ERROR "Git not found: git is required for submodules.")
endif()

include(cmake/platform.cmake)
include(cmake/libraries.cmake)
include(cmake/filesystem.cmake)

include_directories(include external/utf8/source external/cxxopts/include)

add_subdirectory(src)

if (SABLE_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(external/Catch2/)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/Catch2/contrib")
    include_directories(src)
    add_subdirectory(tests)
endif()
