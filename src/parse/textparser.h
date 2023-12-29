#ifndef PARSE_H
#define PARSE_H

#include <istream>
#include <map>
#include <string>
#include <tuple>
#include <memory>
#include "font/font.h"
#include "util.h"

namespace sable {
    typedef std::back_insert_iterator<std::vector<unsigned char>> back_inserter;
    struct ParseSettings {
        bool autoend, printpc;
        std::string mode, label;
        int maxWidth;
        int currentAddress;
        int page;
    };

    class TextParser
    {
        struct Impl;
        std::unique_ptr<Impl> _pImpl;
    public:
        // these need to be defaulted externally for the pImpl idiom to work
        ~TextParser();
        TextParser(std::map<std::string, sable::Font>&& list, const std::string& defaultMode, const std::string& locale);
        TextParser& opterator();
        struct lineNode{
            bool hasNewLines;
            int length;
            std::vector<unsigned char> data;
        };
        std::pair<bool, int> parseLine(std::istream &input, ParseSettings &settings, back_inserter insert, const util::Mapper& mapper);
        const std::map<std::string, sable::Font>& getFonts() const;
        ParseSettings getDefaultSetting(int address) const;
    };
}


#endif // PARSE_H
