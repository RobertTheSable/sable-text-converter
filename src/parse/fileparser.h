#ifndef FILEPARSER_H
#define FILEPARSER_H

#include <vector>
#include <istream>

#include "blockparser.h"
#include "util.h"
#include "data/addresslist.h"

namespace sable {

//template<class Handler>
struct FileParser {
    BlockParser bp;
    util::Mapper mapper;
    Blocks textRanges;

    template<class Handler, class Writer>
    int processFile(
        std::istream& input,
        std::string currentDir,
        std::string fileKey,
        int nextAddress,
        int startingDirIndex,
        Handler handler,
        Writer writer
    ) {
        int dirIndex = startingDirIndex;
        auto settings = bp.m_Parser.getDefaultSetting(nextAddress);

        int line = 0;

        while (input) {
            auto parseResult = bp.parse<Handler>(input, line, settings, mapper, handler);
            if (parseResult.data.empty() || !bp.m_Parser.getFonts()[settings.mode]) {
                continue;
            }
            if (settings.label.empty()) {
                settings.label = currentDir + '_' + std::to_string(dirIndex++);
            }

            for (auto b: parseResult.bankBounds) {
                auto baseOutputFileName = settings.label + b.fileSuffix + ".bin";

                // check for collisions
                {
                    int blockLocation = mapper.ToPC(b.address);
                    if (auto result = textRanges.addBlock(
                            blockLocation,
                            blockLocation + b.length,
                            settings.label,
                            fileKey
                        ); result != Blocks::Collision::None) {
                        handler(error::Levels::Warning,
                                std::string{"block \""} + settings.label + "\" collides with block \"" +
                                result->label + "\" from file \"" + result->file + "\".",
                               parseResult.line
                        );
                    }
                } // collision check end

                writer(
                    baseOutputFileName,
                    b.labelPrefix + settings.label,
                    parseResult.data,
                    b.address,
                    b.start,
                    b.length,
                    settings.printpc
                );

                settings.printpc = false;
            }
            settings.currentAddress = parseResult.bankBounds.back().getNextAddress();

        }

        settings.label = "";
        settings.printpc = false;

        nextAddress = settings.currentAddress;

        if (settings.maxWidth < 0) {
            settings.maxWidth = 0;
        }
        return dirIndex;
    }
};

}

#endif // FILEPARSER_H
