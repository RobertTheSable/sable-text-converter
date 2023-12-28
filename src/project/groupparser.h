#ifndef GROUPPARSER_H
#define GROUPPARSER_H

#include <iostream>
#include <fstream>
#include <utility>

#include "parse/fileparser.h"
#include "exceptions.h"
#include "errorhandling.h"

#include "group.h"

namespace sable {

template<class FP = FileParser>
struct GroupParser {
    FP fp;
    template<typename ...Args>
    GroupParser(FontList&& fl, Args ...args): fp(std::move(fl), args...) {}

    const FontList& getFonts() const {
        return fp.bp.m_Parser.getFonts();
    }
    template<class Writer>
    auto processGroup(
        files::Group group,
        const util::Mapper& mapper,
        const std::string& currentDir,
        int nextAddress,
        int dirIndex,
        Writer writer
    ) {
        for (auto file: group) {
            if (!fs::exists(file)) {
                throw ParseError(
                    "In " + group.getName()
                    + ": file " + file.string()
                    + " does not exist."
                );
            }
            std::ifstream input(file.string());
            // probably this can be replaced with std::format whenever I get reliable access to it
            auto handler = [file] (error::Levels l, std::string msg, int line) {
                switch (l) {
                case error::Levels::Error:
                    throw ParseError("Error in text file " + fs::absolute(file).string() + ", line " + std::to_string(line+1) + ": " + msg);
                    break;
                case error::Levels::Warning:
                    std::cerr <<
                        "Warning in " + fs::absolute(file).string() + " on line " + std::to_string(line) +
                        ": " << msg << "\n";
                    break;
                default:
                    break;
                }
            };

            auto r = fp.processFile(input, mapper, currentDir, fs::absolute(file).string(), nextAddress, dirIndex, handler, writer);
            dirIndex = r.dirIndex;
            nextAddress = r.address;
            input.close();
        }
        return nextAddress;
    }
};
}

#endif // GROUPPARSER_H
