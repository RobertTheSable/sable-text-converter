file(GLOB SABLE_PARSING_SOURCE_FILES

    unicode.h
    unicode.cpp
    textparser.h
    textparser.cpp
    parse.h
    block.h
    block.cpp
    result.h
)

add_library(sable_parsing STATIC ${SABLE_PARSING_SOURCE_FILES})
target_include_directories(sable_parsing PUBLIC ${SABLE_INCLUDE_DIR})
