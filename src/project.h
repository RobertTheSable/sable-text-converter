#ifndef PROJECT_H
#define PROJECT_H

#include <unordered_map>
#include <map>
#include <string>
#include <yaml-cpp/yaml.h>
#include "util.h"
#include "font/font.h"
#include "data/table.h"

namespace sable {

typedef std::vector<std::string> StringVector;

//class DataInterface {
//public:
//    virtual ~DataInterface() {};
//    virtual std::string toString();
//};

//class Text : public DataInterface {
//private:
//    std::string m_Files;
//    size_t m_Size;
//    bool m_PrintPc;
//public:
//    Text (const std::string& files, const size_t& size, bool printPC);
//    virtual std::string toString();
//};


class Project
{
public:
    Project(const YAML::Node &config, const std::string &projectDir);
    Project(const std::string& projectDir);
    void init(const YAML::Node &config, const std::string &projectDir);
    bool parseText();
    void writePatchData();
    std::string MainDir() const;
    std::string RomsDir() const;
    std::string FontConfig() const;
    std::string TextOutDir() const;
    int getMaxAddress() const;
    explicit operator bool() const;
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
    static constexpr const char* USE_MIRRORED_BANKS = "useMirrorBanks";
    static constexpr const char* OUT_SIZE = "outputSize";
    static constexpr const char* LOCALE = "locale";


private:
    struct Rom {
        std::string file, name;
        int hasHeader;
        std::vector<std::string> includes;
    };
    friend YAML::convert<sable::Project::Rom>;

    std::string m_MainDir, m_InputDir, m_OutputDir, m_BinsDir,
    m_TextOutDir, m_RomsDir, m_FontDir,
    m_DefaultMode, m_ConfigPath, m_LocaleString;
    size_t m_OutputSize;
    StringVector m_Includes, m_Extras, m_FontIncludes;
    std::vector<Rom> m_Roms;
    std::vector<std::string> m_MappingPaths;
    util::MapperType m_BaseType;
    util::Mapper m_Mapper;
    static bool validateConfig(const YAML::Node& configYML);
    int maxAddress;
    void writeSettings();

};
}

namespace YAML {
    template <>
    struct convert<sable::Project::Rom> {
        static bool decode(const Node& node, sable::Project::Rom& rhs);
    };
}

#endif // PROJECT_H
