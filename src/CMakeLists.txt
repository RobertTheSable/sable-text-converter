
set(SABLE_SOURCE_FILES "")

set(SABLE_LIBRARIES "")

list(
    APPEND SABLE_SOURCE_FILES
    
    "${PROJECT_SOURCE_DIR}/include/asar/asardll.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/script.cpp"
)

list(
    APPEND SABLE_LIBRARIES
    
    yaml-cpp
    asar
)

add_executable(
    sable 
    
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    
    ${SABLE_SOURCE_FILES}
)

set_target_properties(sable PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SABLE_BINARY_PATH}"
)

target_link_libraries(
    sable ${SABLE_LIBRARIES} ${SABLE_FS_LIBRARIES} ${SABLE_PLATFORM_LIBRARIES}
)
target_compile_options(sable PRIVATE -fPIC )
if(SABLE_ALT_FILESYSTEM)
    message(STATUS "Defining ${SABLE_ALT_FILESYSTEM}")
    target_compile_definitions(sable PUBLIC ${SABLE_ALT_FILESYSTEM})
endif()
