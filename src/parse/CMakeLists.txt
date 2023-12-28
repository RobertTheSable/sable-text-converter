set(SABLE_PARSING_SOURCE_FILES "")

list(
    APPEND SABLE_PARSING_SOURCE_FILES

    "${CMAKE_CURRENT_SOURCE_DIR}/unicode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/unicode.cpp"
    textparser.h
    textparser.cpp
    parse.h
    block.h
    block.cpp
    result.h
)

add_library(sable_parsing STATIC ${SABLE_PARSING_SOURCE_FILES})
target_include_directories(sable_parsing PUBLIC ${SABLE_INCLUDE_DIR})

