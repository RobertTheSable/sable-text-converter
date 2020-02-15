
set(SABLE_SOURCE_FILES "")

set(SABLE_LIBRARIES "")

list(
    APPEND SABLE_SOURCE_FILES
    
    "${PROJECT_SOURCE_DIR}/include/asar/asardll.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/options.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cache.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/options.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/font.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/font.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/cache.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/exceptions.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/parse.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/parse.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mapping.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/project.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/project.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/table.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/table.h"
)

add_library(sable_lib STATIC ${SABLE_SOURCE_FILES})

list(
    APPEND SABLE_LIBRARIES
    
    sable_lib
    yaml-cpp
    asar
)


if (SABLE_BUILD_MAIN)
    add_executable(
        sable

        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    )

    set_target_properties(sable PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SABLE_BINARY_PATH}"
    )

    target_link_libraries(
        sable ${SABLE_LIBRARIES} ${SABLE_FS_LIBRARIES} ${SABLE_PLATFORM_LIBRARIES}
    )
    if(SABLE_ALT_FILESYSTEM)
        message(STATUS "Defining ${SABLE_ALT_FILESYSTEM}")
        target_compile_definitions(sable PUBLIC ${SABLE_ALT_FILESYSTEM})
    endif()
endif()
