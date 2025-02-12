set(SABLE_LIBRARIES "")

find_package(yaml-cpp)
set(YAML ${yaml-cpp_LIBRARIES})
set(YAML_INCLUDE_DIR ${yaml-cpp_INCLUDE_DIRS})

if(SABLE_BUILD_TESTS)
    find_package(Catch2 REQUIRED)
endif()

find_package(ICU 66.1 REQUIRED COMPONENTS in uc dt)

# conan generates more libraries than I need
set(SABLE_ICU_DEPS ICU::in ICU::uc ICU::dt)

# icu is sometimes in unicode or unicode2 subfolder, but cmake doesn't say which

list(
    APPEND SABLE_LIBRARIES

    ${YAML}
)

function(find_cxxopts_includes Target)
    find_package(cxxopts)

    if (cxxopts_INCLUDE_DIRS)
        target_include_directories(${Target} PUBLIC ${cxxopts_INCLUDE_DIRS})
    endif()
endfunction(find_cxxopts_includes)

function(check_codecov Target)
    if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      message("-- enabling code coverage.")
      # Add required flags (GCC & LLVM/Clang)
      target_compile_options(coverage_config INTERFACE
        -O0        # no optimization
        -g         # generate debug info
        --coverage # sets all required flags
      )
      if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
        target_link_options(coverage_config INTERFACE --coverage)
      else()
        target_link_libraries(coverage_config INTERFACE --coverage)
      endif()
    endif()
    list(APPEND ${Libraries} coverage_config)
endfunction(check_codecov)
