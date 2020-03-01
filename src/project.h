#ifndef PROJECT_H
#define PROJECT_H

#include <unordered_map>
#include <map>
#include <string>
#include "parse.h"
#include "font.h"
#include "mapping.h"
#include "table.h"

namespace sable {

class Project
{
public:
    Project()=default;
    Project(const std::string& projectDir);
    bool parseText();
    void writeOutput();
    void writeFontData();
    std::string MainDir() const;
    std::string RomsDir() const;
    std::string FontConfig() const;
    std::string TextOutDir() const;
    int getMaxAddress() const;
    explicit operator bool() const;

private:
    struct AddressNode {
        int address;
        std::string label;
        bool isTable;
    };
    struct TextNode {
        std::string files;
        size_t size;
    };
    struct Rom {
        std::string file, name;
        int hasHeader;
        std::vector<std::string> includes;
    };
    friend YAML::convert<sable::Project::Rom>;

    int nextAddress;
    std::string m_MainDir, m_InputDir, m_OutputDir, m_BinsDir, m_TextOutDir, m_RomsDir, m_FontConfig;
    std::vector<std::string> m_Includes, m_Extras, m_FontIncludes;
    std::vector<AddressNode> m_Addresses;
    std::vector<Rom> m_Roms;
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
