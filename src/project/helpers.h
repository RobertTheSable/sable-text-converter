#ifndef HELPERS_H
#define HELPERS_H

#include <optional>
#include "wrapper/filesystem.h"

namespace sable {
inline bool isFile(fs::path path) {
    return fs::exists(path) && !fs::is_directory(path);
}

inline std::optional<fs::path> getPathIfFile(fs::directory_entry e)
{
    if (!fs::is_regular_file(e)) {
        return std::nullopt;
    }
    return e.path();
}
}

#endif // HELPERS_H
