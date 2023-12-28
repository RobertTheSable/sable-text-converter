set(SABLE_TEST_FILES "")

list(
    APPEND SABLE_TEST_FILES
    
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/helpers/helpers.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/helpers/helpers.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/catch/parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/table.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/project.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/datastore.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/mapping.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/fonts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/rompatcher.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/characteriterator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/collisions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/unicode.cpp"
)

include_directories(helpers)

message(STATUS ${Catch2_INCLUDE_DIRS})

add_executable(tests ${SABLE_TEST_FILES})
target_link_libraries(tests sable_lib Catch2::Catch2)
target_include_directories(tests PUBLIC ${Catch2_INCLUDE_DIRS})
#if (NOT ASAR)
#    add_custom_command(TARGET tests
#        POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:asar> "${CMAKE_CURRENT_BINARY_DIR}"
#        COMMENT "Copy asar file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
#    )
#endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.sfc" "sample.sfc" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.asm" "sample.asm" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/bad_test.asm" "bad_test.asm" COPYONLY)

include(Catch)
catch_discover_tests(tests)
