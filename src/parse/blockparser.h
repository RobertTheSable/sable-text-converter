#ifndef BLOCKPARSER_H
#define BLOCKPARSER_H

#include "data/textblockrange.h"
#include "errorhandling.h"
#include "util.h"
#include "parse.h"

#include "exceptions.h"
#include <iostream>
#include <string>
#include <vector>

namespace sable {

struct BlockParser
{
    struct Bounds {
        int address;
        std::size_t start, length;
        std::string labelPrefix, fileSuffix;
        int getNextAddress() const {
            return address + length;
        }
    };

    struct Result {
        int line;
        bool done;
        std::vector<unsigned char> data;
        std::vector<Bounds> bankBounds;
    };

    TextParser m_Parser;
    template<typename ...Args>
    BlockParser(FontList&& fl, Args ...args): m_Parser(std::move(fl), args...) {}

    template<class Handler>
    Result parse(
            std::istream& input,
            int line,
            ParseSettings& settings,
            const util::Mapper& mapper,
            Handler errorHandler
    ) {
        size_t lastSize = 0;

        Result r;
        while (input.peek() != EOF) {
            bool done;
            int length;
            try {
                std::tie(done, length) = m_Parser.parseLine(input, settings, std::back_inserter(r.data), mapper);
                line++;
            } catch (std::runtime_error &e) {
                errorHandler(
                    error::Levels::Error,
                    std::string(e.what()),
                    line
                );
            }
            if (settings.maxWidth > 0 && length > settings.maxWidth) {
                errorHandler(
                    error::Levels::Warning,
                    std::string{"Line is longer than the specified max width of "} + std::to_string(settings.maxWidth) + " pixels.",
                    line
                );
            }
            if (done && !r.data.empty()) {
                r.line = line;
                break;
            }
        }
        r.done = (input.peek() == EOF);

        auto blockLength =  r.data.size();
        auto blockEndAddress = settings.currentAddress + blockLength;

        std::size_t dataLength = 0;
        int bankDifference = ((blockEndAddress & 0xFF0000) - (settings.currentAddress & 0xFF0000)) >>16;
        int dataStart = 0;
        auto insertPos = r.bankBounds.end();
        int firstAddress = settings.currentAddress;
        if (bankDifference > 0) {
            if (bankDifference > 1) {
                errorHandler(error::Levels::Error,
                        "read data that would cross two banks, aborting.",
                       r.line
                );
            }
            size_t bankLength = ((settings.currentAddress + blockLength) & 0xFFFF);
            dataLength = blockLength - bankLength;
            settings.currentAddress = mapper.skipToNextBank(settings.currentAddress);

            r.bankBounds.push_back(
                Bounds{
                    .address = settings.currentAddress,
                    .start = dataLength,
                    .length = bankLength,
                    .labelPrefix = "$",
                    .fileSuffix = "bank"
                }
            );

            insertPos = r.bankBounds.begin() + 1;
        } else {
            dataLength = blockLength;
        }

        r.bankBounds.insert(
            insertPos,
            Bounds{
                .address = firstAddress,
                .start = 0,
                .length = dataLength,
                .labelPrefix = "",
                .fileSuffix = ""
            }
        );

        return r;
    }
};

} // namespace sable


#endif // BLOCKPARSER_H
