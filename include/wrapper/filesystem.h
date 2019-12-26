#ifndef FS_WRAPPER_H
#define FS_WRAPPER_H

#if defined(USE_BOOST_FILESYSTEM)

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#elif defined(USE_EXPERIMENTAL_FILESYSTEM)

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#else

#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

#endif

#ifdef _WIN32
    static constexpr char sable_preferred_separator = '\\';
#else
    static constexpr char sable_preferred_separator = '/';
#endif

#endif
