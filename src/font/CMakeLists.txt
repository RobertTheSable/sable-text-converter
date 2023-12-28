set(SABLE_FONT_SOURCE_FILES "")

list(
    APPEND SABLE_FONT_SOURCE_FILES

    "${CMAKE_CURRENT_SOURCE_DIR}/characteriterator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/characteriterator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/font.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/font.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/builder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/builder.h"
    fonthelpers.h
    error.cpp

)

add_library(sable_font STATIC ${SABLE_FONT_SOURCE_FILES})

target_link_libraries(sable_font)
target_include_directories(sable_font PUBLIC ${SABLE_INCLUDE_DIR})
