#ifndef GROUPPARSER_H
#define GROUPPARSER_H

#include <iostream>
#include <fstream>
#include <utility>
#include <optional>

#include "group.h"
#include "folder.h"
#include "handler.h"

namespace sable {

template<typename Hl = Handler>
class GroupParser {
    Hl& handler;
public:
    GroupParser(Hl& handler_): handler{handler_} {}

    auto processGroup(
        files::Group group,
        const util::Mapper& mapper,
        const std::string& currentDir,
        int dirIndex
    ) {
        int nextAddtess = handler.getNextAddress(currentDir);

        for (auto file: group) {
            if (!fs::exists(file)) {
                throw ParseError(
                    "In " + group.getName()
                    + ": file " + file.string()
                    + " does not exist."
                );
            }
            std::ifstream input(file.string());

            auto r = handler.processFile(input, mapper, currentDir, fs::absolute(file).string(), nextAddtess, dirIndex);

            dirIndex = r.dirIndex;
            nextAddtess = r.address;
            input.close();
        }

        handler.setNextAddress(nextAddtess);
    }
};
}

#endif // GROUPPARSER_H
