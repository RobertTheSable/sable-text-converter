set(FE3_SHARED_FILES "")

list(
    APPEND FE3_SHARED_FILES
    
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/asar/asardll.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/script.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/script.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/asar/asardll.h"
)

add_executable(
    fe3-inserter 
    
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    
    ${FE3_SHARED_FILES}
)

add_library( asar SHARED IMPORTED )

set (ASAR_LIBRARY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/lib/libasar.so")

set_target_properties(asar PROPERTIES IMPORTED_LOCATION ${ASAR_LIBRARY_FILE})

set_target_properties(fe3-inserter PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)

target_link_libraries(
    fe3-inserter ${CMAKE_DL_LIBS} yaml-cpp stdc++fs asar
)
target_compile_options(fe3-inserter PRIVATE -fPIC)

add_custom_command(TARGET fe3-inserter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ASAR_LIBRARY_FILE}
        $<TARGET_FILE_DIR:fe3-inserter>
)