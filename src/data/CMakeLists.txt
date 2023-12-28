set(SABLE_DATA_SOURCE_FILES "")

list(
    APPEND SABLE_DATA_SOURCE_FILES

    "${CMAKE_CURRENT_SOURCE_DIR}/datastore.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/datastore.h"
)

add_library(sable_data STATIC ${SABLE_DATA_SOURCE_FILES})

target_link_libraries(sable_data)
target_include_directories(sable_data PUBLIC ${SABLE_INCLUDE_DIR} ${SABLE_INC_DIR})
