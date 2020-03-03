#ifndef PROJECT_H
#define PROJECT_H

#include <unordered_map>
#include <map>
#include <string>
#include <yaml-cpp/yaml.h>
#include "parse.h"
#include "font.h"
#include "mapping.h"
#include "table.h"

namespace sable {

typedef std::vector<std::string> StringVector;

class Project
{
public:
    Project()=default;
    Project(const YAML::Node &config, const std::string &projectDir);
    Project(const std::string& projectDir);
    void init(const YAML::Node &config, const std::string &projectDir);
    bool parseText();
    void writePatchData();
    void writeFontData();
    std::string MainDir() const;
    std::string RomsDir() const;
    std::string FontConfig() const;
    std::string TextOutDir() const;
    int getMaxAddress() const;
    explicit operator bool() const;
    int getWarningCount() const;
    StringVector::const_iterator getWarnings() const;

    static constexpr const char* FILES_SECTION = "files";
    static constexpr const char* INPUT_SECTION = "input";
    static constexpr const char* OUTPUT_SECTION = "output";
    static constexpr const char* OUTPUT_BIN = "binaries";
    static constexpr const char* CONFIG_SECTION = "config";
    static constexpr const char* IN_MAP = "inMapping";
    static constexpr const char* DIR_VAL = "directory";
    static constexpr const char* DIR_MAIN = "mainDir";
    static constexpr const char* DIR_TEXT = "textDir";
    static constexpr const char* DIR_ROM = "romDir";
    static constexpr const char* DIR_FONT = "dir";
    static constexpr const char* FONT_SECTION = "fonts";
    static constexpr const char* INCLUDE_VAL = "includes";
    static constexpr const char* EXTRAS = "extras";
    static constexpr const char* ROMS = "roms";
    static constexpr const char* DEFAULT_MODE = "defaultMode";
    static constexpr const char* MAP_TYPE = "mapper";


private:
    struct AddressNode {
        int address;
        std::string label;
        bool isTable;
    };
    struct TextNode {
        std::string files;
        size_t size;
        bool printpc;
    };
    struct Rom {
        std::string file, name;
        int hasHeader;
        std::vector<std::string> includes;
    };
    friend YAML::convert<sable::Project::Rom>;

    int nextAddress;
    std::string m_MainDir, m_InputDir, m_OutputDir, m_BinsDir, m_TextOutDir, m_RomsDir, m_FontDir;
    StringVector m_Includes, m_Extras, m_FontIncludes;
    std::vector<AddressNode> m_Addresses;
    std::vector<Rom> m_Roms;
    StringVector m_Warnings;
    std::unordered_map<std::string, TextNode> m_TextNodeList;
    std::unordered_map<std::string, Table> m_TableList;
    TextParser m_Parser;
    void outputFile(const std::string &file, const std::vector<unsigned char>& data, size_t length, int start = 0);
    static bool validateConfig(const YAML::Node& configYML);
};
}

namespace YAML {
    template <>
    struct convert<sable::Project::Rom> {
        static bool decode(const Node& node, sable::Project::Rom& rhs);
    };
}

#endif // PROJECT_H
