file(GLOB SABLE_OUTPUT_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/include/asar/asardll.c"
    "${PROJECT_SOURCE_DIR}/include/asar/asardll.h"

    rompatcher.cpp
    rompatcher.h
    outputcapture.cpp
    outputcapture.h
    formatter.cpp
    formatter.h
)

add_library(sable_output STATIC ${SABLE_OUTPUT_SOURCE_FILES})
set_target_properties(sable_output PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(sable_output PUBLIC sable_data)
target_include_directories(sable_output PUBLIC ${SABLE_INCLUDE_DIR} ${SABLE_INC_DIR})
