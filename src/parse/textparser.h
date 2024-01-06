#ifndef PARSE_H
#define PARSE_H

#include <istream>
#include <map>
#include <string>
#include <tuple>
#include <memory>
#include "font/font.h"
#include "util.h"
#include "data/options.h"

namespace sable {
    typedef std::back_insert_iterator<std::vector<unsigned char>> back_inserter;
    struct ParseSettings {
        enum class Autoend {
            On, Off
        } autoend;
        bool printpc;
        std::string mode, label;
        int maxWidth;
        int currentAddress;
        int page;
        enum class EndOnLabel {
            On, Off
        } endOnLabel;
        sable::options::ExportAddress exportAddress;
        sable::options::ExportWidth exportWidth;
    };

    class TextParser
    {
        struct Impl;
        std::unique_ptr<Impl> _pImpl;
        options::ExportWidth defaultExportWidth_;
        options::ExportAddress defaultExportAddress_;
    public:
        enum class Metadata {
            Yes, No
        };
        struct Result {
            bool endOfBlock;
            int length;
            std::string label;
            Metadata metadata;
        };
        // these need to be defaulted externally for the pImpl idiom to work
        ~TextParser();
        TextParser(
            std::map<std::string, sable::Font>&& list,
            const std::string& defaultMode,
            const std::string& locale,
            options::ExportWidth defaultExportWidth,
            options::ExportAddress defaultExportAddress
        );
        TextParser& opterator();
        struct lineNode{
            bool hasNewLines;
            int length;
            std::vector<unsigned char> data;
        };

        Result parseLine(
                std::istream &input,
                ParseSettings &settings,
                back_inserter insert,
                Metadata lastReadWasMetadata,
                const util::Mapper& mapper
        );
        const std::map<std::string, sable::Font>& getFonts() const;
        ParseSettings getDefaultSetting(int address) const;
    };
}


#endif // PARSE_H
