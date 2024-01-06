set(SABLE_TEST_FILES "")

list(
    APPEND SABLE_TEST_FILES
    
    catch/main.cpp
    helpers/files.h
    helpers/helpers.h
    helpers/helpers.cpp

    catch/data/collisions.cpp
    catch/data/table.cpp
    catch/data/addresslist.cpp

    catch/font/fonts.cpp
    catch/font/characteriterator.cpp
    catch/font/error.cpp

    catch/output/rompatcher.cpp
    catch/output/capture.cpp
    catch/output/formatter.cpp

    catch/parse/textparser.cpp
    catch/parse/unicode.cpp
    catch/parse/block.cpp
    catch/parse/parse.cpp

    catch/project/group.cpp
    catch/project/groupparser.cpp
    catch/project/folder.cpp
    catch/project/handler.cpp
    catch/project/mapperconv.cpp
    catch/project/roms.cpp

    catch/project.cpp
    catch/mapping.cpp
)

include_directories(helpers)

message(STATUS ${Catch2_INCLUDE_DIRS})

add_executable(tests ${SABLE_TEST_FILES})
target_compile_definitions(sable_lib PUBLIC USER_SABLE_TEST_HELPERS)
target_link_libraries(tests sable_lib Catch2::Catch2)
target_include_directories(tests PUBLIC ${Catch2_INCLUDE_DIRS})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.sfc" "sample.sfc" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.smc" "sample.smc" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/sample.asm" "sample.asm" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sample/bad_test.asm" "bad_test.asm" COPYONLY)

include(Catch)
catch_discover_tests(tests)
