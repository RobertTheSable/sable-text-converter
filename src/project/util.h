#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

namespace sable {
namespace util {

//project
size_t calculateFileSize(const std::string& value);
std::string getFileSizeString(int value);

}

}
#endif // UTIL_H
