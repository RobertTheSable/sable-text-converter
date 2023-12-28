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
    std::optional<Table> table = std::nullopt;
public:
    template<class Check>
    Group(
        const std::string& name_,
        fs::path path_,
        Check checker,
        std::istream& tableFile,
        const util::Mapper& mapper,
        int nextAddress
    ) {
        name = name_;
        path = path_;
        table = Table{};
        table->setAddress(nextAddress);
        auto res = table->getDataFromFile(tableFile, mapper);
        files.reserve(res.size());
        for (auto f: res) {
            files.push_back(path / f);
        }
    }

    template<class Itr, class Filter>
    Group(const std::string& name_, fs::path path_, Filter filter, Itr begin, Itr end) {
        name = name_;
        path = path_;
        for (auto it = begin; it != end; ++it) {
            auto res = filter(*it);
            if (!res) {
                continue;
            }
            files.push_back(*res);
        }
        sort();
    }
    void sort() {
        std::sort(files.begin(), files.end());
    }

    auto begin() {
        return files.begin();
    }
    auto begin() const {
        return files.cbegin();
    }
    auto end() {
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
    std::optional<Table> getTable() const {
        return table;
    }
};

} // namespace files
} // namespace sable

#endif // SABLE_FILES_GROUP_H
