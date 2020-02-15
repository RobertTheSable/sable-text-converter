set(SABLE_TEST_FILES "")

list(
    APPEND SABLE_TEST_FILES
    
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch/table.cpp"
)

add_executable(tests ${SABLE_TEST_FILES})
target_link_libraries(tests yaml-cpp sable_lib Catch2::Catch2)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample_text_map.yml" "sample_text_map.yml" COPYONLY)

include(ParseAndAddCatchTests)
ParseAndAddCatchTests(tests)
