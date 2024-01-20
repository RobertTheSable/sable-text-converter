file(GLOB SABLE_TEST_FILES
    
    catch/main.cpp
    helpers/files.h
    helpers/helpers.h
    helpers/helpers.cpp

    catch/data/collisions_test.cpp
    catch/data/table_test.cpp
    catch/data/addresslist_test.cpp
    catch/data/mapper_test.cpp

    catch/font/fonts_test.cpp
    catch/font/fonts_prefix_tests.cpp
    catch/font/characteriterator_test.cpp
    catch/font/error_test.cpp
    catch/font/normalize_test.cpp

    catch/output/rompatcher_test.cpp
    catch/output/capture_test.cpp
    catch/output/formatter_test.cpp

    catch/parse/textparser_tests.cpp
    catch/parse/testparser_singleline_tests.cpp
    catch/parse/textparser_nonprefixed_tests.cpp
    catch/parse/unicode_test.cpp
    catch/parse/block_test.cpp
    catch/parse/parse_test.cpp
    catch/parse/textparser_test_helpers.h

    catch/project/group_test.cpp
    catch/project/groupparser_test.cpp
    catch/project/folder_test.cpp
    catch/project/handler_test.cpp
    catch/project/mapperconv_test.cpp
    catch/project/roms_test.cpp
    catch/project/project_test.cpp
    catch/project/util_test.cpp
)

include_directories(helpers)

message(STATUS ${Catch2_INCLUDE_DIRS})

add_executable(tests ${SABLE_TEST_FILES})
target_compile_definitions(sable_project PUBLIC USER_SABLE_TEST_HELPERS)
target_link_libraries(tests sable_project Catch2::Catch2)
target_include_directories(tests PUBLIC ${Catch2_INCLUDE_DIRS})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.sfc" "sample.sfc" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.smc" "sample.smc" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.asm" "sample.asm" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/bad_test.asm" "bad_test.asm" COPYONLY)

if (ICU_DATA_FILE)
    get_filename_component(ICU_DATA_FILENAME ${ICU_DATA_FILE} NAME)
    configure_file(${ICU_DATA_FILE} ${ICU_DATA_FILENAME} COPYONLY)
endif()

include(Catch)
catch_discover_tests(tests)
