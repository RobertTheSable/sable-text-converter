#include "folder.h"

#include <fstream>
#include <algorithm>
#include <utility>

#include "exceptions.h"

sable::files::Group sable::files::Folder::init(fs::path folder, const util::Mapper &mapper)
{

    std::string name = folder.filename().string();
    if (auto tablePath = folder / "table.txt"; fs::is_regular_file(tablePath)) {
        std::ifstream tableFile(tablePath.string());
        if (!tableFile) {
            throw ParseError(fs::absolute(tablePath).string() + " could not be opened.");
        }
        table = Table{};
        auto files = table->getDataFromFile(tableFile, mapper);
        return Group::fromTable(name, folder, files.begin(), files.end());
    }
    return Group::fromList(name, folder, fs::directory_iterator(folder), fs::directory_iterator());
}

sable::files::Folder::Folder(fs::path folder, const util::Mapper &mapper)
    : group{init(folder, mapper)}
{
}

sable::Table sable::files::Folder::releaseTable()
{
    return std::exchange(table, std::nullopt).value();
}
