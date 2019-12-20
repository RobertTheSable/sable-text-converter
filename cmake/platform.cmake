set(FE3_PLATFORM_LIBRARIES "")

# Don't need ldl on windows
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set_target_properties(asar PROPERTIES PREFIX "")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_EXE_LINKER_FLAGS " -static")
        list(
            APPEND FE3_PLATFORM_LIBRARIES
            
            stdc++
        )
    endif()
else()
    list(
        APPEND FE3_PLATFORM_LIBRARIES
        
        ${CMAKE_DL_LIBS}
    )   
endif() 
set(CMAKE_VERBOSE_MAKEFILE TRUE)
