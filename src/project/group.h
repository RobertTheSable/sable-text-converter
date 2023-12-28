#ifndef SABLE_FILES_GROUP_H
#define SABLE_FILES_GROUP_H

#include <vector>
#include <string>
#include <algorithm>
#include <optional>
#include <type_traits>
#include "data/table.h"
#include "wrapper/filesystem.h"

namespace sable {

namespace files {

class Group
{
    std::string name;
    fs::path path;
    std::vector<fs::path> files;
    Group(const std::string& name_, fs::path path_) : name{name_}, path{path_} {}
public:
    Group(const Group&)=default;
    Group(Group&&)=default;
    Group& operator=(const Group&)=default;
    Group& operator=(Group&&)=default;

    template<class Itr>
    static auto fromList(
        const std::string& name,
        fs::path path,
        Itr begin,
        Itr end
    )
    {
        Group g(name, path);
        for (auto it = begin; it != end; ++it) {
            if (!it->is_regular_file()) {
                continue;
            }
            g.files.push_back(it->path());
        }
        g.sort();
        return std::move(g);
    }
    template<class Itr>
    static auto fromTable(
        const std::string& name,
        fs::path path,
        Itr begin,
        Itr end
    )
    {
        Group g(name, path);
        g.files.reserve(end - begin);
        for (auto it = begin; it != end; ++it) {
            g.files.push_back(path / *it);
        }
        return std::move(g);
    }
    void sort()
    {
        std::sort(files.begin(), files.end());
    }

    auto begin() {
        return files.begin();
    }
    auto begin() const
    {
        return files.cbegin();
    }
    auto end()
    {
        return files.end();
    }
    auto end() const {
        return files.cend();
    }
    std::string getName() const {
        return name;
    }

    fs::path getPath() const {
        return path;
    }
};

} // namespace files
} // namespace sable

#endif // SABLE_FILES_GROUP_H
