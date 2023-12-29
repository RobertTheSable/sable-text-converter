
set(SABLE_SOURCE_FILES "")

set(SABLE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})


file(GLOB SABLE_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/project.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/project.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/project/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/project/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/exceptions.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/localecheck.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/localecheck.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/util.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/util.h"
    settings.h
    settings.h
    settings.cpp

    errorhandling.h
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/parse")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/font")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/data")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/output")

add_library(sable_lib STATIC ${SABLE_SOURCE_FILES})

target_link_libraries(sable_lib sable_font sable_data sable_parsing sable_output ${SABLE_LIBRARIES} ${ICU_LIBRARIES} ${SABLE_FS_LIBRARIES} ${SABLE_PLATFORM_LIBRARIES})

message(STATUS "ICU libraries: ${ICU_LIBRARIES}")

target_include_directories(sable_lib PUBLIC ${Boost_INCLUDE_DIRS} ${ICU_INCLUDE_DIRS} ${YAML_INCLUDE_DIR} ${SABLE_INC_DIR})

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
        sable sable_lib
    )
    find_cxxopts_includes(sable)
endif()

if(SABLE_ALT_FILESYSTEM)
    message(STATUS "Defining ${SABLE_ALT_FILESYSTEM}")
    target_compile_definitions(sable_lib PUBLIC ${SABLE_ALT_FILESYSTEM})
    if (SABLE_PLATFORM_DEFINES)
        target_compile_definitions(sable_lib PUBLIC ${SABLE_PLATFORM_DEFINES})
    endif()
endif()
