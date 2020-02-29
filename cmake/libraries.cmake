if (NOT SABLE_REBUILD_YAML)
    find_library(YAML yaml-cpp)
endif()

if(NOT YAML)
    message("-- yaml-cpp library not found, using git.")

    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init -- external/yaml-cpp
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    
    set (YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set (YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set (YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/yaml-cpp)
    set_target_properties(yaml-cpp PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SABLE_BINARY_PATH}/yaml-cpp"
        LIBRARY_OUTPUT_DIRECTORY "${SABLE_BINARY_PATH}"
    )
    include_directories(external/yaml-cpp/include)
else()
    message(STATUS "Using system yaml-cpp library.")
endif()

if (SABLE_BUILD_MAIN)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init -- external/asar 
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set (ASAR_GEN_EXE OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/asar/src/asar)
    set_target_properties(asar PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SABLE_BINARY_PATH}/asar"
        LIBRARY_OUTPUT_DIRECTORY "${SABLE_BINARY_PATH}"
    )
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        set_target_properties(asar PROPERTIES PREFIX "")
endif()
endif()

execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init -- external/utf8 
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init -- external/cxxopts
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
                
if (SABLE_BUILD_TESTS)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init -- external/Catch2
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()
