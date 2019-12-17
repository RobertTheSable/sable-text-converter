cmake_minimum_required(VERSION 3.4)
project(FE3Inserter LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

macro(update_sub_module_library library)
    

find_library(YAML yaml-cpp)

if(NOT YAML)
    message("yaml-cpp library not found.")

    execute_process(COMMAND git submodule update --init -- external/yaml-cpp
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirectory(external/yaml-cpp ${CMAKE_CURRENT_BINARY_DIR}/yaml-cpp EXCLUDE_FROM_ALL)
    target_include_directories(yaml-cpp_static SYSTEM PUBLIC
                               $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/cmark/src>
                               $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/cmark/src>)
    target_compile_definitions(yaml-cpp_static PUBLIC YAML_CPP_STATIC_DEFINE)
else()
    message("using system yaml-cpp library.")
endif()

add_subdirectory(src)
