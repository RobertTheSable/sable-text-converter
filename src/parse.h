#ifndef PARSE_H
#define PARSE_H

#include <istream>
#include <map>
#include <string>
#include <tuple>
#include <yaml-cpp/yaml.h>
#include <boost/locale.hpp>
#include "font.h"
#include "util.h"

using boost::locale::boundary::ssegment_index;

namespace sable {
    typedef std::back_insert_iterator<std::vector<unsigned char>> back_inserter;
    struct ParseSettings {
        bool autoend, printpc;
        std::string mode, label;
        int maxWidth;
        int currentAddress;
    };

    class TextParser
    {
    public:
        TextParser()=default;
        TextParser(const YAML::Node& node, const std::string& defaultMode, util::Mapper mapType = util::Mapper::LOROM);
        struct lineNode{
            bool hasNewLines;
            int length;
            std::vector<unsigned char> data;
        };
        std::pair<bool, int> parseLine(std::istream &input, ParseSettings &settings, back_inserter insert);
        const std::map<std::string, Font>& getFonts() const;
        ParseSettings getDefaultSetting(int address);
    private:
        std::locale m_Locale;
        bool useDigraphs;
        int maxWidth;
        std::map<std::string, Font> m_Fonts;
        std::string defaultFont;
        util::Mapper m_RomType;
        ParseSettings updateSettings(
                const ParseSettings &settings,
                ssegment_index::iterator& it,
                const ssegment_index::const_iterator& end,
                unsigned int currentAddress = 0
                );
        static void insertData(unsigned int code, int size, back_inserter bi);
    };
}


#endif // PARSE_H
