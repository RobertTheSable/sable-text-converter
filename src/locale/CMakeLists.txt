file(GLOB SABLE_LOCALE_SOURCE_FILES

    "${CMAKE_CURRENT_SOURCE_DIR}/localecheck.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/localecheck.h"
)

add_library(sable_locale STATIC ${SABLE_LOCALE_SOURCE_FILES})

target_link_libraries(sable_locale ${ICU_LIBRARIES} Boost::locale)
target_include_directories(sable_locale PUBLIC ${Boost_INCLUDE_DIRS} ${ICU_INCLUDE_DIRS})
