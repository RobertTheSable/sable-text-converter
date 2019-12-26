#ifndef SCRIPT_H
#define SCRIPT_H
#include <map>
#include <string>
#include <stdexcept>
#include "yaml-cpp/yaml.h"
#include "wrapper/filesystem.h"

class Script
{
public:
    static const constexpr char* USE_DIGRAPHS = "HasDigraphs";
    static const constexpr char* BYTE_WIDTH = "ByteWidth";
    static const constexpr char* CMD_CHAR = "CommandValue";
    static const constexpr char* FIXED_WIDTH = "FixedWidth";
    static const constexpr char* MAX_CHAR = "MaxEncodedValue";
    static const constexpr char* MAX_WIDTH = "MaxWidth";
    static const constexpr char* FONT_ADDR = "FontWidthAddress";
    static const constexpr char* SABLE_FONT_ENC = "Encoding";
    static const constexpr char* SABLE_FONT_CMD = "Commands";
    static const constexpr char* SABLE_FONT_EX = "Extras";


    Script();
    Script(const char* inConfig);

    void loadScript(const char* inDir, const std::string& defaultMode, int verbosity = 0) noexcept(false);
    bool writeScript(const YAML::Node& config);
    bool writeFontData(const fs::path& fontFileName, const YAML::Node& includes);
    explicit operator bool() const;
private:
    struct textNode {
        unsigned int address;
        std::string label;
        bool isMenuText;
        bool storeWidth;
        bool printpc;
        int maxWidth;
        std::vector<unsigned short> data;
    };
    struct asmTextNode {
        bool isTable;
        int length;
        std::string text;
    };
    struct fontCharacterData {
        int code;
        int length;
    };

    YAML::Node inConfig;
    bool isScriptValid;
    std::vector<textNode> binNodes;
    std::map<int, asmTextNode> tableText;
    std::map<int, std::string> labels;
    std::map<std::string, int> binLengths;
    std::string defaultMode;
    bool digraphs;
    int byteWidth, commandValue, fixedWidth, maxWidth, fileindex;
    unsigned int nextAddress;

    void writeParseWarning(const fs::path &file, std::string message, int line = 0);
    void throwParseError(fs::path&& file, std::string message, int line = 0) noexcept(false);
    void parseScriptFile(const fs::path &file, std::string& textType, bool isTable = false, bool storeWidths = false);
};

#endif
