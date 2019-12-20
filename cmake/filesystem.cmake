include(CheckCXXSourceCompiles)

option(FE3_USE_STD_FS "Skip checks and try the std::filesystem library." OFF)
option(FE3_USE_EXPERIMENTAL_FS "Skip checks and try the std::experimental::filesystem library." OFF)
option(FE3_USE_BOOST_FS "Skip checks and try the boost::filesystem library." OFF)

set(FE3_FS_LIBRARIES "")
    
if (NOT (FE3_USE_STD_FS OR FE3_USE_EXPERIMENTAL_FS OR FE3_USE_BOOST_FS))
    set(STD_FILESYSTEM_TEST_PROGRAM "
        #include <filesystem>
        struct X { std::filesystem::path p; };
        int main() {}")

    set(STD_FILESYSTEM_EXPERIMENTAL_TEST_PROGRAM "
        #include <experimental/filesystem>
        struct X { std::experimental::filesystem::path p; };
        int main() {}")
        
    # Check if we have std::filesystem at our disposal.
    check_cxx_source_compiles(
            "${STD_FILESYSTEM_TEST_PROGRAM}"
            HAS_STD_FILESYSTEM)

    # Check if we have std::experimental::filesystem at our disposal.
    check_cxx_source_compiles(
            "${STD_FILESYSTEM_EXPERIMENTAL_TEST_PROGRAM}"
            HAS_EXPERIMENTAL_FILESYSTEM)
else()
    message(STATUS "Skipping filesystem check")
    set(HAS_STD_FILESYSTEM ${FE3_USE_STD_FS})
    set(HAS_EXPERIMENTAL_FILESYSTEM ${FE3_USE_EXPERIMENTAL_FS})
endif()
        
if (NOT (HAS_STD_FILESYSTEM OR HAS_EXPERIMENTAL_FILESYSTEM) )
    message(STATUS "Checking for Boost libraries...")
    find_library(HAS_BOOST_FS boost_filesystem)

    if(NOT HAS_BOOST_FS)
        message(FATAL_ERROR "-- Boost Filesystem not found.")
    else()
        list(
            APPEND FE3_FS_LIBRARIES
            
            boost_system
            boost_filesystem
        )
    endif()
    set(FE3_ALT_FILESYSTEM USE_BOOST_FILESYSTEM)
else()
    if (NOT HAS_STD_FILESYSTEM )
        set(FE3_ALT_FILESYSTEM USE_EXPERIMENTAL_FILESYSTEM)
    else()
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(FE3_FS_LIBRARIES "stdc++fs")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(FE3_FS_LIBRARIES "c++fs")
        endif()
    endif()
endif()

