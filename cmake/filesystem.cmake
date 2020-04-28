include(CheckCXXSourceCompiles)

option(SABLE_USE_STD_FS "Skip checks and try the std::filesystem library." OFF)
option(SABLE_USE_EXPERIMENTAL_FS "Skip checks and try the std::experimental::filesystem library." OFF)
option(SABLE_USE_BOOST_FS "Skip checks and try the boost::filesystem library." OFF)

set(SABLE_FS_LIBRARIES "")
    
if (NOT (SABLE_USE_STD_FS OR SABLE_USE_EXPERIMENTAL_FS OR SABLE_USE_BOOST_FS))
    set(STD_FILESYSTEM_TEST_PROGRAM "
        #include <filesystem>
        int main() {auto p = std::filesystem::current_path();}")

    set(STD_FILESYSTEM_EXPERIMENTAL_TEST_PROGRAM "
        #include <experimental/filesystem>
        int main() {auto p = std::experimental::filesystem::current_path();}")
        
    set(CMAKE_REQUIRED_DEFINITIONS -std=c++17)
        
    # Check if we have std::filesystem at our disposal.
    check_cxx_source_compiles(
            "${STD_FILESYSTEM_TEST_PROGRAM}"
            HAS_STD_FILESYSTEM)
            
    if(NOT HAS_STD_FILESYSTEM)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(CMAKE_REQUIRED_LIBRARIES c++fs)
            check_cxx_source_compiles(
                "${STD_FILESYSTEM_TEST_PROGRAM}"
                HAS_STD_FILESYSTEM_WITH_CXXFS)
            set(HAS_STD_FILESYSTEM ${HAS_STD_FILESYSTEM_WITH_CXXFS})
            set(CMAKE_REQUIRED_LIBRARIES "")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(CMAKE_REQUIRED_LIBRARIES stdc++fs)
            check_cxx_source_compiles(
                "${STD_FILESYSTEM_TEST_PROGRAM}"
                HAS_STD_FILESYSTEM_WITH_STDCXXFS)
            set(HAS_STD_FILESYSTEM ${HAS_STD_FILESYSTEM_WITH_STDCXXFS})
            set(CMAKE_REQUIRED_LIBRARIES "")
        else()

        endif()
    endif()

    if(NOT HAS_STD_FILESYSTEM)
        # Check if we have std::experimental::filesystem at our disposal.
        check_cxx_source_compiles(
                "${STD_FILESYSTEM_EXPERIMENTAL_TEST_PROGRAM}"
                HAS_EXPERIMENTAL_FILESYSTEM)

        if(NOT HAS_EXPERIMENTAL_FILESYSTEM)
            if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                set(CMAKE_REQUIRED_LIBRARIES stdc++fs)
                check_cxx_source_compiles(
                    "${STD_FILESYSTEM_EXPERIMENTAL_TEST_PROGRAM}"
                    HAS_EXP_FILESYSTEM_WITH_STDCXXFS)
                set(HAS_EXPERIMENTAL_FILESYSTEM ${HAS_EXP_FILESYSTEM_WITH_STDCXXFS})
                set(CMAKE_REQUIRED_LIBRARIES "")
            endif()
        endif()
    endif()

else()
    message(STATUS "Skipping filesystem check")
    set(HAS_STD_FILESYSTEM ${SABLE_USE_STD_FS})
    set(HAS_EXPERIMENTAL_FILESYSTEM ${SABLE_USE_EXPERIMENTAL_FS})
endif()
        
if (NOT (HAS_STD_FILESYSTEM OR HAS_EXPERIMENTAL_FILESYSTEM) )
    message(STATUS "Checking for Boost libraries...")
    find_library(HAS_BOOST_FS boost_filesystem)

    if(NOT HAS_BOOST_FS)
        message(FATAL_ERROR "-- Boost Filesystem not found.")
    else()
        list(
            APPEND SABLE_FS_LIBRARIES
            
            boost_system
            boost_filesystem
        )
    endif()
    set(SABLE_ALT_FILESYSTEM "USE_BOOST_FILESYSTEM")
else()
    if (NOT HAS_STD_FILESYSTEM )
        set(SABLE_ALT_FILESYSTEM "USE_EXPERIMENTAL_FILESYSTEM")
    endif()
    
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND HAS_EXP_FILESYSTEM_WITH_STDCXXFS)
        set(SABLE_FS_LIBRARIES "stdc++fs")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND HAS_STD_FILESYSTEM_WITH_CXXFS)
        set(SABLE_FS_LIBRARIES "c++fs")
    endif()
endif()

