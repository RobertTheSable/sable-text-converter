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

template<class SourceType = fs::path>
class Group
{
    std::string name;
    std::vector<SourceType> files;
    std::optional<sable::Table> table = std::nullopt;
public:
    Group(const std::string& name) {
        this->name = name;
    }
    void sort() {
        std::sort(files.begin(), files.end());
    }
    void add (SourceType newFile) {
        files.push_back(newFile);
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
    std::optional<sable::Table> getTable() const {
        return table;
    }
    template<class Source>
    static Group buildFromFolder(Source src, const util::Mapper& mapper, int nextAddress) {
//        static_assert (std::is_same_v<SourceType, decltype(SourceType(std::declval<Source>()))>, "Test failed.");
        Group g{src.name()};
        if (auto tablePath = src / SourceType{"table.txt"}; tablePath.isFile()) {
            auto tableStream = tablePath.open();
            if (!tableStream) {
                throw ParseError(Source{tablePath}.errorName() + " could not be opened.");
            }
            Table t{};
            t.setAddress(nextAddress);
            std::vector<std::string> res = t.getDataFromFile(tableStream, mapper);

            g.table = t;
            g.files.reserve(res.size());
            for (auto filename: res) {
                if (auto entryPath = SourceType(src / filename); !Source{entryPath}.exists()) {
                    throw ParseError( "In " + g.name
                                 + ": file " + filename
                                 + " does not exist.");
                } else {
                    g.add(entryPath);
                }

            }
        } else {
            src.list([&g] (SourceType s) {
                g.files.push_back(s);
            });
        }
        g.sort();
        return g;
    }

};

} // namespace files
} // namespace sable

#endif // SABLE_FILES_GROUP_H
