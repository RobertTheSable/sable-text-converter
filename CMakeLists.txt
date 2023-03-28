cmake_minimum_required(VERSION 3.20)
project(SableTextConverter LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard whose features are requested to build this target.")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Boolean describing whether the value of CXX_STANDARD is a requirement.")

option(SABLE_BUILD_TESTS "Build tests." OFF)
option(SABLE_BUILD_MAIN "Build main interface." ON)

add_library(coverage_config INTERFACE)

option(CODE_COVERAGE "Enable coverage reporting" OFF)

set(SABLE_BINARY_PATH "${CMAKE_CURRENT_BINARY_DIR}")

find_package(Git)
if(NOT Git_FOUND)
  message(FATAL_ERROR "Git not found: git is required for submodules.")
endif()

include(cmake/platform.cmake)
include(cmake/libraries.cmake)
include(cmake/filesystem.cmake)

set(SABLE_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(src)

if (SABLE_BUILD_TESTS)
    message(STATUS "Building tests.")
    cmake_policy(SET CMP0110 OLD)
    include(CTest)
    enable_testing()
    include_directories(src)
    add_subdirectory(tests)
endif()
