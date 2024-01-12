file(GLOB SABLE_PARSING_SOURCE_FILES

    unicode.h
    unicode.cpp
    textparser.h
    textparser.cpp
    parse.h
    block.h
    block.cpp
    result.h
    errorhandling.h
)

add_library(sable_parsing STATIC ${SABLE_PARSING_SOURCE_FILES})
target_link_libraries(sable_parsing sable_data sable_font ${SABLE_ICU_DEPS})
target_include_directories(sable_parsing PUBLIC ${SABLE_INCLUDE_DIR} ${ICU_INCLUDE_DIRS})
