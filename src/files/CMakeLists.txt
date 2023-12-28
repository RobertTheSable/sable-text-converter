set(SABLE_FILES_SOURCE_FILES "")

list(
    APPEND SABLE_FILES_SOURCE_FILES

    group.h
    file.h
    file.cpp
)

add_library(sable_files STATIC ${SABLE_FILES_SOURCE_FILES})
set_target_properties(sable_files PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(sable_files PUBLIC ${SABLE_INCLUDE_DIR} ${SABLE_INC_DIR})
#target_link_libraries(sable_files)
