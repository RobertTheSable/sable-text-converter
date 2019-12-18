
set(FE3_SOURCE_FILES "")

set(FE3_LIBRARIES "")

list(
    APPEND FE3_SOURCE_FILES
    
    "${PROJECT_SOURCE_DIR}/include/asar/asardll.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/script.cpp"
)

list(
    APPEND FE3_LIBRARIES
    
    stdc++fs
    yaml-cpp
    asar
)

# Don't need ldl on windows
if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    list(
        APPEND FE3_LIBRARIES
        
        ${CMAKE_DL_LIBS}
    )   
endif()

add_executable(
    fe3-inserter 
    
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    
    ${FE3_SOURCE_FILES}
)

set_target_properties(fe3-inserter PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)

target_link_libraries(
    fe3-inserter ${FE3_LIBRARIES}
)
target_compile_options(fe3-inserter PRIVATE -fPIC)
