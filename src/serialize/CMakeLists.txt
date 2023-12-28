set(SABLE_SERIAL_SOURCE_FILES "")

list(
    APPEND SABLE_SERIAL_SOURCE_FILES

    "${CMAKE_CURRENT_SOURCE_DIR}/yamlfontserializer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/yamlfontserializer.h"
)

add_library(sable_serialize STATIC ${SABLE_SERIAL_SOURCE_FILES})

target_link_libraries(sable_serialize sable_font)

target_include_directories(sable_serialize PUBLIC ${SABLE_INCLUDE_DIR})
