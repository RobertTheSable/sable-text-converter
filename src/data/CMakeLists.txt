file(GLOB SABLE_DATA_SOURCE_FILES
    addresslist.cpp
    addresslist.h
    table.cpp
    table.h
    textblockrange.cpp
    textblockrange.h
    address.h
    options.h
    optionhelpers.h
#    "${CMAKE_CURRENT_SOURCE_DIR}/textblock.h"
)

add_library(sable_data STATIC ${SABLE_DATA_SOURCE_FILES})

target_link_libraries(sable_data)
target_include_directories(sable_data PUBLIC ${SABLE_INCLUDE_DIR} ${SABLE_INC_DIR})
