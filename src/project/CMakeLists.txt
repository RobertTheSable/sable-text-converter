file(GLOB SABLE_PROJECT_FILES
    builder.h
    exceptions.h
    folder.h
    group.h
    groupparser.h
    handler.h
    helpers.h
    localecheck.h
    mapperconv.h
    project.h
    util.h
    builder.cpp
    folder.cpp
    group.cpp
    handler.cpp
    localecheck.cpp
    mapperconv.cpp
    project.cpp
    util.cpp
)

add_library(sable_project STATIC ${SABLE_PROJECT_FILES})

target_link_libraries(sable_project sable_font sable_data sable_parsing sable_output ${SABLE_LIBRARIES} ${SABLE_FS_LIBRARIES} ${SABLE_PLATFORM_LIBRARIES})

target_include_directories(sable_project PUBLIC ${ICU_INCLUDE_DIRS} ${YAML_INCLUDE_DIR} ${SABLE_INC_DIR})
