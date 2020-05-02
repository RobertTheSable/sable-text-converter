
set(SABLE_SOURCE_FILES "")

list(
    APPEND SABLE_SOURCE_FILES

    "${PROJECT_SOURCE_DIR}/include/asar/asardll.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/font.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/font.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/exceptions.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/parse.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/parse.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/util.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/util.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/table.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/table.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/rompatcher.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/rompatcher.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/project.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/project.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/datastore.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/datastore.h"
)

add_library(sable_lib STATIC ${SABLE_SOURCE_FILES})

target_link_libraries(sable_lib PUBLIC coverage_config ${SABLE_LIBRARIES} ${SABLE_FS_LIBRARIES} ${SABLE_PLATFORM_LIBRARIES})

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
endif()

if(SABLE_ALT_FILESYSTEM)
    message(STATUS "Defining ${SABLE_ALT_FILESYSTEM}")
    target_compile_definitions(sable_lib PUBLIC ${SABLE_ALT_FILESYSTEM})
    if (SABLE_PLATFORM_DEFINES)
        target_compile_definitions(sable_lib PUBLIC ${SABLE_PLATFORM_DEFINES})
    endif()
endif()
