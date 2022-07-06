set(SABLE_PLATFORM_LIBRARIES "")
set(SABLE_USING_LIBC FALSE)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Use static mingw libraries to avoid a bunch of weird dlls for the user.
        set(CMAKE_EXE_LINKER_FLAGS " -static")
        list(
            APPEND SABLE_PLATFORM_LIBRARIES
            
            stdc++
        )
    endif()
else()
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
            set(SABLE_USING_LIBC TRUE)
        endif()
    endif()
    # Need ldl on *nix for Asar loading.
    list(
        APPEND SABLE_PLATFORM_LIBRARIES
        
        ${CMAKE_DL_LIBS}
    )   
endif() 
set(CMAKE_VERBOSE_MAKEFILE TRUE)
