cmake_minimum_required(VERSION 3.4)
project(FE3Inserter LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(YAML yaml-cpp)

if(NOT YAML)
    message("yaml-cpp library not found.")

    execute_process(COMMAND git submodule update --init -- external/yaml-cpp
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    
    set (YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set (YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set (YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/yaml-cpp)
    set_target_properties(yaml-cpp PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
    )
else()
    message("-- Using system yaml-cpp library.")
endif()

execute_process(COMMAND git submodule update --init -- external/asar 
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set (ASAR_GEN_EXE OFF CACHE BOOL "" FORCE)
add_subdirectory(external/asar/src/asar)
set_target_properties(asar PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)

execute_process(COMMAND git submodule update --init -- external/utf8 
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(include external/utf8/source)

add_subdirectory(src)
