#ifndef TABLEFACTORY_H
#define TABLEFACTORY_H

#include <optional>

#include "data/table.h"
#include "project/group.h"
#include "data/mapper.h"
#include "wrapper/filesystem.h"


namespace sable {

namespace files {
    class Folder {
        Group init(fs::path folder, const util::Mapper& mapper);
    public:
        std::optional<Table> table;
        Group group;
        Folder(fs::path folder, const util::Mapper& mapper);
        Table releaseTable();
    };
}

}

#endif // TABLEFACTORY_H
