#ifndef SABLE_FILES_FINDER_H
#define SABLE_FILES_FINDER_H

#include <fstream>
#include <optional>
#include <algorithm>
#include "data/table.h"
#include "wrapper/filesystem.h"

namespace sable {
namespace files {

class File
{
    fs::path path;
public:
    using StreamType = std::ifstream;
    File(fs::path p);
    std::ifstream open() const;
    void close(std::ifstream&) const;
    explicit operator fs::path() const;

    inline bool exists() const
    {
        return fs::exists(path);
    }
    inline bool isFile() const
    {
        return fs::is_regular_file(path);
    }
    inline bool isDir() const
    {
        return fs::is_directory(path);
    }
    inline std::string name() const {
        return path.filename().string();
    }
    inline std::string errorName() const {
        return fs::absolute(path).string();
    }
    inline fs::path get() const {
        return path;
    }

    template<class T>
    inline bool contains(T entry) const {
        return fs::exists(path / entry);
    }

    template<class Fnc>
    inline void list(Fnc out) const {
        std::for_each(fs::directory_iterator(path), fs::directory_iterator(), out);
    }

    template<class T>
    inline File operator + (T ap) const {
        return File{path / ap};
    }
    template<class T>
    inline File operator / (T ap) const {
        return File{path / ap};
    }
};

} // namespace files
} // namespace sable

#endif // SABLE_FILES_FINDER_H
