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
        bool keepReading = false;
        Metadata lastRead = Metadata::No;
        while (input || keepReading) {
            keepReading = false;
            TextParser::Result rs {false, 0, settings.label};
            try {
                rs = parseLine(input, settings, std::back_inserter(data), lastRead, mapper);
                line++;
            } catch (std::runtime_error &e) {
                static_cast<Derived*>(this)->report(
                    fileKey,
                    error::Levels::Error,
                    std::string(e.what()),
                    line + 1
                );
            }
            if (settings.maxWidth > 0 && rs.length > settings.maxWidth) {
                static_cast<Derived*>(this)->report(
                    fileKey,
                    error::Levels::Warning,
                    std::string{"Line is longer than the specified max width of "} + std::to_string(settings.maxWidth) + " pixels.",
                    line
                );
            }
            lastRead = rs.metadata;
            if (!rs.endOfBlock || data.empty()) {
                keepReading |= (!data.empty() || lastRead == Metadata::Yes);
                continue;
            }
            Block bl(
                settings.currentAddress,
                mapper.skipToNextBank(settings.currentAddress),
                std::move(data)
            );

            data = {};

            if (auto& fl = getFonts(); fl.find(settings.mode) == fl.end()) {
                continue;
            }
            if (rs.label.empty()) {
                rs.label = currentDir + '_' + std::to_string(dirIndex++);
            }

            for (auto b: bl.bankBounds) {
                auto baseOutputFileName = rs.label + b.fileSuffix + ".bin";

                // check for collisions
                {
                    int blockLocation = mapper.ToPC(b.address);
                    if (auto result = textRanges.addBlock(
                            blockLocation,
                            blockLocation + b.length,
                            rs.label,
                            fileKey
                        ); result != Blocks::Collision::None) {
                        static_cast<Derived*>(this)->report(
                            fileKey,
                            error::Levels::Warning,
                            std::string{"block \""} + rs.label + "\" collides with block \"" +
                                result->label + "\" from file \"" + result->file + "\".",
                            line
                        );
                    }
                } // collision check end

                static_cast<Derived*>(this)->write(
                    baseOutputFileName,
                    b.labelPrefix + rs.label,
                    bl.data,
                    b.address,
                    b.start,
                    b.length,
                    settings.printpc,
                    settings.exportWidth,
                    settings.exportAddress
                );

                settings.printpc = false;
            }
            settings.currentAddress = bl.getNextAddress();
            if (rs.label == settings.label) {
                settings.label = "";
            }
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
