#ifndef PROJECT_H
#define PROJECT_H

#include <unordered_map>
#include <map>
#include <string>
#include <memory>

#include "util.h"
#include "data/table.h"
#include "font/font.h"

namespace sable {

typedef std::vector<std::string> StringVector;

class Project
{
    friend class ProjectSerializer;
    struct Rom {
        std::string file, name;
        int hasHeader;
        std::vector<std::string> includes;
    };

    std::map<std::string, sable::Font> fl;
    std::string m_MainDir, m_InputDir, m_OutputDir, m_BinsDir,
    m_TextOutDir, m_RomsDir, m_FontDir,
    m_DefaultMode, m_ConfigPath, m_LocaleString;
    size_t m_OutputSize;
    StringVector m_Includes, m_Extras, m_FontIncludes;
    std::vector<Rom> m_Roms;
    std::vector<std::string> m_MappingPaths;
    util::MapperType m_BaseType;
    util::Mapper m_Mapper;
    int maxAddress;
    Project(util::Mapper&& mapper);
public:
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

    static Project from(const std::string &projectDir);
    bool parseText();
    void writePatchData();
    std::string MainDir() const;
    std::string RomsDir() const;
    std::string FontConfig() const;
    std::string TextOutDir() const;
    int getMaxAddress() const;
    explicit operator bool() const;
};
}

#endif // PROJECT_H
