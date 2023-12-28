#ifndef FILEPARSER_H
#define FILEPARSER_H

#include <vector>
#include <istream>

#include "block.h"
#include "textparser.h"
#include "util.h"
#include "result.h"

namespace sable {

template <class Derived>
class Parser : public TextParser {
    Blocks textRanges;
public:
    using TextParser::TextParser;

    parse::FileResult processFile(
        std::istream& input,
        const util::Mapper& mapper,
        const std::string& currentDir,
        const std::string& fileKey,
        int nextAddress,
        int startingDirIndex
    ) {
        int dirIndex = startingDirIndex;
        auto settings = getDefaultSetting(nextAddress);

        int line = 0;

        std::vector<unsigned char> data;
        while (input) {
            bool done;
            int length;
            try {
                std::tie(done, length) = parseLine(input, settings, std::back_inserter(data), mapper);
                line++;
            } catch (std::runtime_error &e) {
                static_cast<Derived*>(this)->report(
                    fileKey,
                    error::Levels::Error,
                    std::string(e.what()),
                    line + 1
                );
            }
            if (settings.maxWidth > 0 && length > settings.maxWidth) {
                static_cast<Derived*>(this)->report(
                    fileKey,
                    error::Levels::Warning,
                    std::string{"Line is longer than the specified max width of "} + std::to_string(settings.maxWidth) + " pixels.",
                    line
                );
            }
            if (!done || data.empty()) {
                continue;
            }
            Block bl(settings.currentAddress, mapper.skipToNextBank(settings.currentAddress), std::move(data));

            data = {};

            if (auto& fl = getFonts(); fl.find(settings.mode) == fl.end()) {
                continue;
            }
            if (settings.label.empty()) {
                settings.label = currentDir + '_' + std::to_string(dirIndex++);
            }

            for (auto b: bl.bankBounds) {
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
                        static_cast<Derived*>(this)->report(
                            fileKey,
                            error::Levels::Warning,
                            std::string{"block \""} + settings.label + "\" collides with block \"" +
                                result->label + "\" from file \"" + result->file + "\".",
                            line
                        );
                    }
                } // collision check end

                static_cast<Derived*>(this)->write(
                    baseOutputFileName,
                    b.labelPrefix + settings.label,
                    bl.data,
                    b.address,
                    b.start,
                    b.length,
                    settings.printpc
                );

                settings.printpc = false;
            }
            settings.currentAddress = bl.getNextAddress();
            settings.label = "";

        }

        settings.label = "";
        settings.printpc = false;

        nextAddress = settings.currentAddress;

        if (settings.maxWidth < 0) {
            settings.maxWidth = 0;
        }
        return parse::FileResult{dirIndex, nextAddress};
    }
};

}

#endif // FILEPARSER_H
