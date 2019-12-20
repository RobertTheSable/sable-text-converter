cmake_minimum_required(VERSION 3.8)
project(FE3Inserter LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Git)
if(NOT Git_FOUND)
  message(FATAL_ERROR "Git not found: git is required for submodules.")
endif()

include(cmake/libraries.cmake)
include(cmake/platform.cmake)
include(cmake/filesystem.cmake)

include_directories(include external/utf8/source)

add_subdirectory(src)
