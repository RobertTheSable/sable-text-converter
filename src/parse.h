#ifndef PARSE_H
#define PARSE_H

#include <istream>
#include <map>
#include <string>
#include <tuple>
#include <yaml-cpp/yaml.h>
#include "font.h"
#include "util.h"

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
        TextParser(const YAML::Node& node, const std::string& defaultMode);
        struct lineNode{
            bool hasNewLines;
            int length;
            std::vector<unsigned char> data;
        };
        std::pair<bool, int> parseLine(std::istream &input, ParseSettings &settings, back_inserter insert);
        const std::map<std::string, Font>& getFonts() const;
        ParseSettings getDefaultSetting(int address);
    private:
        bool useDigraphs;
        int maxWidth;
        std::map<std::string, Font> m_Fonts;
        std::string defaultFont;
        static std::string readUtf8Char(std::string::iterator& start, std::string::iterator end, bool advance = true);
        ParseSettings updateSettings(const ParseSettings &settings, const std::string& setting = "", unsigned int currentAddress = 0);
        static void insertData(unsigned int code, int size, back_inserter bi);
    };
}


#endif // PARSE_H
