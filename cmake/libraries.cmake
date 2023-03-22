set(SABLE_LIBRARIES "")

find_library(YAML yaml-cpp REQUIRED)

if(SABLE_BUILD_TESTS)
    find_package(Catch2 REQUIRED)
endif()

find_package(Boost 1.71.0 REQUIRED COMPONENTS locale)
find_package(ICU 66.1 REQUIRED COMPONENTS in uc dt)
message(STATUS ${YAML})

list(
    APPEND SABLE_LIBRARIES

    ${YAML}
    Boost::locale
    ${ICU_LIBRARIES}
)
