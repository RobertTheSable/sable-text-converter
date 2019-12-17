set(FE3_SHARED_FILES "")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_library(YAML yaml-cpp)

if(NOT YAML)
    message("yaml-cpp library not found.")
endif()
message("yaml-cpp library found.")

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

target_link_libraries(
    fe3-inserter ${CMAKE_DL_LIBS} yaml-cpp stdc++fs asar
)
target_compile_options(fe3-inserter PRIVATE -fPIC)

add_custom_command(TARGET fe3-inserter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ASAR_LIBRARY_FILE}
        $<TARGET_FILE_DIR:fe3-inserter>)
