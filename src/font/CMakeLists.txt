file(GLOB SABLE_FONT_SOURCE_FILES

    "${CMAKE_CURRENT_SOURCE_DIR}/characteriterator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/characteriterator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/font.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/font.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/builder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/builder.h"
    fonthelpers.h
    normalize.cpp
    normalize.h
    error.cpp
    error.h
    codenotfound.h

)

add_library(sable_font STATIC ${SABLE_FONT_SOURCE_FILES})

target_link_libraries(sable_font PUBLIC ${SABLE_ICU_DEPS})
target_include_directories(sable_font PUBLIC ${SABLE_INCLUDE_DIR} ${YAML_INCLUDE_DIR} ${ICU_INCLUDE_DIRS})
