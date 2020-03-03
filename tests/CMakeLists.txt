set(SABLE_TEST_FILES "")

list(
    APPEND SABLE_TEST_FILES
    
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/table.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/mapping.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/rompatcher.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/fonts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/project.cpp"
)

add_executable(tests ${SABLE_TEST_FILES})
target_link_libraries(tests ${SABLE_LIBRARIES} ${SABLE_FS_LIBRARIES} ${SABLE_PLATFORM_LIBRARIES} Catch2::Catch2)
add_custom_command(TARGET tests
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:asar> "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Copy asar file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample_text_map.yml" "sample/text_map.yml" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.sfc" "sample.sfc" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.asm" "sample.asm" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/bad_test.asm" "bad_test.asm" COPYONLY)

include(ParseAndAddCatchTests)
ParseAndAddCatchTests(tests)
