set(SABLE_OUTPUT_SOURCE_FILES "")

list(
    APPEND SABLE_OUTPUT_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/include/asar/asardll.c"

    rompatcher.cpp
    rompatcher.h
)

add_library(sable_output STATIC ${SABLE_OUTPUT_SOURCE_FILES})
set_target_properties(sable_output PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(sable_output PUBLIC ${SABLE_INCLUDE_DIR} ${SABLE_INC_DIR})


