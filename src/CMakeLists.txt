
set(SABLE_SOURCE_FILES "")

set(SABLE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/font")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/data")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/parse")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/output")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/project")

message(STATUS "Filesystem libraries: ${SABLE_FS_LIBRARIES}, Platform-specific libraries: ${SABLE_PLATFORM_LIBRARIES}")

if (SABLE_BUILD_MAIN)
    add_executable(
        sable

        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    )

    set_target_properties(sable PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SABLE_BINARY_PATH}"
    )

    target_link_libraries(
        sable sable_project
    )
    find_cxxopts_includes(sable)
endif()

if(SABLE_ALT_FILESYSTEM)
    message(STATUS "Defining ${SABLE_ALT_FILESYSTEM}")
    target_compile_definitions(sable_project PUBLIC ${SABLE_ALT_FILESYSTEM})
    if (SABLE_PLATFORM_DEFINES)
        target_compile_definitions(sable_project PUBLIC ${SABLE_PLATFORM_DEFINES})
    endif()
endif()
