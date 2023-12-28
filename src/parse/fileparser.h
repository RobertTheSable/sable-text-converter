#ifndef FILEPARSER_H
#define FILEPARSER_H

#include "data/textblockrange.h"
#include "errorhandling.h"
#include "util.h"
#include "parse.h"

#include "exceptions.h"
#include <iostream>
#include <string>
#include <vector>

namespace sable {

struct TextBlock {
    std::vector<unsigned char> data;
    TextBlockRange range;
    std::string label;
};



struct BlockParser
{
    struct Result {
        int line;
        bool done;
        std::vector<unsigned char> data;
    };

    TextParser m_Parser;

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
                r.done = (input.peek() == EOF);
                return r;
            }
        }
        r.done = true;
        return r;
    }
};

} // namespace sable

#endif // FILEPARSER_H
