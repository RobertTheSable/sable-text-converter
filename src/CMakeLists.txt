
set(FE3_SOURCE_FILES "")

set(FE3_LIBRARIES "")

list(
    APPEND FE3_SOURCE_FILES
    
    "${PROJECT_SOURCE_DIR}/include/asar/asardll.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/script.cpp"
)

list(
    APPEND FE3_LIBRARIES
    
    yaml-cpp
    asar
)

add_executable(
    fe3-inserter 
    
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    
    ${FE3_SOURCE_FILES}
)

set_target_properties(fe3-inserter PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)

target_link_libraries(
    fe3-inserter ${FE3_LIBRARIES} ${FE3_FS_LIBRARIES} ${FE3_PLATFORM_LIBRARIES}
)
target_compile_options(fe3-inserter PRIVATE -fPIC )
if(FE3_ALT_FILESYSTEM)
    message(STATUS "Defining ${FE3_ALT_FILESYSTEM}")
    target_compile_definitions(fe3-inserter PUBLIC ${FE3_ALT_FILESYSTEM})
endif()
