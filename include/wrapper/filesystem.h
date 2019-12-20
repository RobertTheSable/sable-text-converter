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
namespace fs = std::filesystem;

#endif

#endif
